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
    bool IsForceStopActivated(int);
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
