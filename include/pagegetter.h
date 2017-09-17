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
    QDateTime GrabMinUpdate(QString text);
    int timeout = 500;
    std::atomic<bool> working;
public slots:
    void Task(QString url, QString lastUrl, QDateTime updateLimit, ECacheMode cacheMode);
    void TaskList(QStringList urls, ECacheMode cacheMode);
signals:
    void pageReady(WebPage);
};


BIND_TO_SELF_SINGLE(PageManager);
