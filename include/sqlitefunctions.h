#pragma once
#include <QSqlQuery>
#include "include/section.h"
class sqlite3_context;
class sqlite3_value;


namespace database{
bool ExecAndCheck(QSqlQuery& q);
bool CheckExecution(QSqlQuery& q);
bool ExecuteQuery(QSqlQuery& q, QString query);
bool ExecuteQueryChain(QSqlQuery& q, QStringList queries);
namespace sqlite {
int GetLastIdForTable(QString tableName, QSqlDatabase db);
void cfRegexp(sqlite3_context* ctx, int argc, sqlite3_value** argv);
void cfReturnCapture(sqlite3_context* ctx, int argc, sqlite3_value** argv);
void cfGetFirstFandom(sqlite3_context* ctx, int argc, sqlite3_value** argv);
void cfGetSecondFandom(sqlite3_context* ctx, int argc, sqlite3_value** argv);
void InstallCustomFunctions(QSqlDatabase db);
bool ReadDbFile(QString file, QString connectionName, QSqlDatabase db);
QStringList GetIdListForQuery(core::Query query);
void BackupSqliteDatabase(QString dbname);
void PushFandomToTopOfRecent(QString fandom, QSqlDatabase db);
QStringList FetchRecentFandoms(QSqlDatabase db);
void RebaseFandomsToZero(QSqlDatabase db);



}

}
