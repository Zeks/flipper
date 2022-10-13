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
#pragma once
#include <QObject>
#include <QDate>
#include <QSharedPointer>

#include "include/pageconsumer.h"
#include "include/core/section.h"
#include "include/core/fandom.h"
#include "include/web/cache_strategy.h"

struct FandomParseTask{
    FandomParseTask() = default;
    FandomParseTask(const QStringList& parts,
                    QDate stopAt,
                    fetching::CacheStrategy cacheStrategy,
                    int delay,
                    int pageRetries = 3){
        this->stopAt = stopAt;
        this->cacheStrategy = cacheStrategy;
        this->pageRetries = pageRetries;
        this->delay = delay;
        this->parts = parts;
    }
    QString fandom;
    QStringList parts;
    int pageRetries = 3;
    int delay = 500;
    QDate stopAt;
    fetching::CacheStrategy cacheStrategy;

};


struct FandomParseTaskResult
{
    FandomParseTaskResult(){}
    bool finished = false;
    bool failedToAcquirePages = false;
    bool criticalErrors = false;
    int parsedPages = 0;
    int addedFics = 0;
    int skippedFics = 0;
    int addedAuthors= 0;
    int updatedFics= 0;
    int updatedAuthors= 0;
    QStringList failedParts;
};

namespace interfaces{
class Fanfics;
class Fandoms;
class PageTask;
}

class PageTask;
typedef QSharedPointer<PageTask> PageTaskPtr;

struct ForcedFandomUpdateDate{
    bool isValid = false;
    QDate date;
};
class FandomLoadProcessor: public PageConsumer{
Q_OBJECT
public:
    FandomLoadProcessor(sql::Database db,
                        QSharedPointer<interfaces::Fanfics> fanficInterface,
                        QSharedPointer<interfaces::Fandoms> fandomsInterface,
                        QSharedPointer<interfaces::PageTask> pageInterface,
                        QObject* obj = nullptr);
    FandomParseTaskResult Run(FandomParseTask task);
    void Run(PageTaskPtr task);
    PageTaskPtr CreatePageTaskFromFandoms(QList<core::FandomPtr> fandoms,
                                          QString prototype,
                                                      QString taskComment,
                                                      fetching::CacheStrategy cacheStrategy,
                                                      bool allowCacheRefresh,
                                                      ForcedFandomUpdateDate forcedDate = ForcedFandomUpdateDate());

private:
    FandomParseTask task;
    sql::Database db;
    QSharedPointer<interfaces::Fanfics> fanficsInterface;
    QSharedPointer<interfaces::Fandoms> fandomsInterface;
    QSharedPointer<interfaces::PageTask> pageInterface;
signals:
    void taskStarted(FandomParseTask);
    void requestProgressbar(int);
    void updateCounter(int);
    void updateInfo(QString);
};
