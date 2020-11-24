#include "sql_abstractions/sql_query.h"
#include "sql_abstractions/sql_query_impl_base.h"
#include "sql_abstractions/sql_query_impl_sqlite.h"
#include "sql_abstractions/sql_error.h"
#include "sql_abstractions/sql_database.h"

namespace sql {

Query::Query(Database)
{

}

Query::Query(std::string, Database)
{

}

bool Query::prepare(const std::string &)
{

}

bool Query::prepare(std::string &&)
{

}

bool Query::exec()
{

}

void Query::setForwardOnly(bool)
{

}

void Query::bindVector(const std::vector<Variant> &)
{

}

void Query::bindValue(const std::string &, const Variant &)
{

}

void Query::bindValue(const std::string &, Variant &&)
{

}

void Query::bindValue(std::string &&, const Variant &)
{

}

void Query::bindValue(std::string &&, Variant &&)
{

}

void Query::bindValue(const QueryBinding &)
{

}

void Query::bindValue(QueryBinding &&)
{

}

bool Query::next()
{

}

bool Query::supportsVectorizedBind() const
{

}

Variant Query::value(int) const
{

}

Variant Query::value(const std::string &) const
{

}

Variant Query::value(std::string &&) const
{

}

Variant Query::value(const char *) const
{

}

QSqlRecord Query::record()
{

}

Error Query::lastError() const
{

}

std::string Query::lastQuery() const
{

}

}
