#pragma once
#include <QString>
#include <QDateTime>
#include <QByteArray>
#include <QScopedPointer>

enum class EPageType
{
    hub_page,
    sorted_ficlist,
    author_profile,
    fic_page
};

struct WebPage
{
    QString url;
    QDateTime generated;
    QByteArray content;
    QString previousUrl;
    QString nextUrl;
    QStringList referencedFics;
    EPageType type;
    bool isValid = false;
};

class PageGetterPrivate;
class PageManager
{
    public:
    PageManager();
    ~PageManager();
    void SetCachedMode(bool value);
    bool GetCachedMode() const;
    WebPage GetPage(QString url, EPageType type, bool useCache = false);
    void SavePageToDB(const WebPage & page,  EPageType type);
    QScopedPointer<PageGetterPrivate> d;
};
