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
#include "include/page_utils.h"
#include "include/transaction.h"
#include "include/Interfaces/pagetask_interface.h"
#include <QThread>
#include <QDebug>
namespace page_utils{
SplitJobs SplitJob(QString data, bool splitOnThreads)
{
    SplitJobs result;
    int threadCount;
    if(splitOnThreads)
        threadCount = QThread::idealThreadCount();
    else
        threadCount = 50;
    thread_local QRegExp rxStart("<div\\sclass=\'z-list\\sfavstories\'");
    int index = rxStart.indexIn(data);

    int captured = data.count(" favstories");
    result.favouriteStoryCountInWhole = captured;
    //qDebug() << "RX fav fics:" << captured;

    thread_local QRegExp rxAuthorStories("<div\\sclass=\'z-list\\smystories\'");
    index = rxAuthorStories.indexIn(data);
    int capturedAuthorStories = data.count(rxAuthorStories);
    result.authorStoryCountInWhole = capturedAuthorStories;
    //qDebug() << "RX own fics:" << capturedAuthorStories;
    thread_local QRegExp rxAll("<div\\sclass=\'z-list\\s(favstories|mystories)\'");

    int partSize = (captured+capturedAuthorStories)/(threadCount-1);
    //qDebug() << "In packs of "  << partSize;
    index = 0;

    if(partSize < 40)
        partSize = 40;

    QList<int> splitPositions;
    int counter = 0;
    do{
        index = rxAll.indexIn(data, index+1);
        if(counter%partSize == 0 && index != -1)
        {
            splitPositions.push_back(index);
        }
        counter++;
    }while(index != -1);


    result.parts.reserve(splitPositions.size());
    QStringList partSizes;
    for(int i = 0; i < splitPositions.size(); i++)
    {
        if(i != splitPositions.size()-1)
            result.parts.push_back({data.mid(splitPositions[i], splitPositions[i+1] - splitPositions[i]), i});
        else
            result.parts.push_back({data.mid(splitPositions[i], data.length() - splitPositions[i]),i});
        partSizes.push_back(QString::number(result.parts.last().data.length()));
    }
    return result;
}


PageTaskPtr CreatePageTaskFromUrls(QSharedPointer<interfaces::PageTask>pageTask,
                                   QDateTime currentDateTime,
                                   QStringList urls,
                                   QString taskComment,
                                   int subTaskSize,
                                   int subTaskRetries,
                                   ECacheMode cacheMode,
                                   bool allowCacheRefresh)
{
    database::Transaction transaction(pageTask->db);

    auto timestamp = currentDateTime;
    qDebug() << "Task timestamp" << timestamp;
    auto task = PageTask::CreateNewTask();
    task->allowedSubtaskRetries = subTaskRetries;
    task->cacheMode = cacheMode;
    task->parts = urls.size() / subTaskSize;
    task->refreshIfNeeded = allowCacheRefresh;
    task->taskComment = taskComment;
    task->type = 0;
    task->allowedRetries = 2;
    task->created = timestamp;
    task->isValid = true;
    task->scheduledTo = timestamp;
    task->startedAt = timestamp;
    pageTask->WriteTaskIntoDB(task);

    SubTaskPtr subtask;
    int i = 0;
    int counter = 0;
    do{
        auto last = i + subTaskSize <= urls.size() ? urls.begin() + i + subTaskSize : urls.end();
        subtask = PageSubTask::CreateNewSubTask();
        subtask->type = 0;
        subtask->parent = task;
        auto content = SubTaskAuthorContent::NewContent();
        auto cast = static_cast<SubTaskAuthorContent*>(content.data());
        std::copy(urls.begin() + i, last, std::back_inserter(cast->authors));
        subtask->content = content;
        subtask->parentId = task->id;
        subtask->created = timestamp;
        subtask->size = last - (urls.begin() + i); // fucking idiot
        task->size += subtask->size;
        subtask->id = counter;
        subtask->isValid = true;
        subtask->allowedRetries = subTaskRetries;
        subtask->success = false;
        task->subTasks.push_back(subtask);
        i += subTaskSize;
        counter++;
    }while(i < urls.size());
    // now with subtasks
    pageTask->WriteTaskIntoDB(task);
    transaction.finalize();
    return task;
}
}
