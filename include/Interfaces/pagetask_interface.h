#pragma once
#include <QString>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSharedPointer>
#include <QUuid>
#include "pagetask.h"
namespace database{
class PageSubtask;
class PageTask;

class PageTaskDBInterface{
public:
    void WriteTaskIntoDB(PageTaskPtr);

    bool IsLastTaskSuccessful();
    PageTaskPtr GetLastTask();
    PageTaskPtr GetTaskById(int id);
    SubTaskErrors GetErrorsForTask(int id, int subId = -1, int cutoffLevel = 1);

    void SetCurrent(PageTaskPtr);

    PageTaskPtr currentTask;
    QSqlDatabase db;
};



}
