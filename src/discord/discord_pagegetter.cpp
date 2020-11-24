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
#include "discord/discord_pagegetter.h"
#include "discord/db_vendor.h"
#include "include/transaction.h"
#include "GlobalHeaders/run_once.h"
#include "logger/QsLog.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QObject>
#include <QCoreApplication>
#include "sql_abstractions/sql_query.h"
#include <QSqlRecord>
#include "sql_abstractions/sql_database.h"
#include <QDebug>
#include "sql_abstractions/sql_database.h"
#include <QSqlError>
namespace discord {

class PageGetterPrivate
{
public:
    explicit PageGetterPrivate();
    QNetworkAccessManager manager;
    QNetworkRequest currentRequest;
    QNetworkRequest* currentReply= nullptr;
    WebPage result;
    int timeout = 500;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    WebPage GetPage(QString url, ECacheMode useCache = ECacheMode::use_cache);
    WebPage GetPageFromDB(QString url);
    WebPage GetPageFromNetwork(QString url);
    void SavePageToDB(const WebPage&);
    void SetDatabaseGetter(PageManager::DBGetterFunc _dbGetter);
    PageManager::DBGetterFunc dbGetter;
};

PageGetterPrivate::PageGetterPrivate()
{
}

WebPage PageGetterPrivate::GetPage(QString url, ECacheMode useCache)
{
    WebPage result;

    auto fetchPageFromNetwork = [&](){
        result = GetPageFromNetwork(url);
        qDebug() << QStringLiteral("From network");
        if(result.isValid)
            SavePageToDB(result);
    };
    if(useCache != ECacheMode::dont_use_cache)
    {
        result = GetPageFromDB(url);
        QLOG_INFO() << QStringLiteral("Version from cache was generated: ") << result.generated;
        if(result.isValid)
             result.isFromCache = true;
        else
            fetchPageFromNetwork();
    }
    else
        fetchPageFromNetwork();
    return result;
}

WebPage PageGetterPrivate::GetPageFromDB(QString url)
{
    WebPage result;

    bool dbOpen = false;
    auto dbToken = dbGetter();
    dbOpen = dbToken->db.isOpen();

    if(!dbOpen)
        return result;

    // first we search for the exact page in the database
    sql::Query q(dbToken->db);
    q.prepare(QStringLiteral("select * from PageCache where url = :URL "));
    q.bindValue(QStringLiteral(":URL"), url);
    q.exec();
    bool dataFound = q.next();

    if(q.lastError().isValid())
    {
        qDebug() << "Reading url: " << url;
        qDebug() << "Error getting page from database: " << q.lastError().text();
        return result;
    }

    if(!dataFound)
        return result;
    //qDebug() << q.record();
    result.url = url;
    result.isValid = true;
    if(q.value("COMPRESSED").toInt() == 1)
        result.content = QString::fromUtf8(qUncompress(q.value(QStringLiteral("CONTENT")).toByteArray()));
    else
        result.content = q.value(QStringLiteral("CONTENT")).toByteArray();
    //result.crossover= q.value("CROSSOVER").toInt();
    //result.fandom= q.value("FANDOM").toString();
    result.generated= q.value(QStringLiteral("GENERATION_DATE")).toDateTime();
    result.source = EPageSource::cache;
    result.type = static_cast<EPageType>(q.value(QStringLiteral("PAGE_TYPE")).toInt());

    return result;
}

WebPage PageGetterPrivate::GetPageFromNetwork(QString url)
{
    result = WebPage();
    result.url = url;
    currentRequest = QNetworkRequest(QUrl(url));
    auto reply = manager.get(currentRequest);
    static const int maxNumberOfRetries = 40;
    int retries = maxNumberOfRetries;
    //qDebug() << "entering wait phase";
    while(!reply->isFinished() && retries > 0)
    {
//        if(retries%10 == 0)
//            qDebug() << "retries left: " << retries;
        if(reply->isFinished())
            break;
        retries--;
        QThread::msleep(timeout);
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
    auto dbToken = dbGetter();
    sql::Query q(dbToken->db);
    q.prepare(QStringLiteral("delete from pagecache where url = :url"));
    q.bindValue(QStringLiteral(":url"), page.url);
    q.exec();
    QString insert = QStringLiteral("INSERT INTO PAGECACHE(URL, GENERATION_DATE, CONTENT,  PAGE_TYPE, COMPRESSED) "
                     "VALUES(:URL, :GENERATION_DATE, :CONTENT, :PAGE_TYPE, :COMPRESSED)");
    q.prepare(insert);
    q.bindValue(QStringLiteral(":URL"), page.url);
    q.bindValue(QStringLiteral(":GENERATION_DATE"), QDateTime::currentDateTime());
    q.bindValue(QStringLiteral(":CONTENT"), qCompress(page.content.toUtf8()));
    q.bindValue(QStringLiteral(":COMPRESSED"), 1);
    q.bindValue(QStringLiteral(":PAGE_TYPE"), static_cast<int>(page.type));
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << QStringLiteral("Writing url: ") << page.url;
        qDebug() << QStringLiteral("Error saving page to database: ")  << q.lastError().text();
    }
}

void PageGetterPrivate::SetDatabaseGetter(PageManager::DBGetterFunc _db)
{
    dbGetter  = _db;
}

PageManager::PageManager() : d(new PageGetterPrivate)
{

}

PageManager::~PageManager()
{
    qDebug() << "deleting page manager";
}

void PageManager::SetDatabaseGetter(DBGetterFunc _db)
{
    d->SetDatabaseGetter(_db);
}


WebPage PageManager::GetPage(QString url, ECacheMode useCache)
{
    return d->GetPage(url, useCache);
}

void PageManager::SavePageToDB(const WebPage & page)
{
    d->SavePageToDB(page);
}

}

