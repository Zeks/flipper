/*Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

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
#include "include/web/pagegetter.h"
#include "GlobalHeaders/run_once.h"
#include "include/transaction.h"

namespace interfaces {

void PageTask::WriteTaskIntoDB(PageTaskPtr task)
{
    if(!task || !task->isValid)
        return;
    if(task->NeedsInsertion())
    {
        auto result = sql::CreateTaskInDB(task, db);
        task->id = result.data;
    }
    else
        sql::UpdateTaskInDB(task, db);
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
        auto result = sql::CreateSubTaskInDB(subtask, db);
        subtask->isNew = false;
    }
    else
        sql::UpdateSubTaskInDB(subtask, db);

    for(const auto& action : std::as_const(subtask->executedActions))
    {
        if(!action || !action->isNewAction)
            continue;

        sql::CreateActionInDB(action, db);
        sql::CreateErrorsInDB(action->errors, db);
    }
}

bool PageTask::DropLastTask()
{
    auto id = GetLastTaskId();
    auto result = sql::SetTaskFinished(id, db);
    return true;
}

bool PageTask::DropTaskId(int id)
{
    auto result = sql::SetTaskFinished(id, db);
    return true;
}

bool PageTask::IsLastTaskSuccessful()
{
    auto id = sql::GetLastExecutedTaskID(db);
    bool success = sql::GetTaskSuccessByID(id.data, db).data;
    return success;
}

bool PageTask::IsForceStopActivated(int taskId)
{
    return sql::IsForceStopActivated(taskId, db).data;
}

PageTaskPtr PageTask::GetLastTask()
{
    auto id = sql::GetLastExecutedTaskID(db);
    auto task = GetTaskById(id.data);
    return task;
}

int PageTask::GetLastTaskId()
{
    auto id = sql::GetLastExecutedTaskID(db);
    return id.data;
}

TaskList PageTask::GetUnfinishedTasks()
{
    auto taskList = sql::GetUnfinishedTasks(db);
    // todo: this needs error processing
    return taskList.data;
}

PageTaskPtr PageTask::GetTaskById(int id)
{

    auto taskResult = sql::GetTaskData(id, db);
    auto result =  taskResult.data;
    auto subtaskResult = sql::GetSubTaskData(id, db);
    bool valid = true;
    for(const auto& subtask: std::as_const(subtaskResult.data))
    {
        if(!subtask)
            continue;

        auto errors = sql::GetErrorsForSubTask(subtask->parentId, db, subtask->id);
        subtask->errors = errors.data;

        auto actions = sql::GetActionsForSubTask(subtask->parentId, db, subtask->id);
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
    auto errors = sql::GetErrorsForSubTask(id, db, subId);
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
