/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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
#include "tasks/author_task_processor.h"
#include "include/pagegetter.h"
#include "include/pagetask.h"
#include "include/parsers/ffn/fandomparser.h"
#include "include/transaction.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/fandoms.h"
#include "include/Interfaces/pagetask_interface.h"
#include "include/Interfaces/authors.h"
#include "include/url_utils.h"
#include "include/timeutils.h"
#include "include/parsers/ffn/favparser.h"
#include "include/page_utils.h"
#include "include/pagetask.h"
#include "include/generic_utils.h"

#include <QThread>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

AuthorLoadProcessor::AuthorLoadProcessor(QSqlDatabase db,
                                         QSqlDatabase taskDb,
                                         QSharedPointer<interfaces::Fanfics> fanficInterface,
                                         QSharedPointer<interfaces::Fandoms> fandomsInterface,
                                         QSharedPointer<interfaces::Authors> authorsInterface,
                                         QSharedPointer<interfaces::PageTask> pageTaskInterface,
                                         QObject *obj) : PageConsumer(obj)
{
    this->fanfics = fanficInterface;
    this->fandoms = fandomsInterface;
    this->authors = authorsInterface;
    this->pageInterface = pageTaskInterface;

    this->db = db;
    this->taskDB = taskDb;
    CreatePageThreadWorker();
    connect(this, &AuthorLoadProcessor::pageTaskList, worker.data(), &PageThreadWorker::TaskList);
}
static core::AuthorPtr CreateAuthorFromNameAndUrl(QString name, QString url)
{
    core::AuthorPtr author(new core::Author);
    author->name = name;
    author->SetWebID("ffn", url_utils::GetWebId(url, "ffn").toInt());
    return author;
}

void WriteProcessedFavourites(FavouriteStoryParser& parser,
                              core::AuthorPtr author,
                              QSharedPointer<interfaces::Fanfics> fanficsInterface,
                              QSharedPointer<interfaces::Authors> authorsInterface,
                              QSharedPointer<interfaces::Fandoms> fandomsInterface)
{
    QSet<int> uniqueAuthors;
    fanficsInterface->ProcessIntoDataQueues(parser.processedStuff);
    auto fandoms = fandomsInterface->EnsureFandoms(parser.processedStuff);
    fandoms.intersect(fandoms);
    QList<core::FicRecommendation> tempRecommendations;
    tempRecommendations.reserve(parser.processedStuff.size());
    uniqueAuthors.reserve(parser.processedStuff.size());

    for(auto& section : parser.processedStuff)
    {
        if(section->ficSource == core::Fic::efs_own_works)
            continue;

        tempRecommendations.push_back({section, author});
        if(!uniqueAuthors.contains(section->author->GetWebID("ffn")))
            uniqueAuthors.insert(section->author->GetWebID("ffn"));
    }
    fanficsInterface->AddRecommendations(tempRecommendations);
    auto result =fanficsInterface->FlushDataQueues();
    //todo this also needs to be done everywhere
    authorsInterface->UploadLinkedAuthorsForAuthor(author->id, "ffn", uniqueAuthors.values());
}

