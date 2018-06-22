#pragma once

#include "include/queryinterfaces.h"
#include "include/rng.h"

#include <QSqlDatabase>


namespace core{

struct IRNGGenerator{
    virtual ~IRNGGenerator(){}
    virtual QString Get(QSharedPointer<Query>, QSqlDatabase db)  = 0;
};

struct DefaultRNGgenerator : public IRNGGenerator{
    virtual QString Get(QSharedPointer<Query> where, QSqlDatabase db);
    QHash<QString, QStringList> randomIdLists;
    QSharedPointer<database::IDBWrapper> portableDBInterface;
};
}
