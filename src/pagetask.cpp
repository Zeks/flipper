/*Flipper is a replacement search engine for fanfiction.net search results
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
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#include "pagetask.h"
#include "Interfaces/db_interface.h"
#include "include/pure_sql.h"
#include "GlobalHeaders/run_once.h"
#include "include/transaction.h"
#include <QUuid>


SubTaskContentBasePtr SubTaskAuthorContent::NewContent()
{
    return SubTaskContentBasePtr(new SubTaskAuthorContent());
}

SubTaskAuthorContent::~SubTaskAuthorContent()
{

}

QString SubTaskAuthorContent::ToDB()
{
    return authors.join("\n");
}

PageTaskActionPtr PageTaskAction::CreateNewAction()
{
    return PageTaskActionPtr (new PageTaskAction);
}

void PageTaskAction::SetData(QString str, int taskId, int subTaskId)
{
    this->taskId = taskId;
    this->subTaskId = subTaskId;
    this->id = QUuid(str);
}

PageTaskPtr PageTask::CreateNewTask()
{
    return PageTaskPtr(new PageTask);
}

SubTaskPtr PageSubTask::CreateNewSubTask()
{
    return SubTaskPtr(new PageSubTask);
}

PageFailurePtr PageFailure::CreateNewPageFailure()
{
    return PageFailurePtr(new PageFailure);
}

QStringList BasePageTask::ListFailures()
{
    return QStringList();
}

void BasePageTask::SetFinished(QDateTime dt)
{
    finishedAt = dt;
    success = true;
    finished = true;
    // need to close up an action here
}

void BasePageTask::SetInitiated(QDateTime dt)
{
    attempted = true;
    startedAt = dt;
    // need to create an action here
}

SubTaskContentBasePtr SubTaskFandomContent::NewContent()
{
return SubTaskContentBasePtr(new SubTaskFandomContent());
}

SubTaskFandomContent::~SubTaskFandomContent()
{

}

QString SubTaskFandomContent::ToDB()
{
    return urlLinks.join("\n");
}
