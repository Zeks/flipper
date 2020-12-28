#pragma once
#include "sql_abstractions/sql_variant.h"
#include "sql_abstractions/sql_error.h"
#include "sql_abstractions/sql_query_impl_base.h"

#include <string>
namespace sql{

class QueryImplNull: public QueryImplBase{
public:
    QueryImplNull() = default;

    bool prepare(const std::string &, const std::string &) override;
    bool exec() override;
    void setForwardOnly(bool) override;
    void bindVector(const std::vector<QueryBinding> &) override;
    void bindVector(std::vector<QueryBinding> &&) override;
    void bindValue(const std::string &, const Variant &) override;
    void bindValue(const std::string &, Variant &&) override;
    void bindValue(std::string &&, const Variant &) override;
    void bindValue(std::string &&, Variant &&) override;
    void bindValue(const QueryBinding &) override;
    void bindValue(QueryBinding &&) override;
    bool next(bool warnOnEmpty) override;
    int rowCount() const override;
    virtual void setNamedQuery(std::string);
    bool supportsImmediateResultSize() const override;
    bool supportsVectorizedBind() const override;
    Variant value(int) const override;
    Variant value(const std::string &) const override;
    Variant value(std::string &&) const override;
    Variant value(const char *) const override;

    Error lastError() const override;
    std::string lastQuery() const override;
    std::string implType() const override;
};
}
