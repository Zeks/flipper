#pragma once
#include <string>
#include <memory>
#include "sql_abstractions/sql_connection_token.h"
namespace sql{
class DatabaseImplBase;
class Database{
public:
    Database();
    static Database addDatabase(std::string, std::string = "");
    static void removeDatabase(std::string = "");
    static Database database(std::string s = "");
    void setConnectionToken(ConnectionToken);
    std::string driverType() const;
    bool open();
    bool isOpen() const;
    bool transaction();
    bool commit();
    bool rollback();
    void* internalPointer();
    void close();
    std::string connectionName();
    std::shared_ptr<DatabaseImplBase> d;
};

}
