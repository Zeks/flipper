#pragma once
#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include <QSharedPointer>
//#include "queryinterfaces.h"
namespace core { struct Query;}
namespace database{

class IDBWrapper{
public:
    virtual ~IDBWrapper();
    virtual int GetLastIdForTable(QString tableName) = 0;
    virtual bool PushFandomToTopOfRecent(QString fandom) = 0;
    virtual bool RebaseFandomsToZero() = 0;
    virtual QStringList FetchRecentFandoms() = 0;
    virtual QDateTime GetCurrentDateTime() = 0;
    virtual QStringList GetIdListForQuery(QSharedPointer<core::Query> query) = 0;
    virtual bool BackupDatabase(QString dbname) = 0;
    virtual bool ReadDbFile(QString file, QString connectionName = "default") = 0;
    virtual QSqlDatabase InitDatabase(QString connectionName = "default") = 0;
    QSqlDatabase GetDatabase() {return db;}
protected:

    QSqlDatabase db;
};
}
