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
