#pragma once
#include "ECacheMode.h"
#include <QObject>
#include <QSqlDatabase>

namespace parsers{
namespace ffn{
namespace discord{
class DiscordMobileFavouritesFetcher : public QObject{
    Q_OBJECT;
public:
    DiscordMobileFavouritesFetcher(QObject* parent = nullptr);
    QSet<QString> Execute(ECacheMode cacheMode);
    QString userId;
    int pageToStartFrom = 0;
    int timeout = 500;
signals:
    void progress(int pagesRead, int totalPages);
};
}
}
}




