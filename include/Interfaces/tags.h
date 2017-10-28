#pragma once
#include "Interfaces/base.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace interfaces {
class Tags{
public:
    bool DeleteTag(QString);
    bool AddTag();
    virtual bool AssignTagToFandom(QString, QString tag) = 0;
    QStringList ReadUserTags();

private:
    QStringList CreateDefaultTagList();

    QSharedPointer<Fandoms> fandomInterface;
    QSqlDatabase db;
};


}
