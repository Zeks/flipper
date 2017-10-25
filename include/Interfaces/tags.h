#pragma once
#include "Interfaces/base.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace database {
class Tags{
public:
    bool DeleteTag(QString);
    bool AddTag();
    virtual bool AssignTagToFandom(QString, QString tag) = 0;

    QSharedPointer<DBFandomsBase> fandomInterface;
    QSqlDatabase db;
};


}
