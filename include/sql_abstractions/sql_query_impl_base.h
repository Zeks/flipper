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
    virtual bool next() = 0;
    virtual void setNamedQuery(std::string) = 0;

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
    static std::shared_ptr<std::unordered_map<std::string, std::string>> namedQueriesHolder;
    //std::shared_ptr<QueryImplBase> d;
};


}
