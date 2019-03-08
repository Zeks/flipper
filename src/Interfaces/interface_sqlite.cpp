/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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

