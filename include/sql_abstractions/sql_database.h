#pragma once
#include <string>
#include <memory>
namespace sql{
class DatabaseImpl;
class Database{
public:
    static Database addDatabase(std::string, std::string);
    void setDatabaseName();
    bool open();
    bool transaction();
    bool commit();
    bool rollback();

    std::unique_ptr<DatabaseImpl> d;
};

}
