#pragma once
#include <string>
#include <vector>
#include <QSqlRecord>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "sql_abstractions/sql_variant.h"
#include "sql_abstractions/sql_error.h"
#include "sql_abstractions/sql_query_impl_base.h"
namespace sql{

class QueryImplNull: public QueryImplBase{
public:
    QueryImplNull();

    bool prepare(const std::string &, const std::string &);
    bool exec();
    void setForwardOnly(bool);
    void bindVector(const std::vector<QueryBinding> &);
    void bindVector(std::vector<QueryBinding> &&);
    void bindValue(const std::string &, const Variant &);
    void bindValue(const std::string &, Variant &&);
    void bindValue(std::string &&, const Variant &);
    void bindValue(std::string &&, Variant &&);
    void bindValue(const QueryBinding &);
    void bindValue(QueryBinding &&);
    bool next();
    bool supportsVectorizedBind() const;
    Variant value(int) const;
    Variant value(const std::string &) const;
    Variant value(std::string &&) const;
    Variant value(const char *) const;
    //QSqlRecord record();
    Error lastError() const;
    std::string lastQuery() const;
    std::string implType() const;
};
}
