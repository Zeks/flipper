/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "Interfaces/interface_sqlite.h"
#include "include/sqlitefunctions.h"
#include <QUuid>
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

QStringList SqliteInterface::GetIdListForQuery(QSharedPointer<core::Query> query, sql::Database db)
{
    if(db.isOpen())
        return sqlite::GetIdListForQuery(query, db);
    else
        return sqlite::GetIdListForQuery(query, this->db);
}

bool SqliteInterface::BackupDatabase(QString dbname)
{
    return sqlite::BackupSqliteDatabase(dbname);
}

bool SqliteInterface::ReadDbFile(QString file, QString connectionName)
{
    return sqlite::ReadDbFile(file, connectionName);
}

sql::Database SqliteInterface::InitDatabase(QString connectionName, bool setDefault)
{
    db = sqlite::InitSqliteDatabase(connectionName, setDefault);
    return db;
}

sql::Database SqliteInterface::InitDatabase2(QString fileName, QString connectionName, bool setDefault)
{
    db = sqlite::InitSqliteDatabase2(fileName, connectionName, setDefault);
    return db;
}

sql::Database SqliteInterface::InitAndUpdateDatabaseForFile(QString folder,
                                                           QString file,
                                                           QString sqlFile,
                                                           QString connectionName,
                                                           bool setDefault)
{
    db = sqlite::InitAndUpdateSqliteDatabaseForFile(folder,file,  sqlFile, connectionName, setDefault);
    return db;
}

//DiagnosticSQLResult<bool> PassScoresToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
//DiagnosticSQLResult<bool> PassSnoozesToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
//DiagnosticSQLResult<bool> PassFicTagsToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
//DiagnosticSQLResult<bool> PassFicNotesToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
//DiagnosticSQLResult<bool> PassTagSetToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
//DiagnosticSQLResult<bool> PassRecentFandomsToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
//DiagnosticSQLResult<bool> PassIgnoredFandomsToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
//DiagnosticSQLResult<bool> PassClientDataToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);

bool SqliteInterface::PassScoresToAnotherDatabase(sql::Database dbTarget)
{
    return sql::PassScoresToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassSnoozesToAnotherDatabase(sql::Database dbTarget)
{
    return sql::PassSnoozesToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassFicTagsToAnotherDatabase(sql::Database dbTarget)
{
    return sql::PassFicTagsToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassFicNotesToAnotherDatabase(sql::Database dbTarget)
{
    return sql::PassFicNotesToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassTagSetToAnotherDatabase(sql::Database dbTarget)
{
    return sql::PassTagSetToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassRecentFandomsToAnotherDatabase(sql::Database dbTarget)
{
    return sql::PassRecentFandomsToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassIgnoredFandomsToAnotherDatabase(sql::Database dbTarget)
{
    return sql::PassIgnoredFandomsToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassFandomListSetToAnotherDatabase(sql::Database dbTarget)
{
    return sql::PassFandomListSetToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassFandomListDataToAnotherDatabase(sql::Database dbTarget)
{
    return sql::PassFandomListDataToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassClientDataToAnotherDatabase(sql::Database dbTarget)
{
    return sql::PassClientDataToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassReadingDataToAnotherDatabase(sql::Database dbTarget)
{
    return sql::PassReadingDataToAnotherDatabase(db, dbTarget).success;
}

sql::Database SqliteInterface::InitNamedDatabase(QString dbName, QString fileName, bool setDefault)
{
    db = sqlite::InitNamedSqliteDatabase(dbName, fileName, setDefault);
    EnsureUUIDForUserDatabase();
    return db;
}

bool SqliteInterface::EnsureUUIDForUserDatabase()
{
    return sql::EnsureUUIDForUserDatabase(QUuid::createUuid(), db).success;
}

QString SqliteInterface::GetUserToken()
{
    return sql::GetUserToken(db).data;
}

}

