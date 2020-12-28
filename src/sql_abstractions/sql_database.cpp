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
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_database_impl_base.h"
#include "sql_abstractions/sql_database_impl_null.h"
#include "sql_abstractions/sql_database_impl_sqlite.h"
#include "sql_abstractions/sql_database_impl_pq.h"
#include <locale>
namespace sql {
static std::unordered_map<std::string, Database> databases;

Database::Database():d(new DatabaseImplNull())
{

}

static void toUpper(std::string& s){
    std::locale locale;
    auto to_upper = [&locale] (char ch) { return std::use_facet<std::ctype<char>>(locale).toupper(ch); };
    std::transform(s.begin(), s.end(), s.begin(), to_upper);
}

Database Database::addDatabase(std::string driver, std::string name)
{
    toUpper(driver);toUpper(name);
    Database db;

    if(driver == "QSQLITE"){
        auto impl = std::make_shared<DatabaseImplSqlite>();
        impl->db =  impl->addDatabase(driver, name);
        if(impl->connectionName() != name)
            throw std::runtime_error("database wasn't created properly");
        db.d = impl;
    }
    else if(driver == "PQXX")
    {
        auto impl = std::make_shared<DatabaseImplPq>(name);
        db.d = impl;
    }

    databases.insert_or_assign(name, db);
    return db;
}

void Database::removeDatabase(std::string name)
{
    toUpper(name);
    databases.erase(name);
}

Database Database::database(std::string name)
{
    toUpper(name);
    auto it = databases.find(name);
    if(it != databases.end())
        return it->second;
    return Database();
}

void Database::setConnectionToken(ConnectionToken token)
{
    d->setConnectionToken(token);
}

std::string Database::driverType() const
{
    return d->driverType();
}

bool Database::open()
{
    return d->open();
}

bool Database::isOpen() const
{
    return d->isOpen();
}

bool Database::transaction()
{
    return d->transaction();
}

bool Database::commit()
{
    return d->commit();
}

bool Database::rollback()
{
    return d->rollback();
}

bool Database::hasOpenTransaction() const
{
    return d->hasOpenTransaction();
}

void *Database::internalPointer()
{
    return d->internalPointer();
}

void Database::close()
{
    d->close();
}

std::string Database::connectionName()
{
    return d->connectionName();
}

}
