#pragma once
#include <string>
#include <memory>
namespace sql{
class DatabaseImplBase;
class Database{
public:
    Database();
    static Database addDatabase(std::string, std::string = "");
    static Database database(std::string s = "");
    void setDatabaseName(std::string);
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
