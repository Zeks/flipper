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
#include <QRegularExpression>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QUuid>
#include <QProcess>
#include <QSettings>
#include <QTextStream>
#include <QObject>
#include <QCoreApplication>
#include <QSqlRecord>
#include "sql_abstractions/sql_query.h"
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_error.h"
#include <QDebug>

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
        qDebug() << "From network";
        if(result.isValid)
            SavePageToDB(result);
    };
    if(useCache != ECacheMode::dont_use_cache)
    {
        result = GetPageFromDB(url);
        QLOG_INFO() << "Version from cache was generated: " << result.generated;
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
    q.prepare("select * from PageCache where url = :URL ");
    q.bindValue("URL", url.toStdString());
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
        result.content = QString::fromUtf8(qUncompress(q.value("CONTENT").toByteArray()));
    else
        result.content = q.value("CONTENT").toByteArray();
    //result.crossover= q.value("CROSSOVER").toInt();
    //result.fandom= q.value("FANDOM").toString();
    result.generated= q.value("GENERATION_DATE").toDateTime();
    result.source = EPageSource::cache;
    result.type = static_cast<EPageType>(q.value("PAGE_TYPE").toInt());

    return result;
}

WebPage PageGetterPrivate::GetPageFromNetwork(QString url)
{
    result = WebPage();
    result.url = url;
    QFile file("scripts/flare_post.js");
    QString curlQuery;
    if (file.open(QFile::ReadOnly))
    {
        QTextStream in(&file);
        curlQuery = in.readAll();
    }

    QString id = QUuid::createUuid().toString();
    QStringList params;
    QProcess process;
    QSettings settings("settings/settings_solver.ini", QSettings::IniFormat);
    QString servitorPort = settings.value("Settings/flarePort").toString();
    QRegularExpression rxNum("[0-9]{1,15}");
    auto match = rxNum.match(url);
    QString userPart;
    if(match.hasMatch())
        userPart=match.captured(0);
    else
        userPart=QUuid::createUuid().toString();

    QString filename = QString("tmpfaves/favourites_%1.html").arg(userPart);
    params << "-c" << "curl" << "-L" << "-X" << "POST" << QString("http://%1/v1").arg(servitorPort)
           << "-H" << "'Content-Type: application/json'"
           << "--data-raw" << curlQuery.arg(url) << ">" << filename;
    process.start("curl" , params);
    process.waitForFinished(-1); // will wait forever until finished
    QString stdoutResult = process.readAllStandardOutput();
    QString stderrResult = process.readAllStandardError();

    QFile tempFIle(filename);
    if(tempFIle.open(QFile::WriteOnly | QFile::Truncate))
    {
        QTextStream out(&tempFIle);
        out << stdoutResult;
    }
    file.close();

    process.start("bash", QStringList() << "-c" << "scripts/page_fixer.sh " + QString("%1").arg(filename));
    process.waitForFinished(-1); // will wait forever until finished


    stdoutResult = process.readAllStandardOutput();
    stderrResult = process.readAllStandardError();
    qDebug() << stdoutResult;
    qDebug() << stderrResult;

    QThread::msleep(500);
    QFile favouritesfile(filename);
    QString favourites;
    if (favouritesfile.open(QFile::ReadOnly))
    {
        QTextStream in(&favouritesfile);
        result.content = in.readAll();
        result.isValid = true;
        result.url = url;
        result.source = EPageSource::network;
    }
    else{
        result.isValid = false;
        result.url = url;
        result.source = EPageSource::network;
    }
    return result;
}

void PageGetterPrivate::SavePageToDB(const WebPage & page)
{
    auto dbToken = dbGetter();
    sql::Query q(dbToken->db);
    q.prepare("delete from pagecache where url = :url");
    q.bindValue("url", page.url);
    q.exec();
    auto insert = "INSERT INTO PAGECACHE(URL, GENERATION_DATE, CONTENT,  PAGE_TYPE, COMPRESSED) "
                     "VALUES(:URL, :GENERATION_DATE, :CONTENT, :PAGE_TYPE, :COMPRESSED)";
    q.prepare(insert);
    q.bindValue("URL", page.url);
    q.bindValue("GENERATION_DATE", QDateTime::currentDateTime());
    q.bindValue("CONTENT", qCompress(page.content.toUtf8()));
    q.bindValue("COMPRESSED", 1);
    q.bindValue("PAGE_TYPE", static_cast<int>(page.type));
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << "Writing url: " << page.url;
        qDebug() << "Error saving page to database: "  << q.lastError().text();
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

