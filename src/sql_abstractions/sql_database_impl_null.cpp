#include "sql_abstractions/sql_database_impl_null.h"

namespace sql {

void DatabaseImplNull::setConnectionToken(ConnectionToken)
{
    // intentionally empty
}

bool DatabaseImplNull::open()
{
    return false;
}

bool DatabaseImplNull::isOpen() const
{
    return false;
}

bool DatabaseImplNull::transaction()
{
    return false;
}

bool DatabaseImplNull::commit()
{
    return false;
}

bool DatabaseImplNull::rollback()
{
    return false;
}

void DatabaseImplNull::close()
{
    // intentionally empty
}

std::string DatabaseImplNull::connectionName()
{
    return "";
}

void *DatabaseImplNull::internalPointer()
{
    return nullptr;
}

bool DatabaseImplNull::isNull()
{
    return true;
}

std::string DatabaseImplNull::driverType() const
{
    return "null";
}

}
