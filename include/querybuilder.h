#pragma once

#include "queryinterfaces.h"

#include <QSqlDatabase>
#include <functional>
#include <random>
namespace core{

struct DefaultRNGgenerator : public IRNGGenerator{
    virtual QString Get(Query where);
    QHash<QString, QStringList> randomIdLists;
};


class DefaultQueryBuilder : public IQueryBuilder
{
public:
    Query Build(StoryFilter);
    void SetIdRNGgenerator(IRNGGenerator* generator){rng.reset(generator);}

private:
    QString CreateCustomFields(StoryFilter);
    QString CreateWhere(StoryFilter);

    QString ProcessBias(StoryFilter);
    QString ProcessSumFaves(StoryFilter);
    QString ProcessSumRecs(StoryFilter);
    QString ProcessTags(StoryFilter);
    QString ProcessFandoms(StoryFilter);
    QString ProcessWordcount(StoryFilter);
    QString ProcessGenreIncluson(StoryFilter);
    QString ProcessWordInclusion(StoryFilter);
    QString ProcessActiveRecommendationsPart(StoryFilter);
    QString ProcessWhereSortMode(StoryFilter);

    QString ProcessDiffField(StoryFilter);
    QString ProcessStatusFilters(StoryFilter);
    QString ProcessNormalOrCrossover(StoryFilter);
    QString ProcessFilteringMode(StoryFilter);
    QString ProcessActiveTags(StoryFilter);
    QString ProcessRandomization(StoryFilter, QString);
    void ProcessBindings(StoryFilter, Query&);

    QString BuildSortMode(StoryFilter);
    QString CreateLimitQueryPart(StoryFilter);

    QString BuildIdListQuery(StoryFilter);
    bool HasIdListForQuery(QString);
    QString queryString;
    QString diffField;
    QString activeTags;
    QString wherePart;
    QScopedPointer<IRNGGenerator> rng;
    Query query;
    Query idQuery;
};
}
