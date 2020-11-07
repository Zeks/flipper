#pragma once
#include "include/webpage.h"
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
    bool FetchDesktopUserPage(QString userId, sql::Database, ECacheMode cacheMode = ECacheMode::dont_use_cache, bool isId = true);
    QuickParseResult QuickParseAvailable();
    void FetchFavouritesFromDesktopPage();
    void FetchFavouritesFromMobilePage(int startBoundary = 26);

    WebPage dektopPage;
    QSet<QString> result;
    QString userId;
    QMetaObject::Connection connection;
    ECacheMode cacheMode = ECacheMode::dont_use_cache;
    sql::Database cacheDbToUse;

signals:
    void progress(int pagesRead, int totalPages);
};



}}
