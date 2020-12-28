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
#include "sql_abstractions/sql_query_impl_null.h"
namespace sql {

bool QueryImplNull::prepare(const std::string &, const std::string & )
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

bool QueryImplNull::next(bool)
{
    throw std::logic_error("use of nulld sql driver");
    return {};
}

int QueryImplNull::rowCount() const
{
    throw std::logic_error("use of nulld sql driver");
    return 0;
}

void QueryImplNull::setNamedQuery(std::string)
{
    throw std::logic_error("use of nulld sql driver");
}

bool QueryImplNull::supportsImmediateResultSize() const
{
    throw std::logic_error("use of nulld sql driver");
    return false;
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

//QSqlRecord QueryImplNull::record()
//{
//    throw std::logic_error("use of nulld sql driver");
//    return {};
//}

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
