#include "include/pagegetter.h"
#include "GlobalHeaders/run_once.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QObject>
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
    void SavePageToDB(const WebPage&,  EPageType type);

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
    return GetPageFromNetwork(url);
}

WebPage PageGetterPrivate::GetPageFromDB(QString url)
{
    return WebPage();
}

WebPage PageGetterPrivate::GetPageFromNetwork(QString url)
{
    result = WebPage();
    currentRequest = QNetworkRequest(QUrl(url));
    manager.get(currentRequest);
    waitLoop.exec();
    return result;
}

void PageGetterPrivate::SavePageToDB(const WebPage &, EPageType type)
{

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
}

PageManager::PageManager() : d(new PageGetterPrivate)
{

}

PageManager::~PageManager()
{
}

void PageManager::SetCachedMode(bool value)
{
    d->cachedMode = value;
}

bool PageManager::GetCachedMode() const
{
    return d->cachedMode;
}

WebPage PageManager::GetPage(QString url, EPageType type, bool useCache)
{
    return d->GetPage(url, useCache);
}

void PageManager::SavePageToDB(const WebPage & page,  EPageType type)
{
    d->SavePageToDB(page, type);
}
#include "pagegetter.moc"
