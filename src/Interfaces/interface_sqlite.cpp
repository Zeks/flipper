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

QStringList SqliteInterface::GetIdListForQuery(QSharedPointer<core::Query> query, QSqlDatabase db)
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

QSqlDatabase SqliteInterface::InitDatabase(QString connectionName, bool setDefault)
{
    db = sqlite::InitDatabase(connectionName, setDefault);
    return db;
}

QSqlDatabase SqliteInterface::InitDatabase2(QString fileName, QString connectionName, bool setDefault)
{
    db = sqlite::InitDatabase2(fileName, connectionName, setDefault);
    return db;
}

QSqlDatabase SqliteInterface::InitAndUpdateDatabaseForFile(QString folder,
                                                           QString file,
                                                           QString sqlFile,
                                                           QString connectionName,
                                                           bool setDefault)
{
    db = sqlite::InitAndUpdateDatabaseForFile(folder,file,  sqlFile, connectionName, setDefault);
    return db;
}

//DiagnosticSQLResult<bool> PassScoresToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget);
//DiagnosticSQLResult<bool> PassSnoozesToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget);
//DiagnosticSQLResult<bool> PassFicTagsToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget);
//DiagnosticSQLResult<bool> PassFicNotesToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget);
//DiagnosticSQLResult<bool> PassTagSetToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget);
//DiagnosticSQLResult<bool> PassRecentFandomsToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget);
//DiagnosticSQLResult<bool> PassIgnoredFandomsToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget);
//DiagnosticSQLResult<bool> PassClientDataToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget);

bool SqliteInterface::PassScoresToAnotherDatabase(QSqlDatabase dbTarget)
{
    return puresql::PassScoresToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassSnoozesToAnotherDatabase(QSqlDatabase dbTarget)
{
    return puresql::PassSnoozesToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassFicTagsToAnotherDatabase(QSqlDatabase dbTarget)
{
    return puresql::PassFicTagsToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassFicNotesToAnotherDatabase(QSqlDatabase dbTarget)
{
    return puresql::PassFicNotesToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassTagSetToAnotherDatabase(QSqlDatabase dbTarget)
{
    return puresql::PassTagSetToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassRecentFandomsToAnotherDatabase(QSqlDatabase dbTarget)
{
    return puresql::PassRecentFandomsToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassIgnoredFandomsToAnotherDatabase(QSqlDatabase dbTarget)
{
    return puresql::PassIgnoredFandomsToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassFandomListSetToAnotherDatabase(QSqlDatabase dbTarget)
{
    return puresql::PassFandomListSetToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassFandomListDataToAnotherDatabase(QSqlDatabase dbTarget)
{
    return puresql::PassFandomListDataToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassClientDataToAnotherDatabase(QSqlDatabase dbTarget)
{
    return puresql::PassClientDataToAnotherDatabase(db, dbTarget).success;
}

bool SqliteInterface::PassReadingDataToAnotherDatabase(QSqlDatabase dbTarget)
{
    return puresql::PassReadingDataToAnotherDatabase(db, dbTarget).success;
}

QSqlDatabase SqliteInterface::InitNamedDatabase(QString dbName, QString fileName, bool setDefault)
{
    db = sqlite::InitNamedDatabase(dbName, fileName, setDefault);
    EnsureUUIDForUserDatabase();
    return db;
}

bool SqliteInterface::EnsureUUIDForUserDatabase()
{
    return database::puresql::EnsureUUIDForUserDatabase(QUuid::createUuid(), db).success;
}

QString SqliteInterface::GetUserToken()
{
    return database::puresql::GetUserToken(db).data;
}

}

