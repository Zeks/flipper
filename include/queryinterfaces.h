#pragma once
#include <QString>
#include <QVariantHash>
#include "storyfilter.h"
namespace core {
struct Query
{
    void Clear(){ str.clear(); bindings.clear();}
    QString str;
    QVariantHash bindings;
};

struct IRNGGenerator{
    virtual ~IRNGGenerator(){}
    virtual QString Get(Query)  = 0;
};

class IQueryBuilder
{
public:
    virtual ~IQueryBuilder(){}
    virtual Query Build(StoryFilter) = 0;
};


}
