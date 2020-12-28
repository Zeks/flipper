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
#include <QSqlDatabase>
#include "sql_abstractions/sql_database_impl_base.h"


namespace sql{
class DatabaseImplSqlite : public DatabaseImplBase{
    public:
    DatabaseImplSqlite():DatabaseImplBase(){}
    static QSqlDatabase addDatabase(std::string, std::string = "");
    static QSqlDatabase database(std::string s = "");
    void setConnectionToken(ConnectionToken) override;
    bool open()override;
    bool isOpen() const override;
    bool transaction() override;
    bool hasOpenTransaction() const override;
    bool commit() override;
    bool rollback() override;
    void close() override;
    std::string connectionName() override;
    void *internalPointer() override;
    bool isNull() override;
    std::string driverType() const override;
    QSqlDatabase db;
private:
    bool _hasOpenTransaction = false;


};
}
