#pragma once

#include "sql_abstractions/sql_variant.h"
#include "sql_abstractions/sql_error.h"
#include "sql_abstractions/sql_query_impl_base.h"
#include "sql_abstractions/sql_database_impl_pq.h"

#include <string>

namespace sql{
struct QueryImplPqImpl;
class QueryImplPq: public QueryImplBase{
public:
    QueryImplPq();
    QueryImplPq(std::shared_ptr<DatabaseImplPq>);
    QueryImplPq(const std::string& query, std::shared_ptr<DatabaseImplPq>);
    QueryImplPq(std::string&& query, std::shared_ptr<DatabaseImplPq>);

    bool prepare(const std::string &, const std::string &) override;
    bool exec()  override;
    void setForwardOnly(bool)  override;
    void bindVector(const std::vector<QueryBinding> &)  override;
    void bindVector(std::vector<QueryBinding> &&)  override;
    void bindValue(const std::string &, const Variant &)  override;
    void bindValue(const std::string &, Variant &&)  override;
    void bindValue(std::string &&, const Variant &)  override;
    void bindValue(std::string &&, Variant &&)  override;
    void bindValue(const QueryBinding &)  override;
    void bindValue(QueryBinding &&) override;
    bool next() override;
    bool supportsVectorizedBind() const override;
    Variant value(int) const  override;
    Variant value(const std::string &) const  override;
    Variant value(std::string &&) const  override ;
    Variant value(const char *) const  override;
    //QSqlRecord record()  override;
    Error lastError() const override;
    std::string lastQuery() const  override;
    std::string implType() const override;
    std::shared_ptr<QueryImplPqImpl> d;
private:
    // not in interface
    bool NeedsPreparing(const std::string& statement);
    void ExtractNamedPlaceholders(std::string_view);
    void ReplaceNamedPlaceholders(std::string &);
};
}
