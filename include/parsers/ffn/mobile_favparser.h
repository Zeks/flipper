#pragma once
#include "ECacheMode.h"
#include <QObject>
#include "sql_abstractions/sql_database.h"

namespace parsers{
namespace ffn{

class MobileFavouritesFetcher : public QObject{
    Q_OBJECT;
public:
    MobileFavouritesFetcher(QObject* parent = nullptr);
    QSet<QString> Execute();
    QSet<QString> Execute(sql::Database, ECacheMode cacheMode = ECacheMode::dont_use_cache);
    QString userId;
    int pageToStartFrom = 0;
    int timeout = 500;
signals:
    void progress(int pagesRead, int totalPages);
};

}
}




