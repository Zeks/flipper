/*
FFSSE is a replacement search engine for fanfiction.net search results
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
#include "sqlitefunctions.h"

#include "pure_sql.h"
#include <QSqlError>
#include <QSqlDriver>
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <quazip/quazip.h>
#include <quazip/JlCompress.h>
#include "include/queryinterfaces.h"
#include "pure_sql.h"

namespace database{

namespace sqlite {
int GetLastIdForTable(QString tableName, QSqlDatabase db)
{
    QString qs = QString("select seq from sqlite_sequence where name=:table_name");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":table_name", tableName);
    if(!database::puresql::ExecAndCheck(q))
        return -1;
    q.next();
    return q.value("seq").toInt();
}

void cfRegexp(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    QRegExp regex;
    QString str1((const char*)sqlite3_value_text(argv[0]));
    QString str2((const char*)sqlite3_value_text(argv[1]));

    regex.setPattern(str1);
    regex.setCaseSensitivity(Qt::CaseInsensitive);

    bool b = str2.contains(regex);

    if (b)
    {
        sqlite3_result_int(ctx, 1);
    }
    else
    {
        sqlite3_result_int(ctx, 0);
    }
}
void cfReturnCapture(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    QString pattern((const char*)sqlite3_value_text(argv[0]));
    QString str((const char*)sqlite3_value_text(argv[1]));
    QRegExp regex(pattern);
    regex.indexIn(str);
    QString cap = regex.cap(1);
    sqlite3_result_text(ctx, qPrintable(cap), cap.length(), SQLITE_TRANSIENT);
}
void cfGetFirstFandom(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    QRegExp regex("&");
    QString result;
    QString str((const char*)sqlite3_value_text(argv[0]));
    int index = regex.indexIn(str);
    if(index == -1)
        result = str;
    else
        result = str.left(index);
    sqlite3_result_text(ctx, qPrintable(result), result.length(), SQLITE_TRANSIENT);
}
void cfGetSecondFandom(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    QRegExp regex("&");
    QString result;
    QString str((const char*)sqlite3_value_text(argv[0]));
    int index = regex.indexIn(str);
    if(index == -1)
        result = "";
    else
        result = str.mid(index+1);
    result = result.replace(" CROSSOVER", "");
    result = result.replace("CROSSOVER", "");
    sqlite3_result_text(ctx, qPrintable(result), result.length(), SQLITE_TRANSIENT);
}

bool InstallCustomFunctions(QSqlDatabase db)
{
    QVariant v = db.driver()->handle();
    if (v.isValid() && qstrcmp(v.typeName(), "sqlite3*")==0)
    {
        sqlite3 *db_handle = *static_cast<sqlite3 **>(v.data());
        if (db_handle != 0) {
            sqlite3_initialize();
            sqlite3_create_function(db_handle, "cfRegexp", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &cfRegexp, NULL, NULL);
            sqlite3_create_function(db_handle, "cfGetFirstFandom", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &cfGetFirstFandom, NULL, NULL);
            sqlite3_create_function(db_handle, "cfGetSecondFandom", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &cfGetSecondFandom, NULL, NULL);
            sqlite3_create_function(db_handle, "cfReturnCapture", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &cfReturnCapture, NULL, NULL);
            return true;
        }
    }
    return false;
}

bool ReadDbFile(QString file, QString connectionName)
{
    QFile data(file);

    qDebug() << "Reading init file: " << file;
    if (data.open(QFile::ReadOnly))
    {
        QTextStream in(&data);
        QString dbCode = in.readAll();
        QStringList statements = dbCode.split(";");
        for(QString statement: statements)
        {
            statement = statement.replace(QRegExp("\\t"), " ");
            statement = statement.replace(QRegExp("\\r"), " ");
            statement = statement.replace(QRegExp("\\n"), " ");

            if(statement.trimmed().isEmpty() || statement.trimmed().left(2) == "--")
                continue;
            QSqlDatabase db;
            if(!connectionName.isEmpty())
                db = QSqlDatabase::database(connectionName);
            else
                db = QSqlDatabase::database();
            //bool isOpen = db.isOpen();
            QSqlQuery q(db);
            q.prepare(statement.trimmed());
            database::puresql::ExecAndCheck(q);
        }
    }
    else
        return false;
    return true;
}

QStringList GetIdListForQuery(QSharedPointer<core::Query> query, QSqlDatabase db)
{
    QString where = query->str;
    QStringList result;
    QString qs = QString("select group_concat(id, ',') as merged, " + where);
    //qs= qs.arg(where);
    qDebug() << qs;
    QSqlQuery q(db);
    q.prepare(qs);
    auto it = query->bindings.begin();
    auto end = query->bindings.end();
    while(it != end)
    {
        qDebug() << it.key() << " " << it.value();
        q.bindValue(it.key(), it.value());
        ++it;
    }
    if(!database::puresql::ExecAndCheck(q))
        return result;

    q.next();
    auto temp = q.value("merged").toString();
    if(temp.trimmed().isEmpty())
        return result;
    result = temp.split(",");
    return result;
}


bool BackupSqliteDatabase(QString dbName)
{
    bool success = true;
    QDir dir("backups");
    QStringList filters;
    filters << "*.zip";
    dir.setNameFilters(filters);
    auto entries = dir.entryList(QDir::NoFilter, QDir::Time|QDir::Reversed);
    qSort(entries.begin(),entries.end());
    std::reverse(entries.begin(),entries.end());
    qDebug() << entries;
    QString nameWithExtension = dbName + ".sqlite";
    if(!QFile::exists(nameWithExtension))
        success = success && QFile::copy(nameWithExtension+ ".default", nameWithExtension);
    QString backupName = "backups/" + dbName + "." + QDateTime::currentDateTime().date().toString("yyyy-MM-dd") + ".sqlite.zip";
    if(!QFile::exists(backupName))
        success = success && JlCompress::compressFile(backupName, "CrawlerDB.sqlite");

    int i = 0;
    for(QString entry : entries)
    {
        i++;
        if(i < 10)
            continue;
        success = success && QFile::remove("backups/" + entry);
    };
    return success;
}

bool PushFandomToTopOfRecent(QString fandom, QSqlDatabase db)
{
    QString upsert1 ("UPDATE recent_fandoms SET seq_num= (select max(seq_num) +1 from  recent_fandoms ) WHERE fandom = '%1'; ");
    QString upsert2 ("INSERT INTO recent_fandoms(fandom, seq_num) select '%1',  (select max(seq_num)+1 from recent_fandoms) WHERE changes() = 0;");
    QSqlQuery q1(db);
    upsert1 = upsert1.arg(fandom);
    q1.prepare(upsert1);
    if(!database::puresql::ExecAndCheck(q1))
        return false;
    upsert2 = upsert2.arg(fandom);
    q1.prepare(upsert2);
    if(!database::puresql::ExecAndCheck(q1))
        return false;
    return true;
}

QStringList FetchRecentFandoms(QSqlDatabase db)
{
    QSqlQuery q1(db);
    QString qsl = "select fandom from recent_fandoms where fandom is not 'base' order by seq_num desc";
    q1.prepare(qsl);
    q1.exec();
    QStringList result;
    while(q1.next())
    {
        result.push_back(q1.value(0).toString());
    }
    database::puresql::CheckExecution(q1);
    return result;
}

bool RebaseFandomsToZero(QSqlDatabase db)
{
    QSqlQuery q1(db);
    QString qsl = "UPDATE recent_fandoms SET seq_num = seq_num - (select min(seq_num) from recent_fandoms where fandom is not 'base') where fandom is not 'base'";
    q1.prepare(qsl);
    if(!database::puresql::ExecAndCheck(q1))
        return false;
    return true;
}

QDateTime GetCurrentDateTime(QSqlDatabase db)
{
    QDateTime dt;
    QSqlQuery q(db);
    QString qsl = "select CURRENT_TIMESTAMP";
    q.prepare(qsl);
    if(!database::puresql::ExecAndCheck(q))
        return dt;
    dt = q.value(0).toDateTime();
    return dt;
}

QSqlDatabase InitDatabase(QString name, bool setDefault)
{
    QString path = name;
    QSqlDatabase db;
    if(setDefault)
        db = QSqlDatabase::addDatabase("QSQLITE");
    else
        db = QSqlDatabase::addDatabase("QSQLITE", name);
    db.setDatabaseName(path + ".sqlite");
    bool isOpen = db.open();
    qDebug() << "Database status: " << name << ", open : " << isOpen;
    InstallCustomFunctions(db);

    return db;
}




}
}


