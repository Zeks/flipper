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
#include "sql_abstractions/sql_database_impl_pq.h"
#include "sql_abstractions/sql_error.h"

#include "fmt/format.h"
#include "logger/QsLog.h"

#include <optional>
#include <iostream>

namespace sql {
class DatabaseImplPqImpl:std::enable_shared_from_this<DatabaseImplPqImpl>{
public:
    //std::shared_ptr<PQXXConnectionWrapper> connection;
    std::shared_ptr<pqxx::connection> connection;
    std::string connectionName;
    ConnectionToken token;
    std::shared_ptr<pqxx::work> transaction;
    Error lastError;
};

DatabaseImplPq::DatabaseImplPq(std::string connectionName)
{
    d.reset(new DatabaseImplPqImpl());
    d->connectionName = connectionName;
}

void DatabaseImplPq::setConnectionToken(ConnectionToken token)
{
    d->token = token;
}

bool DatabaseImplPq::open()
{
    auto& token = d->token;
    auto uri = fmt::format("postgresql://{0}:{1}@{2}:{3}", token.user,token.password, token.ip, token.port);
    try {
        d->connection = std::shared_ptr<pqxx::connection>{new pqxx::connection(uri)};
    }
    catch (const pqxx::broken_connection& e) {
        d->lastError = Error(e.what(), ESqlErrors::se_broken_connection);
        QLOG_ERROR() << e.what();
        return false;
    }
    catch (const pqxx::failure& e) {
        d->lastError = Error(e.what(), ESqlErrors::se_generic_sql_error);
        QLOG_ERROR() << e.what();
        return false;
    }
    return d->connection->is_open();
}

bool DatabaseImplPq::isOpen() const
{
    try {
        return d->connection->is_open();
    }  catch (const pqxx::failure& e) {
        d->lastError = Error(e.what(), ESqlErrors::se_generic_sql_error);
        QLOG_ERROR() << e.what();
        return false;
    }
    return true;

}

bool DatabaseImplPq::transaction()
{
    if(d->transaction)
        return true;
    try {
        d->transaction.reset(new pqxx::work(*d->connection));
    }
    catch (const pqxx::broken_connection& e) {
        d->transaction.reset();
        d->lastError = Error(e.what(), ESqlErrors::se_broken_connection);
        QLOG_ERROR() << e.what();
        return false;
    }
    catch (const pqxx::failure& e) {
        d->transaction.reset();
        d->lastError = Error(e.what(), ESqlErrors::se_generic_sql_error);
        QLOG_ERROR() << e.what();
        return false;
    }
    return true;
}

bool DatabaseImplPq::hasOpenTransaction() const
{
    return d->transaction != nullptr;
}

bool DatabaseImplPq::commit()
{
    try{
        d->transaction->commit();
    }  catch (const pqxx::failure& e) {
        d->lastError = Error(e.what(), ESqlErrors::se_generic_sql_error);
        QLOG_ERROR() << e.what();
        return false;
    }
    d->transaction.reset();
    return true;
}

bool DatabaseImplPq::rollback()
{
    try{
        d->transaction->abort();
    }  catch (const pqxx::failure& e) {
        d->lastError = Error(e.what(), ESqlErrors::se_generic_sql_error);
        QLOG_ERROR() << e.what();
        return false;
    }
    d->transaction.reset();
    return true;
}

void DatabaseImplPq::close()
{
    try{
        d->transaction.reset();
        d->connection->close();
        d->connection.reset();
    }  catch (const pqxx::failure& e) {
        d->lastError = Error(e.what(), ESqlErrors::se_generic_sql_error);
        QLOG_ERROR() << e.what();
    }
}

std::string DatabaseImplPq::connectionName()
{
    return d->connectionName;
}

void *DatabaseImplPq::internalPointer()
{
    return static_cast<void*>(this);
}

bool DatabaseImplPq::isNull()
{
    return false;
}

std::string DatabaseImplPq::driverType() const
{
    return "PQXX";
}

std::shared_ptr<pqxx::work> DatabaseImplPq::getTransaction()
{
    return d->transaction;
}

std::shared_ptr<pqxx::connection> DatabaseImplPq::getConnection()
{
    return d->connection;
}

}
