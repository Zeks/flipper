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
#pragma once
#include <string>
#include <vector>
#include <QSqlRecord>
#include "sql_abstractions/sql_variant.h"
#include "sql_abstractions/sql_error.h"
namespace sql{

class QueryImplBase{
public:
    virtual ~QueryImplBase(){};
    virtual bool prepare(const std::string&, const std::string& = "") = 0;
    virtual bool exec() = 0;
    virtual void setForwardOnly(bool) = 0;


    virtual void bindVector(const std::vector<QueryBinding>&) = 0;
    virtual void bindVector(std::vector<QueryBinding>&&) = 0;
    virtual void bindValue(const std::string&, const Variant&) = 0;
    virtual void bindValue(const std::string&, Variant&&) = 0;
    virtual void bindValue(std::string&&, const Variant&) = 0;
    virtual void bindValue(std::string&&, Variant&&) = 0;
    virtual void bindValue(const QueryBinding&) = 0;
    virtual void bindValue(QueryBinding&&) = 0;
    virtual bool next(bool warnOnEmpty = false) = 0;
    virtual int rowCount() const = 0;

    virtual bool supportsImmediateResultSize() const = 0;
    virtual bool supportsVectorizedBind() const = 0;
    virtual Variant value(int) const = 0;
    virtual Variant value(const std::string&)  const = 0;
    virtual Variant value(std::string&&)  const = 0;
    virtual Variant value(const char*) const = 0;
    //virtual QSqlRecord record() = 0;
    virtual Error lastError() const = 0;
    virtual std::string lastQuery() const = 0;
    virtual std::string implType() const = 0;
    std::string queryName;
    //static std::shared_ptr<std::unordered_map<std::string, std::string>> namedQueriesHolder;
    //std::shared_ptr<QueryImplBase> d;
};


}
