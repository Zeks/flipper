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

#include "include/queryinterfaces.h"
#include "include/rng.h"

#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_query.h"
#include <functional>
#include <random>

namespace core{
class IWhereFilter{
public:
    virtual ~IWhereFilter();
    virtual QString GetString(StoryFilter filter) = 0;
    QString userToken;
};

class TagFilteringFullDB : public IWhereFilter{
public:
    virtual ~TagFilteringFullDB();
    virtual QString GetString(StoryFilter filter);
};
class TagFilteringClient : public IWhereFilter{
public:
    virtual ~TagFilteringClient();
    virtual QString GetString(StoryFilter filter);
};

class InRecommendationsFullDB : public IWhereFilter{
public:
    virtual ~InRecommendationsFullDB();
    virtual QString GetString(StoryFilter filter);
};
class InRecommendationsClient : public IWhereFilter{
public:
    virtual ~InRecommendationsClient();
    virtual QString GetString(StoryFilter filter);
};

class FandomIgnoreClient : public IWhereFilter{
public:
    virtual ~FandomIgnoreClient();
    virtual QString GetString(StoryFilter filter);
};
class FandomIgnoreFullDB : public IWhereFilter{
public:
    virtual ~FandomIgnoreFullDB();
    virtual QString GetString(StoryFilter filter);
};

class DefaultQueryBuilder : public IQueryBuilder
{
public:
    DefaultQueryBuilder(bool client = false, QString userToken = QString());
    virtual ~DefaultQueryBuilder() override {}
    virtual QSharedPointer<Query> Build(StoryFilter,  bool createLimits = true) override;
    void SetIdRNGgenerator(IRNGGenerator* generator){rng.reset(generator);}
    virtual void ProcessBindings(StoryFilter, QSharedPointer<Query>);
    void InitTagFilterBuilder(bool client = false, QString userToken = QString());
    QSharedPointer<IRNGGenerator> rng;

protected:
    virtual void InitQuery();
    QString CreateCustomFields(StoryFilter);
    QString CreateWhere(StoryFilter,
                        bool usePageLimiter = false);

    QString ProcessBias(StoryFilter);
    QString ProcessExpiration(StoryFilter);
    QString ProcessSumFaves(StoryFilter);
    QString ProcessFandoms(StoryFilter);
    //QString ProcessAuthor(StoryFilter);
    QString ProcessOtherFandomsMode(StoryFilter, bool renameToFID = true);
    QString ProcessSumRecs(StoryFilter, bool appendComma = true);
    QString ProcessSumVotes(StoryFilter, bool appendComma = true);
    QString ProcessScores(StoryFilter, bool appendComma = true);
    QString ProcessTags(StoryFilter);
    QString ProcessAuthor(StoryFilter);
    QString ProcessFicID(StoryFilter);
    QString ProcessRecommenders(StoryFilter);
    QString ProcessSnoozes(StoryFilter);
    QString ProcessUrl(StoryFilter);
    QString ProcessGenreValues(StoryFilter);
    QString ProcessWordcount(StoryFilter);
    QString ProcessRating(StoryFilter);
    QString ProcessSlashMode(StoryFilter, bool renameToFID = true);
    QString ProcessGenreIncluson(StoryFilter);
    QString ProcessWordInclusion(StoryFilter);
    QString ProcessDateRange(StoryFilter);
    QString ProcessActiveRecommendationsPart(StoryFilter);
    virtual QString ProcessWhereSortMode(StoryFilter);

    QString ProcessDiffField(StoryFilter);
    QString ProcessStatusFilters(StoryFilter);
    QString ProcessNormalOrCrossover(StoryFilter, bool renameToFID = true);
    QString ProcessFilteringMode(StoryFilter);
    QString ProcessFandomIgnore(StoryFilter);
    QString ProcessCrossovers(StoryFilter);
    QString ProcessActiveTags(StoryFilter);
    QString ProcessRandomization(StoryFilter, QString);


    QString BuildSortMode(StoryFilter);
    QString CreateLimitQueryPart(StoryFilter, bool collate = true);

    QString BuildIdListQuery(StoryFilter);
    bool HasIdListForQuery(QString);
    QSharedPointer<Query> NewQuery();

    QString queryString;
    QString diffField;
    QString activeTags;
    QString wherePart;
    //bool countOnlyQuery = false;
    QSharedPointer<IWhereFilter> tagFilterBuilder;
    QSharedPointer<IWhereFilter> ignoredFandomsBuilder;
    QSharedPointer<IWhereFilter> inRecommendationsBuilder;
    QSharedPointer<Query> query;
    QString userToken;
    bool thinClientMode = false;
};

class CountQueryBuilder : public DefaultQueryBuilder
{
public:
    CountQueryBuilder(bool client = false, QString userToken = QString());
    QSharedPointer<Query> Build(StoryFilter,  bool createLimits = false) override;
private:
    QString CreateWhere(StoryFilter,
                        bool usePageLimiter = false);

    virtual QString ProcessWhereSortMode(StoryFilter) override;

//    QString queryString;
//    QString diffField;
//    QString wherePart;

//    QSharedPointer<Query> query;
//    sql::Database db;
};
}
