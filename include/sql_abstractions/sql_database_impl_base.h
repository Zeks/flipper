#pragma once
#include <string>
#include "sql_abstractions/sql_connection_token.h"

namespace sql{
class DatabaseImplBase{
    public:
    DatabaseImplBase(){}
    virtual ~DatabaseImplBase(){};
    virtual void setConnectionToken(ConnectionToken) = 0;
    virtual bool open() = 0;
    virtual bool isOpen() const = 0;
    virtual bool hasOpenTransaction() const = 0;
    virtual bool transaction() = 0;
    virtual bool commit() = 0;
    virtual bool rollback() = 0;
    virtual void close() = 0;
    virtual std::string connectionName() = 0;
    virtual void* internalPointer() = 0;
    virtual bool isNull() = 0;
    virtual std::string driverType() const = 0;
};
}
