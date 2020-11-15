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
#include <QSqlDatabase>
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
    virtual QStringList GetIdListForQuery(QSharedPointer<core::Query> query, QSqlDatabase db = QSqlDatabase()) = 0;
    virtual bool BackupDatabase(QString dbname) = 0;
    virtual bool ReadDbFile(QString file, QString connectionName = QStringLiteral("")) = 0;
    virtual QSqlDatabase InitDatabase(QString connectionName, bool setDefault = false) = 0;
    virtual QSqlDatabase InitDatabase2(QString fileName, QString connectionName, bool setDefault = false) = 0;
    virtual QSqlDatabase InitNamedDatabase(QString dbName, QString fileName, bool setDefault = false) = 0;
    virtual QSqlDatabase InitAndUpdateDatabaseForFile(QString folder,
                                                      QString file,
                                                      QString sqlFile,
                                                      QString connectionName,
                                                      bool setDefault = false) = 0;
    virtual bool EnsureUUIDForUserDatabase() = 0;
    virtual QString GetUserToken() = 0;

    virtual bool PassScoresToAnotherDatabase(QSqlDatabase dbTarget) = 0;
    virtual bool PassSnoozesToAnotherDatabase(QSqlDatabase dbTarget) = 0;
    virtual bool PassFicTagsToAnotherDatabase(QSqlDatabase dbTarget) = 0;
    virtual bool PassFicNotesToAnotherDatabase(QSqlDatabase dbTarget) = 0;
    virtual bool PassTagSetToAnotherDatabase(QSqlDatabase dbTarget) = 0;
    virtual bool PassRecentFandomsToAnotherDatabase(QSqlDatabase dbTarget) = 0;
    virtual bool PassClientDataToAnotherDatabase(QSqlDatabase dbTarget) = 0;
    virtual bool PassReadingDataToAnotherDatabase(QSqlDatabase dbTarget) = 0;
    virtual bool PassIgnoredFandomsToAnotherDatabase(QSqlDatabase dbTarget) = 0;
    virtual bool PassFandomListSetToAnotherDatabase(QSqlDatabase dbTarget) = 0;
    virtual bool PassFandomListDataToAnotherDatabase(QSqlDatabase dbTarget) = 0;

    QSqlDatabase GetDatabase() {return db;}
    void SetDatabase(QSqlDatabase db) {this->db = db;}

    QString userToken;
protected:
    QSqlDatabase db;
};
}
