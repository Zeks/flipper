#include "include/parsers/ffn/favparser_wrapper.h"
#include "include/parsers/ffn/mobile_favparser.h"
#include "include/timeutils.h"
#include "include/environment.h"
#include <stdexcept>
namespace parsers{
namespace ffn{

UserFavouritesParser::UserFavouritesParser(QObject *parent): QObject(parent)
{
}

void UserFavouritesParser::Reset()
{
    dektopPage = {};
    result = {};
    if(connection)
        disconnect(connection);
}

bool UserFavouritesParser::FetchDesktopUserPage(QString userId)
{
    this->userId = userId;
    QStringList result;
    WebPage page;
    QString pageUrl = QString("https://www.fanfiction.net/u/%1").arg(userId);
    TimedAction fetchAction("Author page fetch", [&](){
        page = env::RequestPage(pageUrl,  ECacheMode::dont_use_cache);
    });
    fetchAction.run(false);
    dektopPage = page;
    if(page.isValid)
        return true;
    return false;
}

QuickParseResult UserFavouritesParser::QuickParseAvailable()
{
    QuickParseResult result;
    QRegularExpression rx("Favorite\\sStories\\s<span\\sclass=badge>(\\d{1,3})</span>");
    auto match = rx.match(dektopPage.content);
    if(!match.hasMatch())
    {
        result.validFavouritesCount = false;
        return result;
    }

    int amountOfFavourites = match.captured(1).toInt();
    if(amountOfFavourites < 500)
        result.canDoQuickParse = true;
    else
        result.canDoQuickParse = false;
    result.validFavouritesCount = true;
    return result;
}

void UserFavouritesParser::FetchFavouritesFromDesktopPage()
{
    FavouriteStoryParser parser;
    QString name = ParseAuthorNameFromFavouritePage(dektopPage.content);
    parser.authorName = name;
    parser.ProcessPage(dektopPage.url, dektopPage.content);
    result+=parser.FetchFavouritesIdList();
}

void UserFavouritesParser::FetchFavouritesFromMobilePage(int startBoundary)
{
    MobileFavouritesFetcher parser;
    parser.userId = this->userId;
    parser.pageToStartFrom = startBoundary;

    connection = connect(&parser, &MobileFavouritesFetcher::progress, this, &UserFavouritesParser::progress);
    result+=parser.Execute();
}


}}
