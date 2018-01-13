#include "Interfaces/pagetask_interface.h"
#include "Interfaces/db_interface.h"
#include "include/pure_sql.h"
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

    for(auto subtask : task->subTasks)
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

    for(auto action : subtask->executedActions)
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
    bool success = database::puresql::GetTaskSuccessByID(id.data, db);
    return success;
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
    for(auto subtask: subtaskResult.data)
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
    return result;
}

SubTaskErrors PageTask::GetErrorsForTask(int id, int subId, int cutoffLevel)
{
    SubTaskErrors result;
    auto errors = database::puresql::GetErrorsForSubTask(id, db, subId);
    for(auto part : errors.data)
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
