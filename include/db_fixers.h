#pragma once
#include <QSqlDatabase>
#include "pure_sql.h"
namespace dbfix{
    //void EnsureFandomIndexExists(QSqlDatabase db);
    void FillFFNId(QSqlDatabase db);
    void ReplaceUrlInLinkedAuthorsWithID(QSqlDatabase db);
    bool RebindDuplicateFandoms(QSqlDatabase db);
    void TrimUserUrls(QSqlDatabase db);
    database::puresql::DiagnosticSQLResult<bool> PassSlashDataIntoNewTable(QSqlDatabase db);
    //void SetLastFandomUpdateTo
}
