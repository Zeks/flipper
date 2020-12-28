#pragma once
#include "sql_abstractions/sql_variant.h"
#include <memory>
#include <string>

namespace sql{

class Database;
class Error;
class QueryImplBase;


class Query{
public:

    Query(Database);
    Query(std::string, Database);
    bool prepare(const std::string&);
    bool prepare(std::string&&);
    bool prepare(const std::string&, const std::string&);
    bool exec();
    void setForwardOnly(bool);

    void bindVector(const std::vector<QueryBinding> &);
    void bindVector(std::vector<QueryBinding> &&);
    void bindValue(const std::string&, const Variant&);
    void bindValue(const std::string&, Variant&&);
    void bindValue(std::string&&, const Variant&);
    void bindValue(std::string&&, Variant&&);
    void bindValue(const QueryBinding&);
    void bindValue(QueryBinding&&);
    bool next();
    int rowCount() const;
    bool supportsImmediateResultSize() const;
    bool supportsVectorizedBind() const;
    Variant value(int) const;
    Variant value(const std::string&)  const;
    Variant value(std::string&&)  const;
    Variant value(const char*) const;
    Error lastError() const;
    std::string lastQuery() const;

private:
    void instantiateImpl(Database);
    std::shared_ptr<QueryImplBase> d;
};

};


