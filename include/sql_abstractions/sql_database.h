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
    bool hasOpenTransaction() const;
    void* internalPointer();
    void close();
    std::string connectionName();
    std::shared_ptr<DatabaseImplBase> d;
};

}
