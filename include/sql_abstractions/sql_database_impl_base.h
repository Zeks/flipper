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
