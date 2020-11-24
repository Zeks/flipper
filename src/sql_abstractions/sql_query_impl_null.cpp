#include "sql_abstractions/sql_query_impl_null.h"
namespace sql {
QueryImplNull::QueryImplNull()
{

}


bool QueryImplNull::prepare(const std::string & )
{
    throw std::logic_error("use of nulld sql driver");
    return false;
}

bool QueryImplNull::prepare(std::string && )
{
    throw std::logic_error("use of nulld sql driver");
    return false;
}

bool QueryImplNull::exec()
{
    throw std::logic_error("use of nulld sql driver");
    return false;
}

void QueryImplNull::setForwardOnly(bool )
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplNull::bindVector(const std::vector<QueryBinding> &)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplNull::bindVector(std::vector<QueryBinding> &&)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplNull::bindValue(const std::string &, const Variant &)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplNull::bindValue(const std::string &, Variant &&)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplNull::bindValue(std::string &&, const Variant &)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplNull::bindValue(std::string &&, Variant &&)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplNull::bindValue(const QueryBinding &)
{
    throw std::logic_error("use of nulld sql driver");
}

void QueryImplNull::bindValue(QueryBinding &&)
{
    throw std::logic_error("use of nulld sql driver");
}

bool QueryImplNull::next()
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

bool QueryImplNull::supportsVectorizedBind() const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

Variant QueryImplNull::value(int) const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

Variant QueryImplNull::value(const std::string &) const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

Variant QueryImplNull::value(std::string &&) const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

Variant QueryImplNull::value(const char *) const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

QSqlRecord QueryImplNull::record()
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

Error QueryImplNull::lastError() const
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

std::string QueryImplNull::lastQuery() const
{
    throw std::logic_error("use of nulld sql driver");
}

std::string QueryImplNull::implType() const
{
    return "null";
}

}
