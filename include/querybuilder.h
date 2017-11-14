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
    QSharedPointer<Query> Build(StoryFilter);
    void SetIdRNGgenerator(IRNGGenerator* generator){rng.reset(generator);}
    QScopedPointer<IRNGGenerator> rng;
private:
    QString CreateCustomFields(StoryFilter);
    QString CreateWhere(StoryFilter);

    QString ProcessBias(StoryFilter);
    QString ProcessSumFaves(StoryFilter);
    QString ProcessSumRecs(StoryFilter);
    QString ProcessTags(StoryFilter);
    QString ProcessUrl(StoryFilter);
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
    void ProcessBindings(StoryFilter, QSharedPointer<Query>);

    QString BuildSortMode(StoryFilter);
    QString CreateLimitQueryPart(StoryFilter);

    QString BuildIdListQuery(StoryFilter);
    bool HasIdListForQuery(QString);
    QSharedPointer<Query> NewQuery();
    QString queryString;
    QString diffField;
    QString activeTags;
    QString wherePart;

    QSharedPointer<Query> query;
    QSharedPointer<Query> idQuery;
    QSqlDatabase db;
};
}
