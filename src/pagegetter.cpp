#include "include/pagegetter.h"
#include "GlobalHeaders/run_once.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QObject>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QEventLoop>


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
    WebPage GetPage(QString url, bool useCache = false);
    WebPage GetPageFromDB(QString url);
    WebPage GetPageFromNetwork(QString url);
    void SavePageToDB(const WebPage&);
    void SetDatabase(QSqlDatabase _db);
    QSqlDatabase db;

public slots:
    void OnNetworkReply(QNetworkReply*);
};

PageGetterPrivate::PageGetterPrivate(QObject *parent): QObject(parent)
{
    connect(&manager, SIGNAL (finished(QNetworkReply*)),
            this, SLOT (OnNetworkReply(QNetworkReply*)));
}

WebPage PageGetterPrivate::GetPage(QString url, bool useCache)
{
    WebPage result;
    if(useCache)
    {
        result = GetPageFromDB(url);
        if(result.isValid)
            return result;
    }
    else
        result = GetPageFromNetwork(url);
    return result;
}

WebPage PageGetterPrivate::GetPageFromDB(QString url)
{
    WebPage result;
    QSqlQuery q(db);
    q.prepare("select * from PageCache where url = :URL");
    q.bindValue(":URL", url);
    q.exec();
    q.next();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        return result;
    }
    qDebug() << q.record();
    result.url = url;
    result.isValid = true;
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
    manager.get(currentRequest);
    waitLoop.exec();
    return result;
}

void PageGetterPrivate::SavePageToDB(const WebPage & page)
{
    QSqlQuery q(db);
    q.prepare("delete from pagecache where url = :url");
    q.bindValue(":url", page.url);
    q.exec();
    QString insert = "INSERT INTO PAGECACHE(URL, GENERATION_DATE, CONTENT, FANDOM, CROSSOVER, PAGE_TYPE) "
                     "VALUES(:URL, :GENERATION_DATE, :CONTENT, :FANDOM, :CROSSOVER, :PAGE_TYPE)";
    q.prepare(insert);
    q.bindValue(":URL", page.url);
    q.bindValue(":GENERATION_DATE", QDateTime::currentDateTime());
    q.bindValue(":CONTENT", page.content);
    q.bindValue(":FANDOM", page.fandom);
    q.bindValue(":CROSSOVER", page.crossover);
    q.bindValue(":PAGE_TYPE", static_cast<int>(page.type));
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();
}

void PageGetterPrivate::SetDatabase(QSqlDatabase _db)
{
    db  = _db;
}

void PageGetterPrivate::OnNetworkReply(QNetworkReply * reply)
{
    FuncCleanup f([&](){waitLoop.quit();});
    QByteArray data=reply->readAll();
    error = reply->error();
    reply->deleteLater();
    if(error != QNetworkReply::NoError)
        return;
    QString str(data);
    result.content = data;
    result.isValid = true;
    result.url = currentRequest.url().toString();
    result.source = EPageSource::network;
}

PageManager::PageManager() : d(new PageGetterPrivate)
{

}

PageManager::~PageManager()
{
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

WebPage PageManager::GetPage(QString url, bool useCache)
{
    return d->GetPage(url, useCache);
}

void PageManager::SavePageToDB(const WebPage & page)
{
    d->SavePageToDB(page);
}
#include "pagegetter.moc"
