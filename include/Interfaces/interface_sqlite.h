/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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
#include "Interfaces/db_interface.h"
#include <QSharedPointer>


namespace database{
class SqliteInterface : public IDBWrapper
{
public:
    SqliteInterface();
    ~SqliteInterface();
    int GetLastIdForTable(QString tableName);
    bool PushFandomToTopOfRecent(QString fandom);
    bool RebaseFandomsToZero();
    QStringList FetchRecentFandoms();
    QDateTime GetCurrentDateTime();
    QStringList GetIdListForQuery(QSharedPointer<core::Query> query, sql::Database db = sql::Database());
    bool BackupDatabase(QString dbname);
    bool ReadDbFile(QString file, QString connectionName);

    sql::Database InitDatabase(QString connectionName, bool setDefault = false);
    virtual sql::Database InitDatabase2(QString fileName, QString connectionName, bool setDefault = false);


    sql::Database InitAndUpdateDatabaseForFile(QString folder,
                                              QString file,
                                              QString sqlFile,
                                              QString connectionName,
                                              bool setDefault = false);

    bool PassScoresToAnotherDatabase(sql::Database dbTarget);
    bool PassSnoozesToAnotherDatabase(sql::Database dbTarget);
    bool PassFicTagsToAnotherDatabase(sql::Database dbTarget);
    bool PassFicNotesToAnotherDatabase(sql::Database dbTarget);
    bool PassTagSetToAnotherDatabase(sql::Database dbTarget);
    bool PassRecentFandomsToAnotherDatabase(sql::Database dbTarget);
    bool PassClientDataToAnotherDatabase(sql::Database dbTarget);
    bool PassReadingDataToAnotherDatabase(sql::Database dbTarget);
    bool PassIgnoredFandomsToAnotherDatabase(sql::Database dbTarget);
    bool PassFandomListSetToAnotherDatabase(sql::Database dbTarget);
    bool PassFandomListDataToAnotherDatabase(sql::Database dbTarget);

    virtual sql::Database InitNamedDatabase(QString dbName, QString fileName, bool setDefault = false);
    bool EnsureUUIDForUserDatabase();
    virtual QString GetUserToken();
};

}
