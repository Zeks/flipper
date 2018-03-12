/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
#include <QSqlDatabase>
#include "GlobalHeaders/SingletonHolder.h"
#include "ECacheMode.h"
#include <atomic>


enum class EPageType
{
    hub_page = 0,
    sorted_ficlist = 1,
    author_profile = 2,
    fic_page = 3
};

enum class EPageSource
{
    none = -1,
    network = 0,
    cache = 1,
};

struct FandomParseTask{
    FandomParseTask() = default;
    FandomParseTask(QStringList parts,
                    QDate stopAt,
                    ECacheMode cacheMode,
                    int pageRetries = 3){
        this->stopAt = stopAt;
        this->cacheMode = cacheMode;
        this->pageRetries = pageRetries;
        this->parts = parts;
    }
    QString fandom;
    QStringList parts;
    int pageRetries = 3;
    QDate stopAt;
    ECacheMode cacheMode = ECacheMode::dont_use_cache;

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

class PageThreadWorker;
struct WebPage
{
    friend class PageThreadWorker;
    QString url;
    QDateTime generated;
    QDate minFicDate;
    //QString stringContent;
    QString content;
    QString previousUrl;
    QString nextUrl;
    QStringList referencedFics;
    QString fandom;
    bool crossover;
    EPageType type;
    bool isValid = false;
    bool failedToAcquire = false;
    EPageSource source = EPageSource::none;
    QString error;
    QString comment;
    bool isLastPage = false;
    bool isFromCache = false;
    int pageIndex = 0;
    int id = -1;
    QString LoadedIn() {
        QString decimal = QString::number(loadedIn/1000000);
        int offset = decimal == "0" ? 0 : decimal.length();
        QString partial = QString::number(loadedIn).mid(offset,1);
        return decimal + "." + partial;}
private:
    int loadedIn = 0;
};

struct PageQueue{
    bool pending = true;
    QList<WebPage> data;
};

class  PageResult{
public:
    PageResult() = default;
    PageResult(WebPage page, bool _finished): finished(_finished),data(page){}
    bool finished = false;
    WebPage data;
};


class PageGetterPrivate;
class PageManager
{
    public:
    PageManager();
    ~PageManager();
    void SetDatabase(QSqlDatabase _db);
    void SetCachedMode(bool value);
    bool GetCachedMode() const;
    WebPage GetPage(QString url, ECacheMode useCache = ECacheMode::dont_use_cache);
    void SavePageToDB(const WebPage & page);
    void SetAutomaticCacheLimit(QDate);
    void SetAutomaticCacheForCurrentDate(bool);
    void WipeOldCache();
    void WipeAllCache();
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
    int timeout = 500;
    std::atomic<bool> working;
    QDate automaticCache;
    bool automaticCacheForCurrentDate = true;
public slots:
    void Task(QString url, QString lastUrl, QDate updateLimit, ECacheMode cacheMode, bool ignoreUpdateDate);
    void FandomTask(FandomParseTask);
    void ProcessBunchOfFandomUrls(QStringList urls,
                                  QDate stopAt,
                                  ECacheMode cacheMode,
                                  QStringList& failedPages);
    void TaskList(QStringList urls, ECacheMode cacheMode);
signals:
    void pageResult(PageResult);
};


BIND_TO_SELF_SINGLE(PageManager);
