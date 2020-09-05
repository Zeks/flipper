#pragma once
#include <QObject>
#include <QSqlDatabase>

namespace parsers{
namespace ffn{

class MobileFavouritesFetcher : public QObject{
    Q_OBJECT;
public:
    MobileFavouritesFetcher(QObject* parent = nullptr);
    QSet<QString> Execute();
    QSet<QString> Execute(QSqlDatabase);
    QString userId;
    int pageToStartFrom = 0;
    int timeout = 500;
signals:
    void progress(int pagesRead, int totalPages);
};

}
}




