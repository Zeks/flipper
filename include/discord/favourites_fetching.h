#pragma once
#include "ECacheMode.h"

#include <QSet>
#include <QSharedPointer>


namespace discord{
class SendMessageCommand;

struct FavouritesFetchResult{
    bool finished = false;
    bool requiresFullParse = false;
    bool hasFavourites = true;
    int totalFavourites = 0;
    QSet<QString> links;
    QStringList errors;
    QString ffnId;
};

FavouritesFetchResult TryFetchingDesktopFavourites(QString ffnId, ECacheMode cacheMode = ECacheMode::use_only_cache, bool isId = true);
FavouritesFetchResult FetchMobileFavourites(QString ffnId, ECacheMode cacheMode = ECacheMode::use_only_cache);

QSet<QString> FetchUserFavourites(QString ffnId, QSharedPointer<SendMessageCommand> action, ECacheMode cacheMode = ECacheMode::use_only_cache);
}

