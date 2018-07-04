#pragma once
#include <QHash>
#include <QSet>
#include <QSharedPointer>
#include <QVector>
#include "GlobalHeaders/SingletonHolder.h"
namespace interfaces{
class Authors;
}
namespace core{
class RecommendationList;
class FavHolder
{
public:
    void LoadFavourites(QSharedPointer<interfaces::Authors> authorInterface);
    QHash<int, int> GetMatchedFicsForFavList(QSet<int> sourceFics,  QSharedPointer<core::RecommendationList> params);
    QHash<int, QSet<int>> favourites;
};
}
BIND_TO_SELF_SINGLE(core::FavHolder);
