/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#include "tasks/fandom_task_processor.h"
#include "include/pagegetter.h"
#include "include/pagetask.h"
#include "include/parsers/ffn/fandomparser.h"
#include "include/transaction.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/fandoms.h"
#include "include/Interfaces/pagetask_interface.h"
#include "include/timeutils.h"

#include <QThread>

FandomLoadProcessor::FandomLoadProcessor(QSqlDatabase db,
                                         QSharedPointer<interfaces::Fanfics> fanficInterface,
                                         QSharedPointer<interfaces::Fandoms> fandomsInterface,
                                         QSharedPointer<interfaces::PageTask> pageInterface,
                                         QObject *obj) : PageConsumer(obj)
{
    this->fanficsInterface = fanficInterface;
    this->fandomsInterface = fandomsInterface;
    this->pageInterface = pageInterface;
    this->db = db;
    CreatePageThreadWorker();
    connect(this, &FandomLoadProcessor::taskStarted, worker.data(), &PageThreadWorker::FandomTask);
}

FandomParseTaskResult FandomLoadProcessor::Run(FandomParseTask task)
{
    this->task = task;
    emit requestProgressbar(task.parts.size());
    StartPageWorker();
    emit taskStarted(task);
    int counter = 0;
    WebPage webPage;

    QSet<QString> updatedFandoms;
    database::Transaction transaction(db);
    FandomParser parser(fanficsInterface);
    An<PageManager> pager;
    FandomParseTaskResult result;
    do
    {
        while(pageQueue.pending && pageQueue.data.isEmpty())
        {
            QThread::msleep(500);
            if(!worker->working)
                pageThread.start(QThread::HighPriority);
            QCoreApplication::processEvents();
        }
        if(!pageQueue.pending && pageQueue.data.isEmpty())
            break;

        webPage = pageQueue.data.at(0);

        pageQueue.data.pop_front();
        webPage.crossover = webPage.url.contains("Crossovers");
        webPage.fandom =  task.fandom;
        webPage.type = EPageType::sorted_ficlist;

        if(webPage.failedToAcquire)
        {
            result.failedParts.push_back(webPage.url);
            result.failedToAcquirePages = true;
            continue;
        }
        TimeKeeper timeKeeper;
        parser.ProcessPage(webPage);
        timeKeeper.Log("page process", "start");
        if(webPage.source == EPageSource::network)
            pager->SavePageToDB(webPage);

        QCoreApplication::processEvents();
        emit updateCounter(counter);

        QString pageProto = "Min Update: " + webPage.minFicDate.toString("yyMMdd") + " Url: %1 <br>";
        thread_local QString url_proto = "<a href=\"%1\"> %1 </a>";
        QString source = webPage.isFromCache ? "CACHE:" : "WEB:";

        emit updateInfo(source + " " + pageProto.arg(url_proto.arg(webPage.url)));

        timeKeeper.Track("dbwrite start");
        {
            auto startQueue= std::chrono::high_resolution_clock::now();

            fanficsInterface->ProcessIntoDataQueues(parser.processedStuff);
            auto elapsedQueue = std::chrono::high_resolution_clock::now() - startQueue;
            qDebug() << "Queue processed in: " << std::chrono::duration_cast<std::chrono::microseconds>(elapsedQueue).count();
            auto startFandoms= std::chrono::high_resolution_clock::now();
            auto fandoms = fandomsInterface->EnsureFandoms(parser.processedStuff);
            auto elapsedFandoms = std::chrono::high_resolution_clock::now() - startFandoms;
            qDebug() << "Fandoms processed in: " << std::chrono::duration_cast<std::chrono::microseconds>(elapsedFandoms).count();
            updatedFandoms.intersect(fandoms);

            timeKeeper.Track("flush start");
            auto flushResult = fanficsInterface->FlushDataQueues();
            if(!flushResult)
                result.criticalErrors = true;

            result.updatedFics += fanficsInterface->updatedCounter;
            result.addedFics   += fanficsInterface->insertedCounter;
            result.skippedFics += fanficsInterface->skippedCounter;
            result.parsedPages += counter;
            timeKeeper.Log("flush finished", "flush start");
        }
        timeKeeper.Log("dbwrite finished", "dbwrite start");
    }while(pageQueue.pending || pageQueue.data.size() > 0);
    fandomsInterface->RecalculateFandomStats(updatedFandoms.values());
    result.finished = true;
    transaction.finalize();
    StopPageWorker();
    return result;
}

static SubTaskPtr CreateAdditionalSubtask(PageTaskPtr task)
{
    SubTaskPtr result;
    if(!task || task->subTasks.size() == 0)
        return result;
    auto newSubtask = task->subTasks.first()->Spawn();
    newSubtask->content = task->subTasks.first()->content->Spawn();
    return result;
}

void FandomLoadProcessor::Run(PageTaskPtr task)
{
    if(!task->taskComment.isEmpty())
        emit updateInfo(task->taskComment + "<br>");

    QStringList acquisitioFailures;
    QList<SubTaskPtr> subsToInsert;
    for(auto subtask : task->subTasks)
    {
        if(subtask->finished)
            continue;
        acquisitioFailures.clear();
        auto urlList= subtask->content->ToDB().split("\n",QString::SkipEmptyParts);

        FandomParseTask fpt;
        fpt.cacheMode = task->cacheMode;
        fpt.pageRetries = subtask->allowedRetries;
        fpt.parts = urlList;
        fpt.stopAt = subtask->updateLimit.date();
        fpt.fandom = subtask->content->CustomData1();

        auto result = Run(fpt);
        task->updatedFics += result.updatedFics;
        task->addedFics   += result.addedFics;
        task->skippedFics += result.skippedFics;
        task->parsedPages += result.parsedPages;

        if(result.failedToAcquirePages)
        {
            // need to write pages that we failed to acquire into errors
            acquisitioFailures +=result.failedParts;
            subtask->success = false;
        }
        else
            subtask->success = true;
        subtask->finished = true;

        SubTaskPtr sub;
        if(acquisitioFailures.size() > 0)
        {
            task->success = false;
            sub = CreateAdditionalSubtask(task);
        }
        if(sub)
        {
            sub->taskComment = "Failed pages";
            sub->isValid = true;
            auto content = sub->content;
            auto cast = static_cast<SubTaskFandomContent*>(content.data());
            cast->fandom = subtask->content->CustomData1();
            cast->urlLinks = acquisitioFailures;
            subsToInsert.push_back(sub);
        }
        fandomsInterface->SetLastUpdateDate(subtask->content->CustomData1(), QDateTime::currentDateTimeUtc().date());
    }
    if(subsToInsert.size() == 0)
        task->success = true;
    else
        task->success =false;
    for(auto sub: subsToInsert )
        task->subTasks.push_back(sub);
    task->finished = true;

    pageInterface->WriteTaskIntoDB(task);
}
