#pragma once

#include <QObject>
#include "sql_abstractions/sql_database.h"
#include "include/web/cache_strategy.h"

namespace parsers{
namespace ffn{
namespace discord{
class DiscordMobileFavouritesFetcher : public QObject{
    Q_OBJECT;
public:
    DiscordMobileFavouritesFetcher(QObject* parent = nullptr);
    QSet<int> Execute(fetching::CacheStrategy cacheStrategy);
    QString userId;
    int pageToStartFrom = 0;
    int timeout = 500;
signals:
    void progress(int pagesRead, int totalPages);
};
}
}
}




