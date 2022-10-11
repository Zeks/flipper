/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

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
#include "sql_abstractions/sql_query.h"

class FicFilter
{
public:
    FicFilter() = default;
    virtual ~FicFilter() = default;
    virtual bool Passed(core::Fanfic*, const SlashFilterState& slashFilter) = 0;
};

class FicFilterSlash : public FicFilter
{
public:
    FicFilterSlash();
    virtual ~FicFilterSlash()= default;
    virtual bool Passed(core::Fanfic*, const SlashFilterState& slashFilter);
    CommonRegex regexToken;
};

class FicSource
{
public:
    FicSource() = default;
    virtual ~FicSource() = default;

    virtual void FetchData(const core::StoryFilter& filter, QVector<core::Fanfic>*) = 0;
    virtual int GetFicCount(const core::StoryFilter& filter) = 0;

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
    FicSourceDirect(sql::Database db, QSharedPointer<core::RNGData> rngData);
    virtual ~FicSourceDirect() = default;
    virtual void FetchData(const core::StoryFilter &filter, QVector<core::Fanfic>*) override;
    sql::Query BuildQuery(const core::StoryFilter &filter, bool countOnly = false);
    inline core::Fanfic LoadFanfic(sql::Query& q);
    int GetFicCount(const core::StoryFilter &filter) override;
    //QSet<int> GetAuthorsForFics(QSet<int> ficIDsForActivetags);
    void InitQueryType(bool client = false, QString userToken = QString());
    QSharedPointer<core::Query> currentQuery;
    core::DefaultQueryBuilder queryBuilder; // builds search queries
    core::CountQueryBuilder countQueryBuilder; // builds specialized query to get the last page for the interface;
    sql::Database db;
};


