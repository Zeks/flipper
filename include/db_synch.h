#pragma once 

#include <QString>
#include <QSqlDatabase>


namespace database{
bool ExecuteCommandsFromDbScriptFile(QString file, QSqlDatabase db, bool usePrepare = true);
}
