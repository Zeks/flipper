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
    QSet<int> links;
    QStringList errors;
    QString ffnId;
};

FavouritesFetchResult TryFetchingDesktopFavourites(QString ffnId, fetching::CacheStrategy cacheStrategy, bool isId = true);
FavouritesFetchResult FetchMobileFavourites(QString ffnId, fetching::CacheStrategy cacheStrategy);

QSet<QString> FetchUserFavourites(QString ffnId, QSharedPointer<SendMessageCommand> action, fetching::CacheStrategy cacheStrategy);
}

