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
#include "include/pagegetter.h"
#include "include/transaction.h"
#include "GlobalHeaders/run_once.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QObject>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlDatabase>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QEventLoop>
#include <QThread>



class PageGetterPrivate : public QObject
{
Q_OBJECT
public:
    PageGetterPrivate(QObject *parent=nullptr);
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
    void SetDatabase(QSqlDatabase _db);
    QSqlDatabase db;

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
    if(useCache == ECacheMode::use_cache || useCache == ECacheMode::use_only_cache)
    {
        result = GetPageFromDB(url);
        if(result.isValid)
        {
            result.isFromCache = true;
        }
        else if(useCache == ECacheMode::use_cache)
        {
            result = GetPageFromNetwork(url);
            SavePageToDB(result);
        }
    }
    else
    {
        result = GetPageFromNetwork(url);
        SavePageToDB(result);
    }
    return result;
}

WebPage PageGetterPrivate::GetPageFromDB(QString url)
{
    WebPage result;
    auto db = QSqlDatabase::database("Service");
    bool dbOpen = db.isOpen();
    QSqlQuery q(db);
    q.prepare("select * from PageCache where url = :URL");
    q.bindValue(":URL", url);
    q.exec();
    bool dataFound = q.next();

    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
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
    result.crossover= q.value("CROSSOVER").toInt();
    result.fandom= q.value("FANDOM").toString();
    result.generated= q.value("GENERATION_DATE").toDateTime();
    result.type = static_cast<EPageType>(q.value("PAGE_TYPE").toInt());
    return result;
}

WebPage PageGetterPrivate::GetPageFromNetwork(QString url)
{
    result = WebPage();
    currentRequest = QNetworkRequest(QUrl(url));
    auto reply = manager.get(currentRequest);
    int retries = 20;
    while(!reply->isFinished() || retries > 0)
    {
        retries--;
        //QThread::sleep(500);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    QByteArray data=reply->readAll();

    error = reply->error();
    reply->deleteLater();
    if(error != QNetworkReply::NoError)
        return result;
    //QString str(data);
    result.content = data;

    result.isValid = true;
    result.url = currentRequest.url().toString();
    result.source = EPageSource::network;
    return result;
}

void PageGetterPrivate::SavePageToDB(const WebPage & page)
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    if(!settings.value("Settings/storeCache", false).toBool())
        return;
    QSqlQuery q(QSqlDatabase::database("Service"));
    q.prepare("delete from pagecache where url = :url");
    q.bindValue(":url", page.url);
    q.exec();
    QString insert = "INSERT INTO PAGECACHE(URL, GENERATION_DATE, CONTENT,  PAGE_TYPE, COMPRESSED) "
                     "VALUES(:URL, :GENERATION_DATE, :CONTENT, :PAGE_TYPE, :COMPRESSED)";
    q.prepare(insert);
    q.bindValue(":URL", page.url);
    q.bindValue(":GENERATION_DATE", QDateTime::currentDateTime());
    q.bindValue(":CONTENT", qCompress(page.content.toUtf8()));
    q.bindValue(":COMPRESSED", 1);
    q.bindValue(":PAGE_TYPE", static_cast<int>(page.type));
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();
}

void PageGetterPrivate::SetDatabase(QSqlDatabase _db)
{
    db  = _db;
}

//void PageGetterPrivate::OnNetworkReply(QNetworkReply * reply)
//{
//    FuncCleanup f([&](){waitLoop.quit();});
//    QByteArray data=reply->readAll();
//    error = reply->error();
//    reply->deleteLater();
//    if(error != QNetworkReply::NoError)
//        return;
//    //QString str(data);
//    result.content = data;
//    result.isValid = true;
//    result.url = currentRequest.url().toString();
//    result.source = EPageSource::network;
//}

PageManager::PageManager() : d(new PageGetterPrivate)
{

}

PageManager::~PageManager()
{
    qDebug() << "deleting page worker";
}

void PageManager::SetDatabase(QSqlDatabase _db)
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
    QSettings settings("settings.ini", QSettings::IniFormat);
    if(settings.value("Settings/storeCache", false).toBool())
        d->SavePageToDB(page);
}
#include "pagegetter.moc"

PageThreadWorker::PageThreadWorker(QObject *parent)
{
    startTimer(2000);
}

PageThreadWorker::~PageThreadWorker()
{
    qDebug() << "deleting pageThread worker";
}

void PageThreadWorker::timerEvent(QTimerEvent *)
{
    qDebug() << "worker is alive";
}

void PageThreadWorker::Task(QString url, QString lastUrl,  QDate updateLimit, ECacheMode cacheMode)
{
    FuncCleanup f([&](){working = false;});

    database::Transaction pcTransaction(QSqlDatabase::database("Service"));
    working = true;
    QScopedPointer<PageManager> pager(new PageManager);
    WebPage result;
    int counter = 0;
    do
    {
        qDebug() << "loading page: " << url;
        auto startPageLoad = std::chrono::high_resolution_clock::now();
        result = pager->GetPage(url, cacheMode);
        result.pageIndex = counter;
        auto minUpdate = GrabMinUpdate(result.content);
        url = GetNext(result.content);
        bool updateLimitReached = false;
        if(counter > 0)
            updateLimitReached = minUpdate < updateLimit;
        if((url == lastUrl || lastUrl.trimmed().isEmpty())|| (updateLimit.isValid() && updateLimitReached))
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
            qDebug() << "thread will sleep for " << timeout;
            QThread::msleep(timeout);
        }
        counter++;
    }while(url != lastUrl && result.isValid && !result.isLastPage);
    emit pageResult({WebPage(), true});
    pcTransaction.finalize();
    qDebug() << "leaving task1";
}

void PageThreadWorker::TaskList(QStringList urls, ECacheMode cacheMode)
{
    FuncCleanup f([&](){working = false;});
    database::Transaction pcTransaction(QSqlDatabase::database("Service"));
    working = true;
    QScopedPointer<PageManager> pager(new PageManager);
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

        emit pageResult({result, false});
        if(!result.isFromCache)
        {
            qDebug() << "thread will sleep for " << timeout;
            QThread::msleep(timeout);
        }
    }
    emit pageResult({WebPage(), true});
    pcTransaction.finalize();
    qDebug() << "leaving task2";
}
static QString CreateURL(QString str)
{
    return "https://www.fanfiction.net/" + str;
}

QString PageThreadWorker::GetNext(QString text)
{
    QString nextUrl;
    QRegExp rxEnd(QRegExp::escape("Next &#187"));
    int indexEnd = rxEnd.indexIn(text);
    if(indexEnd != -1)
        indexEnd-=2;
    int posHref = indexEnd - 400 + text.midRef(indexEnd - 400,400).lastIndexOf("href='");
    nextUrl = CreateURL(text.mid(posHref+6, indexEnd - (posHref+6)));
    if(!nextUrl.contains("&p="))
        nextUrl = "";
    indexEnd = rxEnd.indexIn(text);
    return nextUrl;
}

QDate PageThreadWorker::GrabMinUpdate(QString text)
{
    QDateTime minDate;
    QRegExp rx("Updated:\\s<span\\sdata-xutime='(\\d+)'");
    int indexStart = rx.lastIndexIn(text);
    if(indexStart != 1 && !rx.cap(1).trimmed().replace("-","").isEmpty())
        minDate.setTime_t(rx.cap(1).toInt());

    return minDate.date();
}

