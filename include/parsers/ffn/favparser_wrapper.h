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
};
class UserFavouritesParser : public QObject{
Q_OBJECT
public:
    UserFavouritesParser(QObject* parent = nullptr);
    void Reset();
    bool FetchDesktopUserPage(QString userId);
    QuickParseResult QuickParseAvailable();
    void FetchFavouritesFromDesktopPage();
    void FetchFavouritesFromMobilePage(int startBoundary = 26);

    WebPage dektopPage;
    QSet<QString> result;
    QString userId;
    QMetaObject::Connection connection;

signals:
    void progress(int pagesRead, int totalPages);
};



}}
