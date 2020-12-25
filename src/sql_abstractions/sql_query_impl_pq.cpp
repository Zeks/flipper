#include "sql_abstractions/sql_query_impl_pq.h"
#include "sql_abstractions/sql_database_impl_pq.h"
#include <pqxx/pqxx>
#include "fmt/format.h"
#include "logger/QsLog.h"
#include <QUuid>

namespace sql {
struct QueryImplPqImpl{
    std::shared_ptr<DatabaseImplPq> database;
    std::string preparedStatementId;
    bool namedStatement;
    std::string delayedStatement;
    std::optional<std::string> uniqueQueryIdentifier;
    std::vector<QueryBinding> bindings;
    pqxx::result resultSet;
};
QueryImplPq::QueryImplPq() : d(new QueryImplPqImpl())
{

}

QueryImplPq::QueryImplPq(std::shared_ptr<DatabaseImplPq> db) : QueryImplPq()
{
    d->database = db;
}

bool QueryImplPq::NeedsPreparing(const std::string& statement){
    if(statement.find("$1") == std::string::npos)
    {
        // we're not supposed to prepare this
        d->delayedStatement = statement;
        return false;
    }
    return true;
}

bool QueryImplPq::prepare(const std::string & name, const std::string & statement)
{
    if(!NeedsPreparing(statement))
        return true;
    try {
        d->uniqueQueryIdentifier = name;

        d->database->getConnection()->wrapped.prepare(*d->uniqueQueryIdentifier, statement);
    }  catch (const std::exception& e) {
        QLOG_ERROR() << e.what();
        return false;
    }
    return true;
}


std::string VariantToString(const Variant& v){
    auto indexOfValue = v.data.index();
    switch(indexOfValue){

    default:
        throw std::logic_error("all indexes should be handled in pg driver: " + std::to_string(indexOfValue));
    }

}


bool QueryImplPq::exec()
{

    if(!d->database->transaction())
        return false;

    auto transaction = d->database->getTransaction();

    if(!d->delayedStatement.empty()){
        try{
            d->resultSet = transaction->exec(d->delayedStatement);
        }
        catch (const std::exception& e) {
            QLOG_ERROR() << e.what();
            return false;
        }
    }
    else{


        try{
            auto dynamicParams = pqxx::prepare::make_dynamic_params(d->bindings, [](const auto& v){
                return VariantToString(v.value);
            });
            d->resultSet = transaction->exec_prepared(d->delayedStatement, dynamicParams);
        }
        catch (const std::exception& e) {
            QLOG_ERROR() << e.what();
            return false;
        }

    }
    return false;
}

void QueryImplPq::setForwardOnly(bool )
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplPq::bindVector(const std::vector<QueryBinding> &)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplPq::bindVector(std::vector<QueryBinding> &&)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplPq::bindValue(const std::string &, const Variant &)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplPq::bindValue(const std::string &, Variant &&)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplPq::bindValue(std::string &&, const Variant &)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplPq::bindValue(std::string &&, Variant &&)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplPq::bindValue(const QueryBinding &)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplPq::bindValue(QueryBinding &&)
{
    throw std::logic_error("use of nulld sql driver");
}

bool QueryImplPq::next()
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

bool QueryImplPq::supportsVectorizedBind() const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

Variant QueryImplPq::value(int) const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

Variant QueryImplPq::value(const std::string &) const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

Variant QueryImplPq::value(std::string &&) const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

Variant QueryImplPq::value(const char *) const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

QSqlRecord QueryImplPq::record()
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

Error QueryImplPq::lastError() const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

std::string QueryImplPq::lastQuery() const
{
    throw std::logic_error("use of nulld sql driver");
}

std::string QueryImplPq::implType() const
{
    return "null";
}

}
