#pragma once
#include "sql_abstractions/sql_database_impl_base.h"
#include "sql_abstractions/pqxx_connection_wrapper.h"
#include <pqxx/pqxx>
#include <memory>
namespace sql {
class DatabaseImplPqImpl;
class DatabaseImplPq : public DatabaseImplBase{
    public:
    DatabaseImplPq();
    void setConnectionToken(ConnectionToken) override;
    bool open()override;
    bool isOpen() const override;
    bool transaction() override;
    bool commit() override;
    bool rollback() override;
    void close() override;
    std::string connectionName() override;
    void *internalPointer() override;
    bool isNull() override;
    std::string driverType() const override;
    std::shared_ptr<DatabaseImplPqImpl> d;
    std::shared_ptr<pqxx::work> getTransaction();
    std::shared_ptr<PQXXConnectionWrapper> getConnection();
};

}
