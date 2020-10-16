/*Flipper is a replacement search engine for fanfiction.net search results
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
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#include "Interfaces/pagetask_interface.h"
#include "Interfaces/db_interface.h"
#include "include/pure_sql.h"
#include "include/pagegetter.h"
#include "GlobalHeaders/run_once.h"
#include "include/transaction.h"

namespace interfaces {

void PageTask::WriteTaskIntoDB(PageTaskPtr task)
{
    if(!task || !task->isValid)
        return;
    if(task->NeedsInsertion())
    {
        auto result = database::puresql::CreateTaskInDB(task, db);
        task->id = result.data;
    }
    else
        database::puresql::UpdateTaskInDB(task, db);
    task->isNew = false;

    for(const auto& subtask : std::as_const(task->subTasks))
    {
        if(!subtask || !subtask->isValid)
            continue;
        subtask->parentId = task->id;
        WriteSubTaskIntoDB(subtask);
    }
}

void PageTask::WriteSubTaskIntoDB(SubTaskPtr subtask)
{
    if(!subtask || !subtask->isValid)
        return;
    if(subtask->NeedsInsertion())
    {
        auto result = database::puresql::CreateSubTaskInDB(subtask, db);
        subtask->isNew = false;
    }
    else
        database::puresql::UpdateSubTaskInDB(subtask, db);

    for(const auto& action : std::as_const(subtask->executedActions))
    {
        if(!action || !action->isNewAction)
            continue;

        database::puresql::CreateActionInDB(action, db);
        database::puresql::CreateErrorsInDB(action->errors, db);
    }
}

bool PageTask::DropLastTask()
{
    auto id = GetLastTaskId();
    auto result = database::puresql::SetTaskFinished(id, db);
    return true;
}

bool PageTask::DropTaskId(int id)
{
    auto result = database::puresql::SetTaskFinished(id, db);
    return true;
}

bool PageTask::IsLastTaskSuccessful()
{
    auto id = database::puresql::GetLastExecutedTaskID(db);
    bool success = database::puresql::GetTaskSuccessByID(id.data, db).data;
    return success;
}

bool PageTask::IsForceStopActivated(int taskId)
{
    return database::puresql::IsForceStopActivated(taskId, db).data;
}

PageTaskPtr PageTask::GetLastTask()
{
    auto id = database::puresql::GetLastExecutedTaskID(db);
    auto task = GetTaskById(id.data);
    return task;
}

int PageTask::GetLastTaskId()
{
    auto id = database::puresql::GetLastExecutedTaskID(db);
    return id.data;
}

TaskList PageTask::GetUnfinishedTasks()
{
    auto taskList = database::puresql::GetUnfinishedTasks(db);
    // todo: this needs error processing
    return taskList.data;
}

PageTaskPtr PageTask::GetTaskById(int id)
{

    auto taskResult = database::puresql::GetTaskData(id, db);
    auto result =  taskResult.data;
    auto subtaskResult = database::puresql::GetSubTaskData(id, db);
    bool valid = true;
    for(const auto& subtask: std::as_const(subtaskResult.data))
    {
        if(!subtask)
            continue;

        auto errors = database::puresql::GetErrorsForSubTask(subtask->parentId, db, subtask->id);
        subtask->errors = errors.data;

        auto actions = database::puresql::GetActionsForSubTask(subtask->parentId, db, subtask->id);
        subtask->executedActions = actions.data;

        if(!subtask->isValid)
            valid = false;

        result->subTasks.push_back(subtask);
        subtask->parent = result;
    }
    taskResult.data->isValid = valid;
    taskResult.data->isNew =false;
    taskResult.data->delay= An<PageManager>()->timeout;
    return result;
}

SubTaskErrors PageTask::GetErrorsForTask(int id, int subId, int cutoffLevel)
{
    SubTaskErrors result;
    auto errors = database::puresql::GetErrorsForSubTask(id, db, subId);
    for(const auto& part : std::as_const(errors.data))
    {
        if(!part || static_cast<int>(part->errorlevel) < cutoffLevel)
            continue;
        result.push_back(part);
    }
    return result;
}

void PageTask::SetCurrentTask(PageTaskPtr task)
{
    currentTask = task;
}

PageTaskPtr PageTask::GetCurrentTask()
{
    return currentTask;
}


}
