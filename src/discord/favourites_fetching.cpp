#include "discord/favourites_fetching.h"
#include "discord/command_generators.h"
#include "discord/db_vendor.h"
#include "parsers/ffn/favparser_wrapper.h"
#include "parsers/ffn/discord/discord_mobile_favparser.h"
#include "timeutils.h"
namespace discord {

FavouritesFetchResult TryFetchingDesktopFavourites(QString ffnId, ECacheMode cacheMode, bool isId)
{
    FavouritesFetchResult resultingData;
    resultingData.ffnId = ffnId;

    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("pageCache"));

    TimedAction linkGet(QStringLiteral("Link fetch"), [&](){
        parsers::ffn::UserFavouritesParser parser;
        auto result = parser.FetchDesktopUserPage(ffnId, dbToken->db, cacheMode,isId);
        parsers::ffn::QuickParseResult quickResult;

        if(!result)
            resultingData.errors.push_back(QStringLiteral("Could not load user page on FFN. Please send your FFN ID and this error to ficfliper@gmail.com if you want it fixed."));

        parser.cacheDbToUse = dbToken->db;
        if(resultingData.errors.size() == 0)
        {
            quickResult = parser.QuickParseAvailable();
            resultingData.ffnId = quickResult.mobileUserId;
            if(!quickResult.validFavouritesCount){
                resultingData.hasFavourites = false;
                resultingData.errors.push_back(QStringLiteral("Could not load favourites from provided user profile."));
            }
            else {
                // we will fetch first 500 regardless to avoid excessive querying later
                parser.FetchFavouritesFromDesktopPage();
                resultingData.links = parser.result;
                if(quickResult.canDoQuickParse)
                    resultingData.finished = true;
                else
                    resultingData.requiresFullParse = true;
            }
        }
    });
    linkGet.run();
    return resultingData;
}

FavouritesFetchResult FetchMobileFavourites(QString ffnId, ECacheMode cacheMode)
{
    FavouritesFetchResult data;
    TimedAction linkGet(QStringLiteral("Link fetch"), [&](){
        parsers::ffn::discord::DiscordMobileFavouritesFetcher parser;
        parser.userId = ffnId;
        auto result = parser.Execute(cacheMode);
        if(result.size() == 0)
            data.errors.push_back(QStringLiteral("Could not load favourites from provided user profile."));
        data.links += result;
    });
    linkGet.run();
    return data;
}

}
