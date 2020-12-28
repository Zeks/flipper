/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
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

bool DatabaseImplNull::hasOpenTransaction() const
{
    return false;
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
