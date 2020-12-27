#include "sql_abstractions/sql_query.h"
#include "sql_abstractions/sql_query_impl_sqlite.h"
#include "sql_abstractions/sql_query_impl_pq.h"
#include "sql_abstractions/sql_query_impl_null.h"
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_error.h"

namespace sql {
static std::unordered_map<std::string, std::string> namedQueries;

Query::Query(Database db)
{
    instantiateImpl(db);
}

Query::Query(std::string query, Database db)
{
    instantiateImpl(db);
    prepare(query);
}

bool Query::prepare(const std::string & query)
{
    return d->prepare(query);
}

bool Query::prepare(std::string && query)
{
    return d->prepare(query);
}

bool Query::prepare(const std::string & name, const std::string & query)
{
    return d->prepare(name, query);
}

bool Query::exec()
{
    auto result = d->exec();
    //todo : probably shitcode
    if(namedQueries.find(d->queryName) == std::end(namedQueries))
        namedQueries[d->queryName] = lastQuery();
    return result;
}

void Query::setForwardOnly(bool value)
{
    d->setForwardOnly(value);
}

void Query::bindVector(const std::vector<QueryBinding> &value)
{
    d->bindVector(value);
}

void Query::bindVector(std::vector<QueryBinding> && value)
{
    d->bindVector(value);
}

void Query::bindValue(const std::string & name, const Variant & value)
{
    d->bindValue(name, value);
}

void Query::bindValue(const std::string & name, Variant && value)
{
    d->bindValue(name, value);
}

void Query::bindValue(std::string && name, const Variant& value)
{
    d->bindValue(name, value);
}

void Query::bindValue(std::string && name, Variant&& value)
{
    d->bindValue(name, value);
}

void Query::bindValue(const QueryBinding & binding)
{
    d->bindValue(binding);
}

void Query::bindValue(QueryBinding && binding)
{
    d->bindValue(binding);
}

bool Query::next()
{
    return d->next();
}

bool Query::supportsVectorizedBind() const
{
    return d->supportsVectorizedBind();
}

Variant Query::value(int index) const
{
    return d->value(index);
}

Variant Query::value(const std::string & name) const
{
    return d->value(name);
}

Variant Query::value(std::string && name) const
{
    return d->value(name);
}

Variant Query::value(const char * name) const
{
    return d->value(name);
}

//QSqlRecord Query::record()
//{
//    return d->record();
//}

Error Query::lastError() const
{
    return d->lastError();
}

std::string Query::lastQuery() const
{
    return d->lastQuery();
}

void Query::instantiateImpl(Database db)
{
    if(db.driverType() == "QSQLITE"){
        auto internals = *static_cast<QSqlDatabase*>(db.internalPointer());
        d.reset(new QueryImplSqlite(internals));
    }
    else if(db.driverType() == "PQXX"){
        auto internals = static_cast<DatabaseImplPq*>(db.internalPointer())->getShared();
        d.reset(new QueryImplPq(internals));
    }
    else
        d.reset(new QueryImplNull());

}

}
