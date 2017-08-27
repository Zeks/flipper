#pragma once
#include "storyfilter.h"
#include <QVariantHash>
#include <functional>
namespace core{

struct Query
{
    QString str;
    QVariantHash bindings;
};

class IQueryBuilder
{
public:
    virtual ~IQueryBuilder();
    virtual QString Build(StoryFilter) = 0;
    Query query;
    std::function<QList<int>(QString)> idListAccess;
    QHash<QString, QList<int>> randomIdLists;
};
class DefaultQueryBuilder : public IQueryBuilder
{
public:
    QString Build(StoryFilter);


private:
    QString CreateFields(StoryFilter);
    QString CreateWhere(StoryFilter);

    QString ProcessBias(StoryFilter);
    QString ProcessSumFaves(StoryFilter);
    QString ProcessSumRecs(StoryFilter);
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
    QString ProcessIdRandomization(StoryFilter);


    QString ProcessBindings(StoryFilter);

    QString BuildSortMode(StoryFilter);
    QString BuildConditions(StoryFilter);

    //select group_concat(id, ',') from recommenders order by id asc
    QString BuildIdListQuery(StoryFilter);
    bool HasIdListForQuery(QString);
    QString queryString;
    QString diffField;
    QString activeTags;



};
}
