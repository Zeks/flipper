#include "core/recommendation_list.h"
#include <QDebug>
#include <QDataStream>
#include <iostream>

namespace core {

void core::RecommendationList:: Log()
{

    qDebug() << "List id: " << id ;
    qDebug() << "name: " << name;
    qDebug() << "automatic: " << isAutomatic;
    qDebug() << "useWeighting: " << useWeighting;
    qDebug() << "useDislikes: " << useDislikes;
    qDebug() << "useDeadFicIgnore: " << useDeadFicIgnore;
    qDebug() << "useMoodAdjustment: " << useMoodAdjustment;
    qDebug() << "hasAuxDataFilled: " << hasAuxDataFilled;
    qDebug() << "maxUnmatchedPerMatch: " << maxUnmatchedPerMatch;
    qDebug() << "ficCount: " << ficCount ;
    qDebug() << "tagToUse: " << tagToUse;
    qDebug() << "minimumMatch: " << minimumMatch ;
    qDebug() << "alwaysPickAt: " << alwaysPickAt ;
    qDebug() << "pickRatio: " << maxUnmatchedPerMatch ;
    qDebug() << "created: " << created.toString();
    //qDebug() << "source fics: " << ficData.sourceFics;
}

void core::RecommendationList::PassSetupParamsInto(RecommendationList &other)
{
    other.isAutomatic = isAutomatic;
    other.useWeighting = useWeighting;
    other.useMoodAdjustment = useMoodAdjustment;
    other.useDislikes = useDislikes;
    other.useDeadFicIgnore= useDeadFicIgnore;
    other.minimumMatch = minimumMatch;
    other.alwaysPickAt = alwaysPickAt;
    other.maxUnmatchedPerMatch = maxUnmatchedPerMatch;
    other.name = name;
}

void RecommendationListFicData::Clear()
{
    sourceFics.clear();
    fics.clear();
    purges.clear();
    matchCounts.clear();
    noTrashScores.clear();
    authorIds.clear();
    matchReport.clear();
    ficToScore.clear();
    breakdowns.clear();
}

}
