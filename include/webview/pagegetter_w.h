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
#include <QDateTime>
#include <QByteArray>
#include <QScopedPointer>
#include "sql_abstractions/sql_database.h"
#include <QThread>
#include "GlobalHeaders/SingletonHolder.h"
#include "include/tasks/fandom_task_processor.h"
#include "include/web/webpage.h"
//
#include "include/web/cache_strategy.h"
#include <atomic>

namespace webview{
class PageThreadWorker;
class PageGetterPrivate;
class PageManager
{
    public:
    PageManager();
    ~PageManager();
    void SetDatabase(sql::Database _db);
    void SetCachedMode(bool value);
    bool GetCachedMode() const;
    WebPage GetPage(QString url, fetching::CacheStrategy cacheStrategy);
    void SavePageToDB(const WebPage & page);
    void SetAutomaticCacheLimit(QDate);
    void SetAutomaticCacheForCurrentDate(bool);
    void WipeOldCache();
    void WipeAllCache();
    int timeout = 500;
    QScopedPointer<PageGetterPrivate> d;
};

class PageThreadWorker: public QObject
{
    Q_OBJECT
public:
    PageThreadWorker(QObject* parent = nullptr);
    ~PageThreadWorker();
    virtual void timerEvent(QTimerEvent *);
    QString GetNext(QString);
    QDate GrabMinUpdate(QString text);
    void SetAutomaticCache(QDate);
    void SetAutomaticCacheForCurrentDate(bool);

    std::atomic<bool> working = false;
    QDate automaticCache;
    bool automaticCacheForCurrentDate = true;
public slots:
    void Task(QString url, QString lastUrl, QDate updateLimit, fetching::CacheStrategy cacheStrategy, bool ignoreUpdateDate, int delay);
    void FandomTask(const FandomParseTask &);
    void ProcessBunchOfFandomUrls(QStringList urls,
                                  QDate stopAt,
                                  fetching::CacheStrategy cacheStrategy,
                                  QStringList& failedPages,
                                  int delay);
    void TaskList(QStringList urls, fetching::CacheStrategy cacheStrategy, int delay);
signals:
    void pageResult(PageResult);
};
}
BIND_TO_SELF_SINGLE(webview::PageManager);


