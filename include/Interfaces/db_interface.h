#pragma once
#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include <QSharedPointer>
//#include "queryinterfaces.h"
namespace core { class Query;}
namespace database{

class IDBWrapper{
public:
    virtual ~IDBWrapper();
    virtual int GetLastIdForTable(QString tableName, QSqlDatabase db) = 0;
    bool PushFandomToTopOfRecent(QString fandom, QSqlDatabase db);
    bool RebaseFandomsToZero(QSqlDatabase db);
    QStringList FetchRecentFandoms(QSqlDatabase db);
    virtual QDateTime GetCurrentDateTime() = 0;
    QStringList GetIdListForQuery(QSharedPointer<core::Query> query, QSqlDatabase db);
    void BackupDatabase(QString dbname);
    bool ReadDbFile(QString file, QString connectionName = "default");
    QSqlDatabase InitDatabase(QString connectionName = "default");
};
}
