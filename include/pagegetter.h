/*
FFSSE is a replacement search engine for fanfiction.net search results
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

struct WebPage
{
    QString url;
    QDateTime generated;
    //QString stringContent;
    QString content;
    QString previousUrl;
    QString nextUrl;
    QStringList referencedFics;
    QString fandom;
    bool crossover;
    EPageType type;
    bool isValid = false;
    EPageSource source = EPageSource::none;
    QString error;
    bool isLastPage = false;
    bool isFromCache = false;
    int loadedIn = 0;
    int pageIndex = 0;
};

struct PageQueue{
    bool pending = true;
    QList<WebPage> data;
};

class  PageResult{
public:
    PageResult(WebPage page, bool _finished):data(page), finished(_finished){}
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
    int timeout = 300;
    std::atomic<bool> working;
public slots:
    void Task(QString url, QString lastUrl, QDate updateLimit, ECacheMode cacheMode);
    void TaskList(QStringList urls, ECacheMode cacheMode);
signals:
    void pageResult(PageResult);
};


BIND_TO_SELF_SINGLE(PageManager);
