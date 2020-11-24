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
#include "sql_abstractions/sql_database.h"
#include "pure_sql.h"
namespace dbfix{
    //void EnsureFandomIndexExists(sql::Database db);
    void FillFFNId(sql::Database db);
    void ReplaceUrlInLinkedAuthorsWithID(sql::Database db);
    bool RebindDuplicateFandoms(sql::Database db);
    void TrimUserUrls(sql::Database db);
    sql::DiagnosticSQLResult<bool> PassSlashDataIntoNewTable(sql::Database db);
    //void SetLastFandomUpdateTo
}
