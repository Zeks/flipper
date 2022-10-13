#pragma once
#include "include/web/webpage.h"
#include "include/parsers/ffn/desktop_favparser.h"
#include "include/parsers/ffn/mobile_favparser.h"
#include <QObject>

namespace parsers{
namespace ffn{

struct QuickParseResult{
    bool validFavouritesCount = false;
    bool canDoQuickParse = false;
    QString mobileUserId;
};
class UserFavouritesParser : public QObject{
Q_OBJECT
public:
    UserFavouritesParser(QObject* parent = nullptr);
    void Reset();
    bool FetchDesktopUserPage(QString userId);
    bool FetchDesktopUserPage(QString userId, sql::Database, fetching::CacheStrategy cacheStrategy, bool isId = true);
    QuickParseResult QuickParseAvailable();
    void FetchFavouritesFromDesktopPage();
    void FetchFavouritesFromMobilePage(int startBoundary = 26);
    QSet<int> GetResultAsIntSet() const;

    WebPage dektopPage;
    QSet<QString> result;
    QString userId;
    QMetaObject::Connection connection;
    fetching::CacheStrategy cacheStrategy;
    sql::Database cacheDbToUse;

signals:
    void progress(int pagesRead, int totalPages);
};



}}
