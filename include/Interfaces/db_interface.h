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
#include "sql_abstractions/sql_database.h"
#include <QString>
#include <QDateTime>
#include <QSharedPointer>
#include "pure_sql.h"
namespace core { struct Query;}
namespace database{

class IDBWrapper{
public:
    virtual ~IDBWrapper() = default;
    virtual int GetLastIdForTable(QString tableName) = 0;
    virtual bool PushFandomToTopOfRecent(QString fandom) = 0;
    virtual bool RebaseFandomsToZero() = 0;
    virtual QStringList FetchRecentFandoms() = 0;
    virtual QDateTime GetCurrentDateTime() = 0;
    virtual QStringList GetIdListForQuery(QSharedPointer<core::Query> query, sql::Database db = sql::Database()) = 0;
    virtual bool BackupDatabase(QString dbname) = 0;
    virtual bool ReadDbFile(QString file, QString connectionName = QStringLiteral("")) = 0;
    virtual sql::Database InitDatabase(QString connectionName, bool setDefault = false) = 0;
    virtual sql::Database InitDatabase2(QString fileName, QString connectionName, bool setDefault = false) = 0;
    virtual sql::Database InitNamedDatabase(QString dbName, QString fileName, bool setDefault = false) = 0;
    virtual sql::Database InitAndUpdateDatabaseForFile(QString folder,
                                                      QString file,
                                                      QString sqlFile,
                                                      QString connectionName,
                                                      bool setDefault = false) = 0;
    virtual bool EnsureUUIDForUserDatabase() = 0;
    virtual QString GetUserToken() = 0;

    virtual bool PassScoresToAnotherDatabase(sql::Database dbTarget) = 0;
    virtual bool PassSnoozesToAnotherDatabase(sql::Database dbTarget) = 0;
    virtual bool PassFicTagsToAnotherDatabase(sql::Database dbTarget) = 0;
    virtual bool PassFicNotesToAnotherDatabase(sql::Database dbTarget) = 0;
    virtual bool PassTagSetToAnotherDatabase(sql::Database dbTarget) = 0;
    virtual bool PassRecentFandomsToAnotherDatabase(sql::Database dbTarget) = 0;
    virtual bool PassClientDataToAnotherDatabase(sql::Database dbTarget) = 0;
    virtual bool PassReadingDataToAnotherDatabase(sql::Database dbTarget) = 0;
    virtual bool PassIgnoredFandomsToAnotherDatabase(sql::Database dbTarget) = 0;
    virtual bool PassFandomListSetToAnotherDatabase(sql::Database dbTarget) = 0;
    virtual bool PassFandomListDataToAnotherDatabase(sql::Database dbTarget) = 0;

    sql::Database GetDatabase() {return db;}
    void SetDatabase(sql::Database db) {this->db = db;}

    QString userToken;
protected:
    sql::Database db;
};
}
