/*Flipper is a recommendation and search engine for fanfiction.net
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
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#include "discord/db_vendor.h"
#include "logger/QsLog.h"
#include <QUuid>
#include "sql_abstractions/sql_query.h"
#include <QSqlError>

namespace discord {

QSharedPointer<LockedDatabase> DatabaseVendor::GetDatabase(QString name)
{
    QSharedPointer<LockedDatabase> databaseWrapper;
    if(name == QStringLiteral("users"))
    {
        databaseWrapper.reset(new LockedDatabase(usersLock));
        auto db = InstantiateDatabase(users);
        databaseWrapper->db = db;
    }
    else{
        databaseWrapper.reset(new LockedDatabase(pageCacheLock));
        auto db = InstantiateDatabase(pageCache);
        databaseWrapper->db = db;
    }
    return databaseWrapper;
}

void DatabaseVendor::AddConnectionToken(QString name, const SqliteConnectionToken& token)
{
    if(name == QStringLiteral("users"))
        users = token;
    else
        pageCache = token;
}

sql::Database DatabaseVendor::InstantiateDatabase(const SqliteConnectionToken & token)
{
    sql::Database db;
    db = sql::Database::addDatabase(QStringLiteral("QSQLITE"), token.databaseName + QUuid::createUuid().toString());
    QString filename = token.folder.isEmpty() ? token.databaseName : token.folder + "/" + token.databaseName;
    db.setDatabaseName(filename + QStringLiteral(".sqlite"));
    db.open();
    return db;
}

LockedDatabase::~LockedDatabase()
{
    db.close();
}

}
