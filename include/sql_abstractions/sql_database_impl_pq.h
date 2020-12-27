#pragma once
#include "sql_abstractions/sql_database_impl_base.h"
#include "sql_abstractions/shared_trick.h"
#include <pqxx/pqxx>
#include <memory>
namespace sql {
class DatabaseImplPqImpl;

class DatabaseImplPq : public DatabaseImplBase, public inheritable_enable_shared_from_this<DatabaseImplPq>{
    public:
    DatabaseImplPq(std::string connectionName = "");
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
    std::shared_ptr<pqxx::connection> getConnection();
    std::shared_ptr<DatabaseImplPq> getShared(){return shared_from_this();}

};

}
