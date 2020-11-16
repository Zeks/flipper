/*
Flipper is a recommendation and search engine for fanfiction.net
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
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#pragma once
#include <QString>
#include <QVector>
#include "include/pagetask.h"
namespace interfaces{
class PageTask;
}
namespace page_utils{
struct SplitPart
{
    QString data;
    int partId;
};

struct SplitJobs
{
    QVector<SplitPart> parts;
    int favouriteStoryCountInWhole;
    int authorStoryCountInWhole;
    QString authorName;
};

SplitJobs SplitJob(QString data, bool splitOnThreads = true);

// creates the task and subtasks to load more authors from urls found in the database
PageTaskPtr CreatePageTaskFromUrls(QSharedPointer<interfaces::PageTask>,
                                   QDateTime currentDateTime,
                                   QStringList urls,
                                   QString taskComment,
                                   int subTaskSize,
                                   int subTaskRetries,
                                   ECacheMode cacheMode,
                                   bool allowCacheRefresh);
}
