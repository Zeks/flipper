#pragma once
#include <QString>
#include <QDateTime>
#include <QByteArray>
#include <QScopedPointer>
#include <QSqlDatabase>
#include "GlobalHeaders/SingletonHolder.h"

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
    WebPage GetPage(QString url, bool useCache = false);
    void SavePageToDB(const WebPage & page);
    QScopedPointer<PageGetterPrivate> d;
};

class PageThreadWorker: public QObject
{
    Q_OBJECT
public:
    PageThreadWorker(QObject* parent = nullptr);
    QString GetNext(QString);
    QDateTime GrabMinUpdate(QString text);

public slots:
    void Task(QString url, QString lastUrl, QDateTime updateLimit, bool cacheMode);
signals:
    void pageReady(WebPage);
};


BIND_TO_SELF_SINGLE(PageManager);
