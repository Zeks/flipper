#include "Interfaces/pagetask_interface.h"
#include "Interfaces/db_interface.h"
#include "include/pure_sql.h"
#include "GlobalHeaders/run_once.h"
#include "include/transaction.h"

namespace database{

void PageTaskDBInterface::WriteTaskIntoDB(PageTaskPtr task)
{
    if(!task || !task->isValid)
        return;
    if(task->NeedsInsertion())
        database::puresql::CreateTaskInDB(task, db);
    else
        database::puresql::UpdateTaskInDB(task, db);

    for(auto subtask : task->subTasks)
    {
        if(!subtask || !subtask->isValid)
            continue;
        if(subtask->NeedsInsertion())
            database::puresql::CreateSubTaskInDB(subtask, db);
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
}

bool PageTaskDBInterface::IsLastTaskSuccessful()
{
    auto id = database::puresql::GetLastExecutedTaskID(db);
    bool success = database::puresql::GetTaskSuccessByID(id, db);
    return success;
}

PageTaskPtr PageTaskDBInterface::GetLastTask()
{
    auto id = database::puresql::GetLastExecutedTaskID(db);
    auto task = GetTaskById(id);
    return task;
}

PageTaskPtr PageTaskDBInterface::GetTaskById(int id)
{
    auto taskResult = database::puresql::GetTaskData(id, db);
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
    }
}

SubTaskErrors PageTaskDBInterface::GetErrorsForTask(int id, int subId, int cutoffLevel)
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

void PageTaskDBInterface::SetCurrent(PageTaskPtr task)
{
    currentTask = task;
}


}
