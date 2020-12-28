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
#include "sql_abstractions/sql_database_impl_sqlite.h"

QSqlDatabase sql::DatabaseImplSqlite::addDatabase(std::string type, std::string name)
{
   return QSqlDatabase::addDatabase(QString::fromStdString(type),QString::fromStdString(name)) ;
}

QSqlDatabase sql::DatabaseImplSqlite::database(std::string name)
{
    return QSqlDatabase::addDatabase(QString::fromStdString(name)) ;
}

void sql::DatabaseImplSqlite::setConnectionToken(ConnectionToken token)
{
    db.setDatabaseName(QString::fromStdString(token.serviceName));
}

bool sql::DatabaseImplSqlite::open()
{
    return db.open();
}

bool sql::DatabaseImplSqlite::isOpen() const
{
    return db.isOpen();
}

bool sql::DatabaseImplSqlite::transaction()
{
    _hasOpenTransaction = db.transaction();;
    return _hasOpenTransaction;
}

bool sql::DatabaseImplSqlite::hasOpenTransaction() const
{
    return _hasOpenTransaction;
}

bool sql::DatabaseImplSqlite::commit()
{
    _hasOpenTransaction = false;
    return db.commit();
}

bool sql::DatabaseImplSqlite::rollback()
{
    _hasOpenTransaction = false;
    return db.rollback();
}

void sql::DatabaseImplSqlite::close()
{
    _hasOpenTransaction = false;
    db.close();
}

std::string sql::DatabaseImplSqlite::connectionName()
{
    return db.connectionName().toStdString();
}

void *sql::DatabaseImplSqlite::internalPointer()
{
    return static_cast<void*>(&db);
}

bool sql::DatabaseImplSqlite::isNull()
{
    return false;
}

std::string sql::DatabaseImplSqlite::driverType() const
{
    return "QSQLITE";
}
