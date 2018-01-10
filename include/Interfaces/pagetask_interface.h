#pragma once
#include <QString>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSharedPointer>
#include <QUuid>
#include "pagetask.h"
class PageSubTask;
class PageTask;

namespace interfaces {
class PageTask{
public:
    void WriteTaskIntoDB(PageTaskPtr);
    void WriteSubTaskIntoDB(SubTaskPtr);

    bool DropLastTask();
    bool DropTaskId(int id);
    bool IsLastTaskSuccessful();
    PageTaskPtr GetLastTask();
    int GetLastTaskId();
    TaskList GetUnfinishedTasks();
    PageTaskPtr GetTaskById(int id);
    SubTaskErrors GetErrorsForTask(int id, int subId = -1, int cutoffLevel = 1);

    void SetCurrentTask(PageTaskPtr);
    PageTaskPtr GetCurrentTask();

    PageTaskPtr currentTask;
    QSqlDatabase db;
};



}
