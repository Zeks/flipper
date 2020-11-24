#pragma once
#include <string>

namespace sql{
class DatabaseImplBase{
    public:
    DatabaseImplBase(){}
    virtual ~DatabaseImplBase(){};

    virtual void setDatabaseName(std::string) = 0;
    virtual bool open() = 0;
    virtual bool isOpen() const = 0;
    virtual bool transaction() = 0;
    virtual bool commit() = 0;
    virtual bool rollback() = 0;
    virtual void close() = 0;
    virtual std::string connectionName() = 0;
    virtual void* internalPointer() = 0;
    virtual bool isNull() = 0;
};
}
