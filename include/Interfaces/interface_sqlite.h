#pragma once
#include "Interfaces/db_interface.h"
#include <QSharedPointer>


namespace database{
class SqliteInterface : public IDBWrapper
{
public:
    SqliteInterface();
    ~SqliteInterface();
    int GetLastIdForTable(QString tableName);
    bool PushFandomToTopOfRecent(QString fandom);
    bool RebaseFandomsToZero();
    QStringList FetchRecentFandoms();
    QDateTime GetCurrentDateTime();
    QStringList GetIdListForQuery(QSharedPointer<core::Query> query);
    bool BackupDatabase(QString dbname);
    bool ReadDbFile(QString file, QString connectionName);
    QSqlDatabase InitDatabase(QString connectionName, bool setDefault = false);

};

}
