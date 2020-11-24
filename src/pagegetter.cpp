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
#include "include/pagegetter.h"
#include "include/transaction.h"
#include "GlobalHeaders/run_once.h"
#include "logger/QsLog.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QObject>
#include "sql_abstractions/sql_query.h"
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_error.h"
#include <QSqlRecord>
#include <QDebug>
#include <QSqlError>
#include <QEventLoop>
#include <QThread>
#include <QCoreApplication>
#include <QSettings>
#include <iostream>



class PageGetterPrivate : public QObject
{
Q_OBJECT
public:
    explicit PageGetterPrivate(QObject *parent=nullptr);
    QNetworkAccessManager manager;
    QNetworkRequest currentRequest;
    QNetworkRequest* currentReply= nullptr;
    QEventLoop waitLoop;
    WebPage result;
    bool cachedMode = false;
    QNetworkReply::NetworkError error;
    WebPage GetPage(QString url, ECacheMode useCache = ECacheMode::use_cache);
    WebPage GetPageFromDB(QString url);
    WebPage GetPageFromNetwork(QString url);
    void SavePageToDB(const WebPage&);
    void SetDatabase(sql::Database _db);
    void WipeOldCache();
    void WipeAllCache();
    QDate automaticCacheDateCutoff;
    bool autoCacheForCurrentDate = true;
    sql::Database db;

public slots:
    //void OnNetworkReply(QNetworkReply*);
};

PageGetterPrivate::PageGetterPrivate(QObject *parent): QObject(parent)
{
//    connect(&manager, SIGNAL (finished(QNetworkReply*)),
//            this, SLOT (OnNetworkReply(QNetworkReply*)));
}

WebPage PageGetterPrivate::GetPage(QString url, ECacheMode useCache)
{
    WebPage result;
    // first, we get the page from cache anyway
    // not much point doing otherwise if the page is super fresh
    auto temp = GetPageFromDB(url);
    if(temp.isValid)
        QLOG_INFO() << "Version from cache was generated: " << temp.generated;
    if(useCache != ECacheMode::dont_use_cache && autoCacheForCurrentDate && temp.generated.date() >= QDate::currentDate().addDays(-1))
    {
        result = temp;
        qDebug() << "pickign cache version";
        result.isFromCache = true;
        return result;
    }
    if(useCache == ECacheMode::use_cache || useCache == ECacheMode::use_only_cache)
    {
        result = GetPageFromDB(url);
        if(result.isValid)
        {
            if(useCache == ECacheMode::use_only_cache || result.generated > QDateTime::currentDateTime().addDays(-7))
                result.isFromCache = true;
        }
        else
        {
            result = GetPageFromNetwork(url);
            qDebug() << "From network";
            SavePageToDB(result);
        }
    }
    else
    {
        result = GetPageFromNetwork(url);
        qDebug() << "From network";
        if(result.isValid)
            SavePageToDB(result);
    }
    return result;
}

WebPage PageGetterPrivate::GetPageFromDB(QString url)
{
    WebPage result;
//    auto db = sql::Database::database("PageCache");
    bool dbOpen = db.isOpen();
    if(!dbOpen)
        return result;
    {
        // first we search for the exact page in the database
        sql::Query q(db);
        q.prepare("select * from PageCache where url = :URL ");
        q.bindValue(":URL", url);
        q.exec();
        bool dataFound = q.next();

        if(q.lastError().isValid())
        {
            qDebug() << "Reading url: " << url;
            qDebug() << "Error getting page from database: " << QString::fromStdString(q.lastError().text());
            return result;
        }

        if(!dataFound)
            return result;
        //qDebug() << q.record();
        result.url = url;
        result.isValid = true;
        if(q.value("COMPRESSED").toInt() == 1)
            result.content = QString::fromUtf8(qUncompress(q.value("CONTENT").toByteArray()));
        else
            result.content = q.value("CONTENT").toByteArray();
        //result.crossover= q.value("CROSSOVER").toInt();
        //result.fandom= q.value("FANDOM").toString();
        result.generated= q.value("GENERATION_DATE").toDateTime();
        result.source = EPageSource::cache;
        result.type = static_cast<EPageType>(q.value("PAGE_TYPE").toInt());
    }
    return result;
}

