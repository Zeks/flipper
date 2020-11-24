#include "sql_abstractions/sql_query_impl_sqlite.h"
namespace sql {
QueryImplSqlite::QueryImplSqlite()
{

}

QueryImplSqlite::QueryImplSqlite(QSqlDatabase db):q(db)
{

}

bool QueryImplSqlite::prepare(const std::string & sql)
{
    return q.prepare(QString::fromStdString(sql));
}

bool QueryImplSqlite::prepare(std::string && sql)
{
    return q.prepare(QString::fromStdString(sql));
}

bool QueryImplSqlite::exec()
{
    return q.exec();
}

void QueryImplSqlite::setForwardOnly(bool value)
{
    q.setForwardOnly(value);
}

void QueryImplSqlite::bindVector(const std::vector<QueryBinding> &)
{

}

void QueryImplSqlite::bindVector(std::vector<QueryBinding> &&)
{

}

void QueryImplSqlite::bindValue(const std::string &, const Variant &)
{

}

void QueryImplSqlite::bindValue(const std::string &, Variant &&)
{

}

void QueryImplSqlite::bindValue(std::string &&, const Variant &)
{

}

void QueryImplSqlite::bindValue(std::string &&, Variant &&)
{

}

void QueryImplSqlite::bindValue(const QueryBinding &)
{

}

void QueryImplSqlite::bindValue(QueryBinding &&)
{

}

bool QueryImplSqlite::next()
{

}

bool QueryImplSqlite::supportsVectorizedBind() const
{

}

Variant QueryImplSqlite::value(int) const
{

}

Variant QueryImplSqlite::value(const std::string &) const
{

}

Variant QueryImplSqlite::value(std::string &&) const
{

}

Variant QueryImplSqlite::value(const char *) const
{

}

QSqlRecord QueryImplSqlite::record()
{

}

Error QueryImplSqlite::lastError() const
{

}

std::string QueryImplSqlite::lastQuery() const
{

}

}
