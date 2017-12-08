#pragma once
#include <QSqlDatabase>
namespace dbfix{
    void EnsureFandomIndexExists(QSqlDatabase db);
}
