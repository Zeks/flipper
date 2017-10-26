#pragma once
#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
namespace database{

class IDBWrapper{
public:
    virtual ~IDBWrapper();
    virtual int GetLastIdForTable(QString tableName, QSqlDatabase db) = 0;
    virtual QDateTime GetCurrentDateTime() = 0;
};
}
