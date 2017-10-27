#pragma once
#include <QString>
#include <QVariantHash>
#include <QSharedPointer>
#include <QSqlDatabase>
#include "storyfilter.h"
//#include "Interfaces/db_interface.h"
namespace database { class IDBWrapper; }
namespace core {
struct Query
{
    void Clear(){ str.clear(); bindings.clear();}
    QString str;
    QVariantHash bindings;
};

struct IRNGGenerator{
    virtual ~IRNGGenerator(){}
    virtual QString Get(QSharedPointer<Query>, QSqlDatabase db)  = 0;
};

class IQueryBuilder
{
public:
    virtual ~IQueryBuilder(){}
    virtual QSharedPointer<Query> Build(StoryFilter) = 0;
    QSharedPointer<database::IDBWrapper> portableDBInterface;
};


}
