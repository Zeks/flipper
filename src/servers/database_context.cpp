/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017  Marchenko Nikolai

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
#include "servers/database_context.h"
#include "Interfaces/db_interface.h"

#include "Interfaces/ffn/ffn_authors.h"
#include "Interfaces/ffn/ffn_fanfics.h"
#include "Interfaces/fandoms.h"
#include "sqlitefunctions.h"
static QString GetDbNameFromCurrentThread(){
    std::stringstream ss;
    ss << std::this_thread::get_id();
    std::string id = ss.str();
    return QString("Crawler_") + QString::fromStdString(id);
}

DatabaseContext::DatabaseContext(){
    QString name = GetDbNameFromCurrentThread();
    QLOG_TRACE() << "OPENING CONNECTION:" << name;
    db = database::sqlite::InitSqliteDatabase2("database/CrawlerDB", name, false);
}

template<typename Base, typename Derived>
QSharedPointer<Base> MakeAndAssignDB(sql::Database db){
    QSharedPointer<Base> entity {new Derived()};
    entity->db = db;
    return entity;
}

template<typename Type>
QSharedPointer<Type> MakeAndAssignDB(sql::Database db){
    QSharedPointer<Type> entity {new Type()};
    entity->db = db;
    return entity;
}

void DatabaseContext::InitFanfics()
{
    authors =  MakeAndAssignDB<interfaces::Authors, interfaces::FFNAuthors>(this->db);;
    fanfics =  MakeAndAssignDB<interfaces::Fanfics, interfaces::FFNFanfics>(this->db);;
    fandoms =  MakeAndAssignDB<interfaces::Fandoms>(this->db);

    fanfics->authorInterface = authors;
    fanfics->fandomInterface = fandoms;
}

void DatabaseContext::InitFandoms()
{
    fandoms =  QSharedPointer<interfaces::Fandoms> {new interfaces::Fandoms()};
    fandoms->db = this->db;
}

void DatabaseContext::InitAuthors()
{
    authors =  QSharedPointer<interfaces::Authors> {new interfaces::FFNAuthors()};
    authors->db = this->db;
}
