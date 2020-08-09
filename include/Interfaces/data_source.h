/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#pragma once
#include "core/section.h"
#include "storyfilter.h"
#include "queryinterfaces.h"
#include "querybuilder.h"
#include "regex_utils.h"
#include "in_tag_accessor.h"
#include <QSqlQuery>

class FicFilter
{
public:
    FicFilter();
    virtual ~FicFilter();
    virtual bool Passed(core::Fanfic*, const SlashFilterState& slashFilter) = 0;
};

class FicFilterSlash : public FicFilter
{
public:
    FicFilterSlash();
    virtual ~FicFilterSlash();
    virtual bool Passed(core::Fanfic*, const SlashFilterState& slashFilter);
    CommonRegex regexToken;
};

class FicSource
{
public:
    FicSource();
    virtual ~FicSource();

    virtual void FetchData(core::StoryFilter filter, QVector<core::Fanfic>*) = 0;
    virtual int GetFicCount(core::StoryFilter filter) = 0;

    void AddFicFilter(QSharedPointer<FicFilter>);
    void ClearFilters();
    QList<QSharedPointer<FicFilter>> filters;
    int availableFics = 0;
    int availablePages = 0;
    int currentPage = 0;
    int lastFicId = 0;
    UserData userData;
};


class FicSourceDirect : public FicSource
{
public:
    FicSourceDirect(QSharedPointer<database::IDBWrapper> db, QSharedPointer<core::RNGData> rngData);
    virtual ~FicSourceDirect();
    virtual void FetchData(core::StoryFilter filter, QVector<core::Fanfic>*);
    QSqlQuery BuildQuery(core::StoryFilter filter, bool countOnly = false) ;
    inline core::Fanfic LoadFanfic(QSqlQuery& q);
    int GetFicCount(core::StoryFilter filter);
    //QSet<int> GetAuthorsForFics(QSet<int> ficIDsForActivetags);
    void InitQueryType(bool client = false, QString userToken = QString());
    QSharedPointer<core::Query> currentQuery;
    core::DefaultQueryBuilder queryBuilder; // builds search queries
    core::CountQueryBuilder countQueryBuilder; // builds specialized query to get the last page for the interface;
    QSharedPointer<database::IDBWrapper> db;
};


