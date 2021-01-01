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
#include "sqlitefunctions.h"

#include "pure_sql.h"
#include <QSqlError>
#include <QSqlDriver>
#include <QFile>
#include <QDebug>
#include <QUuid>
#include <QSettings>
#include <QTextStream>
#include <QCoreApplication>
//#include <third_party/quazip/quazip.h>
//#include <third_party/quazip/JlCompress.h>
#include "include/queryinterfaces.h"
#include "include/transaction.h"
#include "include/in_tag_accessor.h"
#include "pure_sql.h"
#include "logger/QsLog.h"
#include "GlobalHeaders/snippets_templates.h"
#include <QSqlDatabase>

namespace database{

namespace sqlite {
int GetLastIdForTable(QString tableName, sql::Database db)
{
    QString qs = QString("select seq from sqlite_sequence where name=:table_name");
    sql::Query q(db);
    q.prepare(qs.toStdString());
    q.bindValue("table_name", tableName);
    if(!sql::ExecAndCheck(q))
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

void cfInTags(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int ficId = sqlite3_value_int(argv[0]);
    //QLOG_INFO() << "accessing info for fic: " << ficId<< " user: " << userToken;
    auto* data = ThreadData::GetUserData();

    if(data->allTaggedFics.contains(ficId))
        sqlite3_result_int(ctx, 1);
    else
        sqlite3_result_int(ctx, 0);
}


void cfInFicsForAuthors(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int ficId = sqlite3_value_int(argv[0]);
    //QLOG_INFO() << "accessing info for fic: " << ficId<< " user: " << userToken;
    auto* data = ThreadData::GetUserData();

    if(data->ficsForAuthorSearch.contains(ficId))
        sqlite3_result_int(ctx, 1);
    else
        sqlite3_result_int(ctx, 0);
}

void cfInSnoozes(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int ficId = sqlite3_value_int(argv[0]);
    //QLOG_INFO() << "accessing info for fic: " << ficId<< " user: " << userToken;
    auto* data = ThreadData::GetUserData();

    if(data->allSnoozedFics.contains(ficId))
        sqlite3_result_int(ctx, 1);
    else
        sqlite3_result_int(ctx, 0);
}


void cfInRecommendations(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int ficId = sqlite3_value_int(argv[0]);
    auto* data= ThreadData::GetRecommendationData();

    if(data->ficMetascores.find(ficId) != data->ficMetascores.end())
    {
        //qDebug() << "fic in data: " << ficId;
        sqlite3_result_int(ctx, 1);
    }
    else
        sqlite3_result_int(ctx, 0);
}

void cfInScores(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int ficId = sqlite3_value_int(argv[0]);
    auto* data= ThreadData::GetRecommendationData();

    if(data->scoresList.contains(ficId))
    {
        //qDebug() << "fic in data: " << ficId;
        sqlite3_result_int(ctx, 1);
    }
    else
        sqlite3_result_int(ctx, 0);
}


void cfInSourceFics(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int ficId = sqlite3_value_int(argv[0]);
    //QLOG_INFO() << "accessing info for fic: " << ficId<< " user: " << userToken;
    auto* data= ThreadData::GetRecommendationData();
    if(!data)
    {
        sqlite3_result_int(ctx, 0);
        return;
    }
    auto& sources = data->sourceFics;
    if(sources.contains(ficId))
        sqlite3_result_int(ctx, 1);
    else
        sqlite3_result_int(ctx, 0);
}

void cfRecommendationsMetascore(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int ficId = sqlite3_value_int(argv[0]);
    auto* data= ThreadData::GetRecommendationData();
    if(!data)
    {
        sqlite3_result_int(ctx, 0);
        return;
    }
    auto& hash = data->ficMetascores;
    auto it = hash.find(ficId);
    if(it == hash.end())
        sqlite3_result_int(ctx, 0);
    else
        sqlite3_result_int(ctx, it->second);
}

void cfRecommendationsPureVotes(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int ficId = sqlite3_value_int(argv[0]);
    auto* data= ThreadData::GetRecommendationData();
    if(!data)
    {
        sqlite3_result_int(ctx, 0);
        return;
    }
    auto& hash = data->ficVotes;
    auto it = hash.find(ficId);
    if(it == hash.end())
        sqlite3_result_int(ctx, 0);
    else
        sqlite3_result_int(ctx, it->second);
}

void cfScoresMatchCount(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int ficId = sqlite3_value_int(argv[0]);
    //QLOG_INFO() << "accessing info for fic: " << ficId<< " user: " << userToken;
    auto* data= ThreadData::GetRecommendationData();
    if(!data)
    {
        sqlite3_result_int(ctx, 0);
        return;
    }
    auto& hash = data->scoresList;
    QHash<int, int>::iterator it = hash.find(ficId);
    if(it == hash.end())
        sqlite3_result_int(ctx, 0);
    else
        sqlite3_result_int(ctx, it.value());
}


void cfInAuthors(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int authorId = sqlite3_value_int(argv[0]);
    auto* data= ThreadData::GetRecommendationData();
    if(!data)
    {
        sqlite3_result_int(ctx, 0);
        return;
    }
    if(data->matchedAuthors.contains(authorId))
        sqlite3_result_int(ctx, 1);
    else
        sqlite3_result_int(ctx, 0);
}

void cfInLikedAuthors(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int authorId = sqlite3_value_int(argv[0]);
    auto* data= ThreadData::GetUserData();
    if(!data)
    {
        sqlite3_result_int(ctx, 0);
        return;
    }
    if(data->usedAuthors.contains(authorId))
        sqlite3_result_int(ctx, 1);
    else
        sqlite3_result_int(ctx, 0);
}


void cfInIgnoredFandoms(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int fandom1 = sqlite3_value_int(argv[0]);
    int fandom2 = sqlite3_value_int(argv[1]);
    using namespace core::fandom_lists;
    auto* data = ThreadData::GetUserData();
    // whitelist branch
    if(data->hasWhitelistedFandoms){
        if(fandom2 == -1){
            auto it = data->fandomStates.find(fandom1);
            bool isWhitelisted = it != data->fandomStates.end()
                    && it->second.inclusionMode == EInclusionMode::im_include
                    && it->second.crossoverInclusionMode *in(ECrossoverInclusionMode::cim_select_all,ECrossoverInclusionMode::cim_select_pure);
            if(!isWhitelisted)
            {
                sqlite3_result_int(ctx,1);
                return;
            }
        }
        else{
            auto itFirstFandom = data->fandomStates.find(fandom1);
            auto itSecondFandom = data->fandomStates.find(fandom2);
            if(itFirstFandom == data->fandomStates.end() && itSecondFandom == data->fandomStates.end()){
                sqlite3_result_int(ctx,1);
                return;
            }
            else if(itFirstFandom == data->fandomStates.end() && itSecondFandom != data->fandomStates.end()){
                bool isWhitelisted = itSecondFandom->second.inclusionMode == EInclusionMode::im_include
                        && itSecondFandom->second.crossoverInclusionMode *in(ECrossoverInclusionMode::cim_select_all,ECrossoverInclusionMode::cim_select_crossovers);
                if(!isWhitelisted)
                {
                    sqlite3_result_int(ctx,1);
                    return;
                }

            }
            else if(itFirstFandom != data->fandomStates.end() && itSecondFandom == data->fandomStates.end()){
                bool isWhitelisted = itFirstFandom->second.inclusionMode == EInclusionMode::im_include
                        && itFirstFandom->second.crossoverInclusionMode *in(ECrossoverInclusionMode::cim_select_all,ECrossoverInclusionMode::cim_select_crossovers);
                if(!isWhitelisted)
                {
                    sqlite3_result_int(ctx,1);
                    return;
                }
            }
            else{
                bool isFirstWhitelisted = itFirstFandom->second.inclusionMode == EInclusionMode::im_include
                        && itFirstFandom->second.crossoverInclusionMode *in(ECrossoverInclusionMode::cim_select_all,ECrossoverInclusionMode::cim_select_crossovers);
                bool isSecondWhitelisted = itSecondFandom->second.inclusionMode == EInclusionMode::im_include
                        && itSecondFandom->second.crossoverInclusionMode *in(ECrossoverInclusionMode::cim_select_all,ECrossoverInclusionMode::cim_select_crossovers);
                if(!(isFirstWhitelisted || isSecondWhitelisted))
                {
                    sqlite3_result_int(ctx,1);
                    return;
                }

            }
        }
    }

    // ignores branch
    if(fandom2 == -1)
    {
        auto it = data->fandomStates.find(fandom1);
        bool isIgnored = it != data->fandomStates.end()
                && it->second.inclusionMode == EInclusionMode::im_exclude
                && it->second.crossoverInclusionMode *in(ECrossoverInclusionMode::cim_select_all,ECrossoverInclusionMode::cim_select_pure);
        if(isIgnored)
        {
            sqlite3_result_int(ctx, 1);
            return;
        }
    }
    else
    {
        // this is SO going to break >_<
        auto itFirstFandom = data->fandomStates.find(fandom1);
        auto itSecondFandom = data->fandomStates.find(fandom2);

        if(itFirstFandom == data->fandomStates.end() && itSecondFandom == data->fandomStates.end()){
            // do nothing here, wil lassign 0 later
        }
        else if(itFirstFandom == data->fandomStates.end() && itSecondFandom != data->fandomStates.end()){
            if(itSecondFandom->second.inclusionMode == EInclusionMode::im_exclude
                    && itSecondFandom->second.crossoverInclusionMode *in(ECrossoverInclusionMode::cim_select_all,ECrossoverInclusionMode::cim_select_crossovers)){
                sqlite3_result_int(ctx, 1);
                return;
            }
        }
        else if(itSecondFandom == data->fandomStates.end() && itFirstFandom != data->fandomStates.end()){
            if(itFirstFandom->second.inclusionMode == EInclusionMode::im_exclude
                    && itFirstFandom->second.crossoverInclusionMode *in(ECrossoverInclusionMode::cim_select_all,ECrossoverInclusionMode::cim_select_crossovers)){
                sqlite3_result_int(ctx, 1);
                return;
            }
        }
        else{
            bool firstFandomIgnored = itFirstFandom->second.inclusionMode == EInclusionMode::im_exclude
                    && itFirstFandom->second.crossoverInclusionMode *in(ECrossoverInclusionMode::cim_select_all,ECrossoverInclusionMode::cim_select_crossovers);
            bool secondFandomIgnored = itSecondFandom->second.inclusionMode == EInclusionMode::im_exclude
                    && itSecondFandom->second.crossoverInclusionMode *in(ECrossoverInclusionMode::cim_select_all,ECrossoverInclusionMode::cim_select_crossovers);
            if(firstFandomIgnored || secondFandomIgnored){
                sqlite3_result_int(ctx, 1);
                return;
            }
        }
    }
    sqlite3_result_int(ctx,0);
}

void cfInActiveTags(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int ficId = sqlite3_value_int(argv[0]);
    //thread_local An<UserInfoAccessor> accessor;
    auto* data = ThreadData::GetUserData();

    if(data->ficIDsForActivetags.contains(ficId))
        sqlite3_result_int(ctx, 1);
    else
        sqlite3_result_int(ctx, 0);
}

void cfInFicSelection(sqlite3_context* ctx, int , sqlite3_value** argv)
{
    int ficId = sqlite3_value_int(argv[0]);
    //thread_local An<UserInfoAccessor> accessor;
    auto* data = ThreadData::GetUserData();

    if(data->ficsForSelection.contains(ficId))
        sqlite3_result_int(ctx, 1);
    else
        sqlite3_result_int(ctx, 0);
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

bool InstallCustomFunctions(sql::Database db){
    return InstallCustomFunctions(*static_cast<QSqlDatabase*>(db.internalPointer()));
}

bool InstallCustomFunctions(QSqlDatabase db)
{
    //QLOG_INFO() << "Installing custom sqlite functions";
    QVariant v = db.driver()->handle();
    if (v.isValid() && qstrcmp(v.typeName(), "sqlite3*")==0)
    {
        sqlite3 *db_handle = *static_cast<sqlite3 **>(v.data());
        if (db_handle != nullptr) {
            sqlite3_initialize();

            sqlite3_create_function(db_handle, "cfRegexp", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr, &cfRegexp, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfGetSecondFandom", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr, &cfGetSecondFandom, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfReturnCapture", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr, &cfReturnCapture, nullptr, nullptr);

            sqlite3_create_function(db_handle, "cfInTags", 1, SQLITE_UTF8 , nullptr, &cfInTags, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfInFicsForAuthors", 1, SQLITE_UTF8 , nullptr, &cfInFicsForAuthors, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfInSnoozes", 1, SQLITE_UTF8 , nullptr, &cfInSnoozes, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfInSourceFics", 1, SQLITE_UTF8 , nullptr, &cfInSourceFics, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfInRecommendations", 1, SQLITE_UTF8 , nullptr, &cfInRecommendations, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfInScores", 1, SQLITE_UTF8 , nullptr, &cfInScores, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfRecommendationsMetascore", 1, SQLITE_UTF8 , nullptr, &cfRecommendationsMetascore, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfRecommendationsPureVotes", 1, SQLITE_UTF8 , nullptr, &cfRecommendationsPureVotes, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfScoresMatchCount", 1, SQLITE_UTF8 , nullptr, &cfScoresMatchCount, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfInAuthors", 1, SQLITE_UTF8 , nullptr, &cfInAuthors, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfInLikedAuthors", 1, SQLITE_UTF8 , nullptr, &cfInLikedAuthors, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfInIgnoredFandoms", 2, SQLITE_UTF8 , nullptr, &cfInIgnoredFandoms, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfInActiveTags", 1, SQLITE_UTF8 , nullptr, &cfInActiveTags, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfInFicSelection", 1, SQLITE_UTF8 , nullptr, &cfInFicSelection, nullptr, nullptr);
            sqlite3_create_function(db_handle, "cfGetFirstFandom", 1, SQLITE_UTF8 , nullptr, &cfGetFirstFandom, nullptr, nullptr);

            //QLOG_INFO() << "Installed funcs succesfully";
            return true;
        }
    }
    QLOG_INFO() << "Func install failed";
    return false;
}

bool ReadDbFile(QString file, QString connectionName)
{
    QFile data(file);

    QSettings settings("settings/settings.ini", QSettings::IniFormat);
    auto reportSchemaErrors = settings.value("Settings/reportSchemaErrors", false).toBool();

    QLOG_INFO() << "Reading init file: " << file;
    if (data.open(QFile::ReadOnly))
    {
        QTextStream in(&data);
        QString dbCode = in.readAll();
        data.close();
        QStringList statements = dbCode.split(";");
        sql::Database db;
        if(!connectionName.isEmpty())
            db = sql::Database::database(connectionName.toStdString());
        else
            db = sql::Database::database();

        db.transaction();
        sql::Query q(db);
        for(QString statement: statements)
        {
            statement = statement.replace(QRegExp("\\t"), " ");
            statement = statement.replace(QRegExp("\\r"), " ");
            statement = statement.replace(QRegExp("\\n"), " ");

            if(statement.trimmed().isEmpty() || statement.trimmed().left(2) == "--")
                continue;

            //bool isOpen = db.isOpen();

            q.prepare(statement.trimmed().toStdString());
            QLOG_TRACE() << "Executing: " << statement;
            sql::ExecAndCheck(q, reportSchemaErrors, {sql::ESqlErrors::se_duplicate_column,sql::ESqlErrors::se_unique_row_violation});
        }
        db.commit();

    }
    else
        return false;
    return true;
}

QStringList GetIdListForQuery(QSharedPointer<core::Query> query, sql::Database db)
{
    auto where = query->str;
    QStringList result;
    auto qs = "select group_concat(id, ',') as merged, " + where;
    //qs= qs.arg(where);

    sql::Query q(db);
    q.prepare(qs);
    auto it = query->bindings.cbegin();
    auto end = query->bindings.cend();
    while(it != end)
    {
        qDebug() << it->key << " " << it->value;
        q.bindValue(it->key, it->value);
        ++it;
    }
    QLOG_INFO_PURE() << "RANDOM: " << QString::fromStdString(qs);
    if(!sql::ExecAndCheck(q))
        return result;
    QLOG_INFO_PURE() << "RANDOM FINISHED";

    q.next();
    auto temp = q.value("merged").toString();
    if(trim_copy(temp).length() == 0)
        return result;
    result = QString::fromStdString(temp).split(",");
    return result;
}


bool BackupSqliteDatabase(QString dbName)
{
    bool success = true;
    #ifndef CLIENT_APP
    Q_UNUSED(dbName)
    #endif
#ifdef CLIENT_APP
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
#endif
    return success;
}

bool PushFandomToTopOfRecent(QString fandom, sql::Database db)
{
    QString upsert1 ("UPDATE recent_fandoms SET seq_num= (select max(seq_num) +1 from  recent_fandoms ) WHERE fandom = '%1'; ");
    QString upsert2 ("INSERT INTO recent_fandoms(fandom, seq_num) select '%1',  (select max(seq_num)+1 from recent_fandoms) WHERE changes() = 0;");
    sql::Query q1(db);
    upsert1 = upsert1.arg(fandom);
    q1.prepare(upsert1.toStdString());
    if(!sql::ExecAndCheck(q1))
        return false;
    upsert2 = upsert2.arg(fandom);
    q1.prepare(upsert2.toStdString());
    if(!sql::ExecAndCheck(q1))
        return false;
    return true;
}

QStringList FetchRecentFandoms(sql::Database db)
{
    sql::Query q1(db);
    QString qsl = "select fandom from recent_fandoms where fandom is not 'base' order by seq_num desc";
    q1.prepare(qsl.toStdString());
    q1.exec();
    QStringList result;
    while(q1.next())
    {
        result.push_back(QString::fromStdString(q1.value(0).toString()));
    }
    sql::CheckExecution(q1);
    return result;
}

bool RebaseFandomsToZero(sql::Database db)
{
    sql::Query q1(db);
    QString qsl = "UPDATE recent_fandoms SET seq_num = seq_num - (select min(seq_num) from recent_fandoms where fandom is not 'base') where fandom is not 'base'";
    q1.prepare(qsl.toStdString());
    if(!sql::ExecAndCheck(q1))
        return false;
    return true;
}

QDateTime GetCurrentDateTime(sql::Database db)
{
    QDateTime dt;
    sql::Query q(db);
    QString qsl = "select CURRENT_TIMESTAMP";
    q.prepare(qsl.toStdString());
    if(!sql::ExecAndCheck(q))
        return dt;
    q.next();
    dt = q.value(0).toDateTime();
    return dt;
}

sql::Database InitSqliteDatabase(QString name, bool setDefault)
{
    QString path = name;
    sql::Database db;
    if(setDefault)
        db = sql::Database::addDatabase("QSQLITE");
    else
        db = sql::Database::addDatabase("QSQLITE", name.toStdString());

    sql::ConnectionToken token;
    token.serviceName = path.toStdString() + ".sqlite";
    db.setConnectionToken(token);

    bool isOpen = db.open();
    QLOG_INFO() << "Database status: " << name << ", open : " << isOpen;
    InstallCustomFunctions(db);

    return db;
}

int CreateNewTask(sql::Database db)
{
    Transaction tr(db);
    sql::Query q(db);
    QString qsl = "insert into PageTasks(type) values(0)";
    q.prepare(qsl.toStdString());
    if(!sql::ExecAndCheck(q))
        return -1;
    auto id = GetLastIdForTable("PageTasks", db);
    tr.finalize();
    return id;
}

int CreateNewSubTask(int taskId, sql::Database db)
{
    Transaction tr(db);
    sql::Query q(db);
    QString qsl = "insert into PageTaskParts(task_id) values(:task_id)";
    q.prepare(qsl.toStdString());
    q.bindValue("task_id", taskId);
    if(!sql::ExecAndCheck(q))
        return -1;
    auto id = GetLastIdForTable("PageTaskParts", db);
    tr.finalize();
    return id;
}

sql::Database InitNamedSqliteDatabase(QString dbName, QString filename, bool setDefault)
{
    sql::Database db;
    if(setDefault)
        db = sql::Database::addDatabase("QSQLITE");
    else
        db = sql::Database::addDatabase("QSQLITE", dbName.toStdString());
    sql::ConnectionToken token;
    token.serviceName = filename.toStdString() + ".sqlite";
    db.setConnectionToken(token);
    bool isOpen = db.open();
    QLOG_INFO() << "Database status: " << dbName << ", open : " << isOpen;
    InstallCustomFunctions(db);

    return db;
}

sql::Database InitSqliteDatabase2(QString file, QString name, bool setDefault)
{
    //QString path = name;
    sql::Database db;
    if(setDefault)
        db = sql::Database::addDatabase("QSQLITE");
    else
        db = sql::Database::addDatabase("QSQLITE", name.toStdString());

    sql::ConnectionToken token;
    token.serviceName = file.toStdString() + ".sqlite";
    db.setConnectionToken(token);

    bool isOpen = db.open();
    QLOG_INFO() << "Database status: " << name << ", open : " << isOpen;
    InstallCustomFunctions(db);

    return db;
}

sql::Database InitAndUpdateSqliteDatabaseForFile(QString folder,
                                          QString file,
                                          QString sqlFile,
                                          QString connectionName,
                                          bool setDefault)
{
    sql::Database db;
    if(setDefault)
        db = sql::Database::addDatabase("QSQLITE");
    else
        db = sql::Database::addDatabase("QSQLITE", connectionName.toStdString());
    QString filename  = folder + "/" + file + ".sqlite";


    sql::ConnectionToken token;
    token.serviceName = filename.toStdString();
    db.setConnectionToken(token);
    bool isOpen = db.open();

    InstallCustomFunctions(db);

    QSettings settings("settings/settings.ini", QSettings::IniFormat);
    ReadDbFile(sqlFile, setDefault ? "" : connectionName);
    bool uuidSuccess = sql::EnsureUUIDForUserDatabase(QUuid::createUuid(), db).success;
    QLOG_INFO() << "Database status: " << connectionName << ", open : " << isOpen << "uuid success: " << uuidSuccess;
    return db;
}




}
}