void AuthorLoadProcessor::Run(PageTaskPtr task)
{
    qDebug() << "///////////////////////////////////////////";
    qDebug() << "/////////////   NEW TASK   ////////////////";
    qDebug() << "///////////////////////////////////////////";

    int currentCounter = 0;
    auto fanfics= this->fanfics;
    auto authors= this->authors;
    auto fandoms = this->fandoms;
    pageInterface->SetCurrentTask(task);
    auto job = [fanfics,authors, fandoms](QString url, QString content){
        FavouriteStoryParser parser(fanfics);
        parser.ProcessPage(url, content);
        return parser;
    };

    QList<QFuture<FavouriteStoryParser>> futures;
    QList<FavouriteStoryParser> parsers;

    int cachedPages = 0;
    int loadedPages = 0;
    bool breakerTriggered = false;
    StartPageWorker();
    bool breakTriggered = false;
    for(auto subtask : task->subTasks)
    {
        if(pageInterface->IsForceStopActivated(task->id))
        {
            breakTriggered = true;
            break;
        }

        auto startSubtask = std::chrono::high_resolution_clock::now();
        emit updateInfo(QString("<br>Starting new subtask: %1 <br>").arg(subtask->id));

        pageQueue.pending = true;
        qDebug() << "starting new subtask: " << subtask->id;
        // well this is bollocks :) wtf
        if(breakerTriggered)
            break;

        if(!subtask)
            continue;

        if(subtask->finished)
        {
            qDebug() << "subtask: " << subtask->id << " already processed its " << QString::number(subtask->content->size()) << "entities";
            qDebug() << "skipping";
            currentCounter+=subtask->content->size();
            emit updateCounter(currentCounter);
            continue;
        }
        if(subtask->attempted)
            qDebug() << "Retrying attempted task: " << task->id << " " << subtask->id;


        subtask->SetInitiated(dbInterface->GetCurrentDateTime());

        auto cast = static_cast<SubTaskAuthorContent*>(subtask->content.data());
        database::Transaction transaction(db);
        //database::Transaction pcTransaction(pageInterface->db);

        pageQueue.data.clear();
        pageQueue.data.reserve(cast->authors.size());
        emit pageTaskList(cast->authors, subtask->parent.toStrongRef()->cacheMode);

        WebPage webPage;
        QSet<QString> fandomsSet;
        int updatedAuthorCounter = -1;

        do
        {
            updatedAuthorCounter++;
            currentCounter++;
            if(currentCounter%300 == 0)
                emit resetEditorText();

            if(currentCounter%50 == 0)
            {
                QLOG_INFO() << "=========================================================================";
                QLOG_INFO() << "At this moment processed:  "<< currentCounter << " authors of: " << subtask->size;
                QLOG_INFO() << "=========================================================================";
            }

            futures.clear();
            parsers.clear();
            int waitCounter = 10;
            while((pageQueue.pending && pageQueue.data.size() == 0) || (pageQueue.pending && waitCounter > 0))
            {
                QCoreApplication::processEvents();
                waitCounter--;
            }

            if(!pageQueue.pending && pageQueue.data.size() == 0)
                break;

            if(cancelCurrentTaskPressed)
            {
                cancelCurrentTaskPressed = false;
                breakerTriggered = true;
                break;
            }

            webPage = pageQueue.data[0];
            pageQueue.data.pop_front();

            if(!webPage.isFromCache)
                loadedPages++;
            else
                cachedPages++;

            qDebug() << "Page loaded in: " << webPage.LoadedIn();
            emit updateCounter(currentCounter);
            auto webId = url_utils::GetWebId(webPage.url, "ffn").toInt();
            auto author = authors->GetByWebID("ffn", webId);

            QCoreApplication::processEvents();
            {
                qDebug() << "processing page:" << webPage.pageIndex << " " << webPage.url;
                auto startRecLoad = std::chrono::high_resolution_clock::now();
                auto splittings = page_utils::SplitJob(webPage.content);

                for(auto part: splittings.parts)
                {
                    futures.push_back(QtConcurrent::run(job, webPage.url, part.data));
                }
                for(auto future: futures)
                {
                    future.waitForFinished();
                    parsers+=future.result();
                }

                auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;

                FavouriteStoryParser sumParser;
                QString name = ParseAuthorNameFromFavouritePage(webPage.content);
                // need to create author when there is no data to parse

                auto author = CreateAuthorFromNameAndUrl(name, webPage.url);
                author->favCount = splittings.favouriteStoryCountInWhole;
                qDebug() << "total: " << splittings.favouriteStoryCountInWhole;
                author->ficCount = splittings.authorStoryCountInWhole;
                auto webId = url_utils::GetWebId(webPage.url, "ffn").toInt();
                author->SetWebID("ffn", webId);
                sumParser.SetAuthor(author);


                authors->EnsureId(sumParser.recommender.author);
                FavouriteStoryParser::MergeStats(author,fandoms, parsers);
                authors->UpdateAuthorRecord(author);
                author = authors->GetByWebID("ffn", webId);
                for(auto actualParser: parsers)
                    sumParser.processedStuff+=actualParser.processedStuff;

                QString result;
                if(webPage.isFromCache)
                    result+= "CACHE   ";
                else
                    result+= "WEB     ";
                result += webPage.url + " " + name + ": All Faves:  " + QString::number(sumParser.processedStuff.size()) + " " ;
                if(sumParser.processedStuff.size() != (splittings.favouriteStoryCountInWhole + splittings.authorStoryCountInWhole))
                {
                    qDebug() << "something is wrong: proc: " << sumParser.processedStuff.size() << " sum:" << splittings.favouriteStoryCountInWhole + splittings.authorStoryCountInWhole;
                }

                result+="<br>";
                emit updateInfo(result);

                WriteProcessedFavourites(sumParser, author, fanfics, authors, fandoms);
                subtask->updatedFics = fanfics->updatedCounter;
                subtask->addedFics = fanfics->insertedCounter;
                subtask->skippedFics = fanfics->skippedCounter;

                if(fanfics->skippedCounter > 0)
                    qDebug() << "skipped: " << fanfics->skippedCounter;

                elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
            }

        }while(pageQueue.pending || pageQueue.data.size() > 0);
        subtask->updatedAuthors = updatedAuthorCounter;
        subtask->SetFinished(dbInterface->GetCurrentDateTime());

        task->updatedFics += subtask->updatedFics;
        task->addedFics   += subtask->addedFics;
        task->skippedFics += subtask->skippedFics;
        task->parsedPages += currentCounter - 1;

        pageInterface->WriteSubTaskIntoDB(subtask);
        fandoms->RecalculateFandomStats(fandomsSet.values());
        fanfics->ClearProcessedHash();

        transaction.finalize();
        //pcTransaction.finalize();
        auto elapsed = std::chrono::high_resolution_clock::now() - startSubtask;
        //qDebug() << "Page Processing done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        QString info  = "subtask finished in: " + QString::number(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()) + "<br>";
        emit updateInfo(info);
        emit resetEditorText();
    }
    if(!breakTriggered)
        task->SetFinished(dbInterface->GetCurrentDateTime());
    pageInterface->WriteTaskIntoDB(task);
    StopPageWorker();
}

void AuthorLoadProcessor::OnCancelCurrentTask()
{
    cancelCurrentTaskPressed = true;
}
