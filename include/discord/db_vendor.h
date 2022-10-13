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
#pragma once

#include "GlobalHeaders/SingletonHolder.h"
#include "sql_abstractions/sql_connection_token.h"
#include "sql_abstractions/sql_database.h"


#include <mutex>

namespace discord {
struct SqliteConnectionToken{
    QString databaseName;
    QString initFileName;
    QString folder;
};

struct LockedDatabase {
    LockedDatabase(std::recursive_mutex& lock): lockGuard(lock){}
    ~LockedDatabase();
    std::lock_guard<std::recursive_mutex> lockGuard;
    sql::Database db;
};



class DatabaseVendor{
public:
    QSharedPointer<LockedDatabase> GetDatabase(QString name);
    void AddConnectionToken(QString, const sql::ConnectionToken &);
private:
    sql::Database InstantiateDatabase(const sql::ConnectionToken&);
    sql::ConnectionToken users;
    sql::ConnectionToken pageCache;
    std::recursive_mutex usersLock;
    std::recursive_mutex pageCacheLock;
};

}
BIND_TO_SELF_SINGLE(discord::DatabaseVendor);
