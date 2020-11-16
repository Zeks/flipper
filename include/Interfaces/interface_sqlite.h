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
    QStringList GetIdListForQuery(QSharedPointer<core::Query> query, QSqlDatabase db = QSqlDatabase());
    bool BackupDatabase(QString dbname);
    bool ReadDbFile(QString file, QString connectionName);

    QSqlDatabase InitDatabase(QString connectionName, bool setDefault = false);
    virtual QSqlDatabase InitDatabase2(QString fileName, QString connectionName, bool setDefault = false);


    QSqlDatabase InitAndUpdateDatabaseForFile(QString folder,
                                              QString file,
                                              QString sqlFile,
                                              QString connectionName,
                                              bool setDefault = false);

    bool PassScoresToAnotherDatabase(QSqlDatabase dbTarget);
    bool PassSnoozesToAnotherDatabase(QSqlDatabase dbTarget);
    bool PassFicTagsToAnotherDatabase(QSqlDatabase dbTarget);
    bool PassFicNotesToAnotherDatabase(QSqlDatabase dbTarget);
    bool PassTagSetToAnotherDatabase(QSqlDatabase dbTarget);
    bool PassRecentFandomsToAnotherDatabase(QSqlDatabase dbTarget);
    bool PassClientDataToAnotherDatabase(QSqlDatabase dbTarget);
    bool PassReadingDataToAnotherDatabase(QSqlDatabase dbTarget);
    bool PassIgnoredFandomsToAnotherDatabase(QSqlDatabase dbTarget);
    bool PassFandomListSetToAnotherDatabase(QSqlDatabase dbTarget);
    bool PassFandomListDataToAnotherDatabase(QSqlDatabase dbTarget);

    virtual QSqlDatabase InitNamedDatabase(QString dbName, QString fileName, bool setDefault = false);
    bool EnsureUUIDForUserDatabase();
    virtual QString GetUserToken();
};

}
