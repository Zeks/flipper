#include "sql_abstractions/sql_database_impl_pq.h"
#include "sql_abstractions/pqxx_connection_wrapper.h"

#include "fmt/format.h"
#include "logger/QsLog.h"
#include <optional>
#include <iostream>

namespace sql {
class DatabaseImplPqImpl:std::enable_shared_from_this<DatabaseImplPqImpl>{
public:
    std::shared_ptr<PQXXConnectionWrapper> connection;
    std::string connectionName;
    ConnectionToken token;
    std::shared_ptr<pqxx::work> transaction;
};

DatabaseImplPq::DatabaseImplPq()
{
    d.reset(new DatabaseImplPqImpl());
    d->connection.reset(new PQXXConnectionWrapper());
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
        d->connection->wrapped = pqxx::connection(uri);
    }  catch (const std::exception& e) {
        QLOG_ERROR() << e.what();
        return false;
    }
    return d->connection->wrapped.is_open();
}

bool DatabaseImplPq::isOpen() const
{
    try {
        return d->connection->wrapped.is_open();
    }  catch (const std::exception& e) {
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
        d->transaction.reset(new pqxx::work(d->connection->wrapped));
    }  catch (const std::exception& e) {
        QLOG_ERROR() << e.what();
        return false;
    }
    return true;
}

bool DatabaseImplPq::commit()
{
    try{
    d->transaction->commit();
    }  catch (const std::exception& e) {
        QLOG_ERROR() << e.what();
        return false;
    }
    return false;
}

bool DatabaseImplPq::rollback()
{
    try{
        d->transaction->abort();
    }  catch (const std::exception& e) {
        QLOG_ERROR() << e.what();
        return false;
    }
    return false;
}

void DatabaseImplPq::close()
{
    try{
    d->connection->wrapped.close();
    }  catch (const std::exception& e) {
        QLOG_ERROR() << e.what();
    }
}

std::string DatabaseImplPq::connectionName()
{
    return d->connectionName;
}

void *DatabaseImplPq::internalPointer()
{
    return static_cast<void*>(d.get());
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

std::shared_ptr<PQXXConnectionWrapper> DatabaseImplPq::getConnection()
{
    return d->connection;
}

}
