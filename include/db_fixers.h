#pragma once
#include <QSqlDatabase>
namespace dbfix{
    //void EnsureFandomIndexExists(QSqlDatabase db);
    void FillFFNId(QSqlDatabase db);
    void ReplaceUrlInLinkedAuthorsWithID(QSqlDatabase db);
    void RebindDuplicateFandoms(QSqlDatabase db);
    //void SetLastFandomUpdateTo
}