WebPage PageGetterPrivate::GetPageFromNetwork(QString url)
{
    result = WebPage();
    result.url = url;
    currentRequest = QNetworkRequest(QUrl(url));
    auto reply = manager.get(currentRequest);
    int retries = 40;
    //qDebug() << "entering wait phase";
    while(!reply->isFinished() && retries > 0)
    {
//        if(retries%10 == 0)
//            qDebug() << "retries left: " << retries;
        if(reply->isFinished())
            break;
        retries--;
        QThread::msleep(500);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    if(!reply->isFinished())
    {
        qDebug() << "failed to get the page in time";
        return result;
    }

    QByteArray data=reply->readAll();

    error = reply->error();
    reply->deleteLater();
    if(error != QNetworkReply::NoError)
        return result;
    //QString str(data);
    result.content = data;

    result.isValid = true;
    result.url = url;
    result.source = EPageSource::network;
    return result;
}

void PageGetterPrivate::SavePageToDB(const WebPage & page)
{
    QSettings settings("settings/settings.ini", QSettings::IniFormat);
    sql::Query q(db);
    q.prepare("delete from pagecache where url = :url");
    q.bindValue(":url", page.url);
    q.exec();
    QString insert = "INSERT INTO PAGECACHE(URL, GENERATION_DATE, CONTENT,  PAGE_TYPE, COMPRESSED) "
                     "VALUES(:URL, :GENERATION_DATE, :CONTENT, :PAGE_TYPE, :COMPRESSED)";
    q.prepare(insert.toStdString());
    q.bindValue(":URL", page.url);
    q.bindValue(":GENERATION_DATE", QDateTime::currentDateTime());
    q.bindValue(":CONTENT", qCompress(page.content.toUtf8()));
    q.bindValue(":COMPRESSED", 1);
    q.bindValue(":PAGE_TYPE", static_cast<int>(page.type));
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << "Writing url: " << page.url;
        qDebug() << "Error saving page to database: "  << QString::fromStdString(q.lastError().text());
    }
}

void PageGetterPrivate::SetDatabase(sql::Database _db)
{
    db  = _db;
}

void PageGetterPrivate::WipeOldCache()
{
    //" and updated > date('now', '-60 days') ";
    sql::Query q(sql::Database::database("PageCache"));
    q.prepare("delete from pagecache where generation_date < date('now', '-2 days')");
    q.exec();
    if(q.lastError().isValid())
        qDebug() << "Error wiping cache: "  << QString::fromStdString(q.lastError().text());
}

void PageGetterPrivate::WipeAllCache()
{
    sql::Query q(sql::Database::database("PageCache"));
    q.prepare("delete from pagecache");
    q.exec();
    if(q.lastError().isValid())
        qDebug() << "Error wiping cache: "  << QString::fromStdString(q.lastError().text());

    q.prepare("vacuum");
    q.exec();
    if(q.lastError().isValid())
        qDebug() << "Error wiping cache: "  << QString::fromStdString(q.lastError().text());
}

PageManager::PageManager() : d(new PageGetterPrivate)
{

}

PageManager::~PageManager()
{
    qDebug() << "deleting page manager";
}

void PageManager::SetDatabase(sql::Database _db)
{
    d->SetDatabase(_db);
}

void PageManager::SetCachedMode(bool value)
{
    d->cachedMode = value;
}

bool PageManager::GetCachedMode() const
{
    return d->cachedMode;
}

WebPage PageManager::GetPage(QString url, ECacheMode useCache)
{
    return d->GetPage(url, useCache);
}

void PageManager::SavePageToDB(const WebPage & page)
{
    d->SavePageToDB(page);
}

void PageManager::SetAutomaticCacheLimit(QDate date)
{
    d->automaticCacheDateCutoff = date;
}

void PageManager::SetAutomaticCacheForCurrentDate(bool value)
{
    d->autoCacheForCurrentDate = value;
}

void PageManager::WipeOldCache()
{
    QSettings settings("settings/settings.ini", QSettings::IniFormat);
    if(!settings.value("Settings/keepOldCache", false).toBool())
        d->WipeOldCache();
}

void PageManager::WipeAllCache()
{
    d->WipeAllCache();
}
#include "pagegetter.moc"

PageThreadWorker::PageThreadWorker(QObject*)
{
    startTimer(2000);
}

PageThreadWorker::~PageThreadWorker()
{
    qDebug() << "deleting pageThread worker";
}

void PageThreadWorker::timerEvent(QTimerEvent *)
{
    //qDebug() << "worker is alive";
}

