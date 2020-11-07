#pragma once
#include <memory>
#include <vector>
#include <variant>
namespace sql{

class Database;
class Error;
class QueryImpl;

class Query{
public:
    using Variant = std::variant<std::string, int32_t, int64_t, uint32_t, uint64_t>;
    Query(Database);
    bool prepare(const std::string&);
    bool prepare(std::string&&);
    bool exec();

    void bindVector(const std::vector<Variant>&);
    void bindValue(const std::string&, const Variant&);
    void bindValue(const std::string&, Variant&&);
    void bindValue(std::string&&, const Variant&);
    void bindValue(std::string&&, Variant&&);
    bool next();


    bool supportsVectorizedBind() const;
    Variant value(int) const;
    Variant value(const std::string&)  const;
    Variant value(std::string&&)  const;
    Variant value(const char*) const;
    Error lastError() const;
    std::string lastQuery() const;

    std::unique_ptr<QueryImpl> d;
};
struct QueryBinding{
    std::string key;
    Query::Variant value;
};

};
