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

#include "include/web/pageconsumer.h"
#include "include/flipper_client_logic.h"
#include "include/core/section.h"
#include "include/web/cache_strategy.h"

namespace interfaces{
class Fanfics;
class Fandoms;
class Authors;
class PageTask;
}

class PageTask;
typedef QSharedPointer<PageTask> PageTaskPtr;


class AuthorStatsProcessor: public PageConsumer{
Q_OBJECT
public:
    AuthorStatsProcessor(sql::Database db,
                        QSharedPointer<interfaces::Fanfics> fanficInterface,
                        QSharedPointer<interfaces::Fandoms> fandomsInterface,
                        QSharedPointer<interfaces::Authors> authorsInterface,
                        QObject* obj = nullptr);

    void ReprocessAllAuthorsStats(fetching::CacheStrategy cacheStrategy = {});


private:
    sql::Database db;
    QSharedPointer<interfaces::Fanfics> fanficsInterface;
    QSharedPointer<interfaces::Fandoms> fandomsInterface;
    QSharedPointer<interfaces::Authors> authorsInterface;
    QSharedPointer<interfaces::PageTask> pageInterface;
    sql::Database dbInterface;
signals:
    void requestProgressbar(int);
    void updateCounter(int);
    void updateInfo(QString);
};
