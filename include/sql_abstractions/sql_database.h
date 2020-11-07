#pragma once
#include <string>
#include <memory>
namespace sql{
class DatabaseImpl;
class Database{
public:
    static Database addDatabase(std::string, std::string = "");
    static Database database(std::string s = "");
    void setDatabaseName(std::string);
    bool open();
    bool isOpen() const;
    bool transaction();
    bool commit();
    bool rollback();
    void* internalPointer();
    bool close();
    std::string connectionName();

    std::shared_ptr<DatabaseImpl> d;
};

}
