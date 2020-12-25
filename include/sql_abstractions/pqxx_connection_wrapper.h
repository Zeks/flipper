#pragma once
#include <pqxx/pqxx>
#include <memory>
namespace sql{
    struct PQXXConnectionWrapper : std::enable_shared_from_this<PQXXConnectionWrapper>{
        pqxx::connection wrapped;
    }
}