void PageThreadWorker::Task(QString url,
                            QString lastUrl,
                            QDate updateLimit,
                            ECacheMode cacheMode,
                            bool ignoreUpdateDate,
                            int delay)
{
    FuncCleanup f([&](){working = false;});
    //qDebug() << updateLimit;
    database::Transaction pcTransaction(sql::Database::database("PageCache"));
    working = true;
    QScopedPointer<PageManager> pager(new PageManager);
    pager->WipeOldCache();
    pager->SetAutomaticCacheLimit(automaticCache);
    pager->SetAutomaticCacheForCurrentDate(automaticCacheForCurrentDate);
    WebPage result;
    int counter = 0;
    QString nextUrl = url;
    do
    {
        url = nextUrl;
        qDebug() << "loading page: " << url;
        auto startPageLoad = std::chrono::high_resolution_clock::now();
        result = pager->GetPage(url, cacheMode);
        result.pageIndex = counter+1;
        auto minUpdate = GrabMinUpdate(result.content);

        bool updateLimitReached = false;
        if(counter > 0)
            updateLimitReached = minUpdate < updateLimit;
        if((url == lastUrl || lastUrl.trimmed().isEmpty()) || (!ignoreUpdateDate && updateLimit.isValid() && updateLimitReached))
        {
            result.error = "Already have the stuff past this point. Aborting.";
            result.isLastPage = true;
        }
        if(!result.isValid || url.isEmpty())
            result.isLastPage = true;
        emit pageResult({result, false});
        auto elapsed = std::chrono::high_resolution_clock::now() - startPageLoad;

        result.loadedIn = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        if(!result.isFromCache)
        {
            //qDebug() << "thread will sleep for " << timeout;
            QThread::msleep(delay);
        }
        nextUrl = GetNext(result.content);
        counter++;
    }while(url != lastUrl && result.isValid && !result.isLastPage);
    emit pageResult({WebPage(), true});
    pcTransaction.finalize();
    qDebug() << "leaving task1";
}


void PageThreadWorker::ProcessBunchOfFandomUrls(QStringList urls,
                                                QDate stopAt,
                                                ECacheMode cacheMode,
                                                QStringList& failedPages,
                                                int delay)
{
    WebPage result;
    QScopedPointer<PageManager> pager(new PageManager);
    pager->SetAutomaticCacheLimit(automaticCache);
    pager->SetAutomaticCacheForCurrentDate(automaticCacheForCurrentDate);
    int counter = 0;
    for (const auto& url : urls)
    {
        qDebug() << QStringLiteral("loading page: ") << url;
        auto startPageLoad = std::chrono::high_resolution_clock::now();
        result = pager->GetPage(url, cacheMode);
        result.pageIndex = counter+1;
        auto minUpdate = GrabMinUpdate(result.content);

        bool updateLimitReached = false;
        // we ALWAYS get at least one page
        if(counter > 1)
            updateLimitReached = minUpdate < stopAt;
        if(stopAt.isValid() && updateLimitReached)
        {
            result.comment = "Already have the stuff past this point. Aborting.";
            result.isLastPage = true;
        }
        if(!result.isValid)
        {
            failedPages.push_back(result.url);
            continue;
        }
        result.minFicDate = minUpdate;
        emit pageResult({result, false});
        auto elapsed = std::chrono::high_resolution_clock::now() - startPageLoad;

        result.loadedIn = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        if(updateLimitReached)
            break;
        if(!result.isFromCache)
            QThread::msleep(delay);
        counter++;
    }
}

void PageThreadWorker::FandomTask(const FandomParseTask& task)
{
    FuncCleanup f([&](){working = false;});
    //qDebug() << updateLimit;
    database::Transaction pcTransaction(sql::Database::database("PageCache"));
    working = true;
    QScopedPointer<PageManager> pager(new PageManager);
    pager->WipeOldCache();
    WebPage result;
    QStringList failedPages;
    ProcessBunchOfFandomUrls(task.parts,task.stopAt, task.cacheMode, failedPages, task.delay);
    QStringList voidPages;
    qDebug() << QStringLiteral("reacquiring urls: ") << failedPages;
    ProcessBunchOfFandomUrls(failedPages,task.stopAt, task.cacheMode, voidPages, task.delay);
    for(const auto& page : std::as_const(voidPages))
    {
        WebPage failedPage;
        failedPage.failedToAcquire = true;
        failedPage.isValid = false;
        failedPage.url = page;
        emit pageResult({result, false});
    }
    emit pageResult({WebPage(), true});
    pcTransaction.finalize();
    qDebug() << "leaving fandom task";
}

