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
    virtual bool PushFandomToTopOfRecent(QString fandom, QSqlDatabase db) = 0;
    virtual bool RebaseFandomsToZero(QSqlDatabase db) = 0;
    virtual QStringList FetchRecentFandoms(QSqlDatabase db) = 0;
    virtual QDateTime GetCurrentDateTime() = 0;
    virtual QStringList GetIdListForQuery(QSharedPointer<core::Query> query, QSqlDatabase db) = 0;
    virtual void BackupDatabase(QString dbname) = 0;
    virtual bool ReadDbFile(QString file, QString connectionName = "default") = 0;
    virtual QSqlDatabase InitDatabase(QString connectionName = "default") = 0;
};
}
