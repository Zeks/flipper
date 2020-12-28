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
#include "sql_abstractions/sql_database_impl_base.h"
#include "sql_abstractions/shared_trick.h"
#include <pqxx/pqxx>
#include <memory>
namespace sql {
class DatabaseImplPqImpl;

class DatabaseImplPq : public DatabaseImplBase, public inheritable_enable_shared_from_this<DatabaseImplPq>{
    public:
    DatabaseImplPq(std::string connectionName = "");
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
    std::shared_ptr<DatabaseImplPqImpl> d;
    std::shared_ptr<pqxx::work> getTransaction();
    std::shared_ptr<pqxx::connection> getConnection();
    std::shared_ptr<DatabaseImplPq> getShared(){return shared_from_this();}

};

}