void PageThreadWorker::TaskList(QStringList urls, ECacheMode cacheMode,  int delay)
{
    FuncCleanup f([&](){working = false;});
    // kinda have to split pagecache db from service db I guess
    // which is only natural anyway... probably
    // still not helping for multithreading later on
    auto db = sql::Database::database("PageCache");
    database::Transaction pcTransaction(db);
    working = true;
    QScopedPointer<PageManager> pager(new PageManager);
    pager->WipeOldCache();
    pager->SetAutomaticCacheLimit(automaticCache);
    pager->SetAutomaticCacheForCurrentDate(automaticCacheForCurrentDate);
    WebPage result;
    qDebug() << "loading task: " << urls;
    for(int i=0; i< urls.size();  i++)
    {
        auto startPageLoad = std::chrono::high_resolution_clock::now();
        result = pager->GetPage(urls[i], cacheMode);
        if(!result.isValid)
            continue;
        if(!result.isFromCache)
            pager->SavePageToDB(result);

        if(i == urls.size()-1)
            result.isLastPage = true;
        auto elapsed = std::chrono::high_resolution_clock::now() - startPageLoad;
        result.loadedIn = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        result.pageIndex = i;
        //qDebug() << "emitting page:" << i;
        emit pageResult({result, false});
        if(!result.isFromCache)
        {
            //qDebug() << "thread will sleep for " << timeout;
            QThread::msleep(delay);
        }
    }
    emit pageResult({WebPage(), true});
    pcTransaction.finalize();
    qDebug() << "leaving task2";
}
static QString CreateURL(QString str)
{
    return QStringLiteral("https://www.fanfiction.net/") + str;
}

QString PageThreadWorker::GetNext(QString text)
{
    QString nextUrl;
    thread_local QRegExp rxEnd(QRegExp::escape(QStringLiteral("Next &#187")));
    int indexEnd = rxEnd.indexIn(text);
    if(indexEnd != -1)
        indexEnd-=2;
    int posHref = indexEnd - 400 + text.midRef(indexEnd - 400,400).lastIndexOf("href='");
    nextUrl = CreateURL(text.mid(posHref+6, indexEnd - (posHref+6)));
    if(!nextUrl.contains(QStringLiteral("&p=")))
        nextUrl = QStringLiteral("");
    //indexEnd = rxEnd.indexIn(text);
    return nextUrl;
}

static QDate GetRealMinDate(QList<QDate> dates)
{
    if(dates.size() == 0)
        return QDate();
    std::sort(std::begin(dates), std::end(dates));
    QDate medianDate = dates[dates.size()/2];
    //QHash<QDate, int> distances;
    int counter = 0;
    int totalDistance = 0;
    for(auto date : std::as_const(dates))
    {
        int distance = std::abs(date.daysTo(medianDate));
        totalDistance+=distance;
        counter++;
    }
    if(counter == 0)
        return QDateTime::currentDateTimeUtc().date();

    double average = static_cast<double>(totalDistance)/static_cast<double>(counter);

    QDate minDate = medianDate;
    for(auto date : std::as_const(dates))
    {
        if(date < minDate && std::abs(date.daysTo(medianDate)) <= average)
            minDate = date;
    }
    return minDate;
}


QDate PageThreadWorker::GrabMinUpdate(QString text)
{
    QList<QDate> dates;
    QDateTime minDate;
    QDate result;
    thread_local QRegExp rx("Updated:\\s<span\\sdata-xutime='(\\d+)'");
    int startFrom = 0;
    int indexStart = -1;
    do{
        indexStart = rx.indexIn(text, startFrom);
        if(indexStart != 1 && !rx.cap(1).trimmed().replace(QStringLiteral("-"),QStringLiteral("")).isEmpty())
        {
            minDate.setTime_t(rx.cap(1).toInt());
            dates.push_back(minDate.date());
        }

        startFrom = indexStart+2;
    }while(indexStart != -1);
    result = GetRealMinDate(dates);
    return result;
}

void PageThreadWorker::SetAutomaticCache(QDate date)
{
    automaticCache = date;
}

void PageThreadWorker::SetAutomaticCacheForCurrentDate(bool value)
{
    automaticCacheForCurrentDate = value;
}

