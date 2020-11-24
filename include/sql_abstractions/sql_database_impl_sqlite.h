#pragma once
#include <string>
#include <QSqlDatabase>
#include "sql_abstractions/sql_database_impl_base.h"

namespace sql{
class DatabaseImplSqlite : public DatabaseImplBase{
    public:
    DatabaseImplSqlite():DatabaseImplBase(){}
    static QSqlDatabase addDatabase(std::string, std::string = "");
    static QSqlDatabase database(std::string s = "");
    void setDatabaseName(std::string) override;
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
    QSqlDatabase db;
private:


};
}
