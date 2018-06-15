#pragma once
#include "core/section.h"
#include "storyfilter.h"
#include "queryinterfaces.h"
#include "querybuilder.h"
#include "regex_utils.h"
#include <QSqlQuery>

class FicFilter
{
public:
    FicFilter();
    virtual ~FicFilter();
    virtual bool Passed(core::Fic*, const SlashFilterState& slashFilter) = 0;
};

class FicFilterSlash : public FicFilter
{
public:
    FicFilterSlash();
    virtual ~FicFilterSlash();
    virtual bool Passed(core::Fic*, const SlashFilterState& slashFilter);
    CommonRegex regexToken;
};

class FicSource
{
public:
    FicSource();
    virtual ~FicSource();

    virtual void FetchData(core::StoryFilter filter, QList<core::Fic>*) = 0;
    virtual int GetFicCount(core::StoryFilter filter) = 0;

    void AddFicFilter(QSharedPointer<FicFilter>);
    void ClearFilters();
    QList<QSharedPointer<FicFilter>> filters;
    int availableFics = 0;
    int availablePages = 0;
    int currentPage = 0;
    int lastFicId = 0;

};


class FicSourceDirect : public FicSource
{
public:
    FicSourceDirect(QSharedPointer<database::IDBWrapper> db);
    virtual ~FicSourceDirect();
    virtual void FetchData(core::StoryFilter filter, QList<core::Fic>*);
    QSqlQuery BuildQuery(core::StoryFilter filter, bool countOnly = false) ;
    inline core::Fic LoadFanfic(QSqlQuery& q);
    int GetFicCount(core::StoryFilter filter);

    QSharedPointer<core::Query> currentQuery;
    core::DefaultQueryBuilder queryBuilder; // builds search queries
    core::CountQueryBuilder countQueryBuilder; // builds specialized query to get the last page for the interface;
    QSharedPointer<database::IDBWrapper> db;
};


