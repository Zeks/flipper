/*
FFSSE is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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

#include "queryinterfaces.h"

#include <QSqlDatabase>
#include <functional>
#include <random>

namespace core{

struct DefaultRNGgenerator : public IRNGGenerator{
    virtual QString Get(QSharedPointer<Query> where, QSqlDatabase db);
    QHash<QString, QStringList> randomIdLists;
    QSharedPointer<database::IDBWrapper> portableDBInterface;
};


class DefaultQueryBuilder : public IQueryBuilder
{
public:
    DefaultQueryBuilder();
    virtual ~DefaultQueryBuilder(){}
    virtual QSharedPointer<Query> Build(StoryFilter);
    void SetIdRNGgenerator(IRNGGenerator* generator){rng.reset(generator);}
    virtual void ProcessBindings(StoryFilter, QSharedPointer<Query>);
    QScopedPointer<IRNGGenerator> rng;

protected:
    virtual void InitQuery();
    QString CreateCustomFields(StoryFilter);
    QString CreateWhere(StoryFilter,
                        bool usePageLimiter = false);

    QString ProcessBias(StoryFilter);
    QString ProcessSumFaves(StoryFilter);
    QString ProcessFandoms(StoryFilter);
    QString ProcessSumRecs(StoryFilter, bool appendComma = true);
    QString ProcessTags(StoryFilter);
    QString ProcessUrl(StoryFilter);
    QString ProcessWordcount(StoryFilter);
    QString ProcessGenreIncluson(StoryFilter);
    QString ProcessWordInclusion(StoryFilter);
    QString ProcessActiveRecommendationsPart(StoryFilter);
    virtual QString ProcessWhereSortMode(StoryFilter);

    QString ProcessDiffField(StoryFilter);
    QString ProcessStatusFilters(StoryFilter);
    QString ProcessNormalOrCrossover(StoryFilter);
    QString ProcessFilteringMode(StoryFilter);
    QString ProcessActiveTags(StoryFilter);
    QString ProcessRandomization(StoryFilter, QString);


    QString BuildSortMode(StoryFilter);
    QString CreateLimitQueryPart(StoryFilter);

    QString BuildIdListQuery(StoryFilter);
    bool HasIdListForQuery(QString);
    QSharedPointer<Query> NewQuery();
    QString queryString;
    QString diffField;
    QString activeTags;
    QString wherePart;
    //bool countOnlyQuery = false;

    QSharedPointer<Query> query;
    QSqlDatabase db;
};

class CountQueryBuilder : public DefaultQueryBuilder
{
public:
    CountQueryBuilder();
    QSharedPointer<Query> Build(StoryFilter);
private:
    QString CreateWhere(StoryFilter,
                        bool usePageLimiter = false);

    virtual QString ProcessWhereSortMode(StoryFilter) override;

    QString queryString;
    QString diffField;
    QString wherePart;

    QSharedPointer<Query> query;
    QSqlDatabase db;
};
}
