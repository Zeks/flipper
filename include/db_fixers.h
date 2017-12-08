#pragma once
#include <QSqlDatabase>
namespace dbfix{
    void EnsureFandomIndexExists(QSqlDatabase db, bool forcedReload = false);
}
