#pragma once
#include "sql_abstractions/sql_query.h"
#include "sql_abstractions/sql_database.h"
#include "include/core/section.h"
#include "include/queryinterfaces.h"
#include "sqlite3.h"
#include <QSqlDatabase>


namespace database{

namespace sqlite {
int GetLastIdForTable(QString tableName, sql::Database db);
void cfRegexp(sqlite3_context* ctx, int argc, sqlite3_value** argv);
void cfReturnCapture(sqlite3_context* ctx, int argc, sqlite3_value** argv);
void cfGetFirstFandom(sqlite3_context* ctx, int argc, sqlite3_value** argv);
void cfGetSecondFandom(sqlite3_context* ctx, int argc, sqlite3_value** argv);
bool InstallCustomFunctions(QSqlDatabase db);
bool InstallCustomFunctions(sql::Database db);
bool ReadDbFile(QString file, QString connectionName);
QStringList GetIdListForQuery(QSharedPointer<core::Query> query, sql::Database db);
bool PushFandomToTopOfRecent(QString fandom, sql::Database db);
QStringList FetchRecentFandoms(sql::Database db);
bool RebaseFandomsToZero(sql::Database db);
QDateTime GetCurrentDateTime(sql::Database db);
sql::Database InitSqliteDatabase(QString name, bool setDefault = false);
sql::Database InitSqliteDatabase2(QString file, QString name, bool setDefault = false);
sql::Database InitNamedSqliteDatabase(QString dbName, QString filename, bool setDefault = false);

sql::Database InitAndUpdateSqliteDatabaseForFile(QString folder, QString file, QString sqlFile, QString connectionName, bool setDefault);

int CreateNewTask(sql::Database db);
int CreateNewSubTask(int taskId, int subTaskId, sql::Database db);
}

}
