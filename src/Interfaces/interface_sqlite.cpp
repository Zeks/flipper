
#include "Interfaces/interface_sqlite.h"
#include "include/sqlitefunctions.h"
namespace database{

SqliteInterface::SqliteInterface()
{

}

SqliteInterface::~SqliteInterface()
{}

int SqliteInterface::GetLastIdForTable(QString tableName)
{
    return sqlite::GetLastIdForTable(tableName, db);
}

bool SqliteInterface::PushFandomToTopOfRecent(QString fandom)
{
    return sqlite::PushFandomToTopOfRecent(fandom, db);
}

bool SqliteInterface::RebaseFandomsToZero()
{
    return sqlite::RebaseFandomsToZero(db);
}

QStringList SqliteInterface::FetchRecentFandoms()
{
    return sqlite::FetchRecentFandoms(db);
}

QDateTime SqliteInterface::GetCurrentDateTime()
{
    return sqlite::GetCurrentDateTime(db);
}

QStringList SqliteInterface::GetIdListForQuery(QSharedPointer<core::Query> query)
{
    return sqlite::GetIdListForQuery(query, db);
}

bool SqliteInterface::BackupDatabase(QString dbname)
{
    return sqlite::BackupSqliteDatabase(dbname);
}

bool SqliteInterface::ReadDbFile(QString file, QString connectionName)
{
        return sqlite::ReadDbFile(file, connectionName);
}

QSqlDatabase SqliteInterface::InitDatabase(QString connectionName)
{
    return sqlite::InitDatabase(connectionName);
}

}

