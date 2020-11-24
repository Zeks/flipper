#pragma once
#include "sql_abstractions/sql_variant.h"
#include <memory>
#include <vector>
#include <variant>
#include <type_traits>
#include <string>
#include <QDateTime>
#include <QSqlRecord>

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


    bool supportsVectorizedBind() const;
    Variant value(int) const;
    Variant value(const std::string&)  const;
    Variant value(std::string&&)  const;
    Variant value(const char*) const;
    QSqlRecord record();
    Error lastError() const;
    std::string lastQuery() const;

    std::shared_ptr<QueryImplBase> d;
private:
    void instantiateImpl(Database);
};

};


