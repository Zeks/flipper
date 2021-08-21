/*Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#include "discord/favourites_fetching.h"
#include "discord/command_generators.h"
#include "discord/db_vendor.h"
#include "parsers/ffn/favparser_wrapper.h"
#include "parsers/ffn/discord/discord_mobile_favparser.h"
#include "timeutils.h"
#include <QFile>
namespace discord {

FavouritesFetchResult TryFetchingDesktopFavourites(QString ffnId, fetching::CacheStrategy cacheStrategy, bool isId)
{
    FavouritesFetchResult resultingData;
    resultingData.ffnId = ffnId;

    auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("pageCache"));

    TimedAction linkGet(QStringLiteral("Link fetch"), [&](){
        parsers::ffn::UserFavouritesParser parser;
        auto result = parser.FetchDesktopUserPage(ffnId, dbToken->db, cacheStrategy,isId);
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
                QFile favouritesfile(QString("tmpfaves/favourites_%1.html").arg(ffnId));
                favouritesfile.rename(QString("tmpfaves/0_err_favourites_%1.html").arg(ffnId));
                favouritesfile.close();
            }
            else {
                // we will fetch first 500 regardless to avoid excessive querying later
                parser.FetchFavouritesFromDesktopPage();
                resultingData.links = parser.GetResultAsIntSet();
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

FavouritesFetchResult FetchMobileFavourites(QString ffnId, fetching::CacheStrategy cacheStrategy)
{
    FavouritesFetchResult data;
    TimedAction linkGet(QStringLiteral("Link fetch"), [&](){
        parsers::ffn::discord::DiscordMobileFavouritesFetcher parser;
        parser.userId = ffnId;
        auto result = parser.Execute(cacheStrategy);
        if(result.size() == 0)
        {
            data.errors.push_back(QStringLiteral("Could not load favourites from provided user profile."));
        }
        data.links += result;
    });
    linkGet.run();
    return data;
}

}
