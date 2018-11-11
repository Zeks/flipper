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

struct RecommendationListResult{
    QHash<int, int> recommendations;
    QHash<int, int> matchReport;
};

class RecommendationList;
class FavHolder
{
public:
    void LoadFavourites(QSharedPointer<interfaces::Authors> authorInterface);
    void CreateTempDataDir();
    void LoadDataFromDatabase(QSharedPointer<interfaces::Authors> authorInterface);
    void LoadStoredData();
    void SaveData();
    RecommendationListResult GetMatchedFicsForFavList(QSet<int> sourceFics,  QSharedPointer<core::RecommendationList> params);
    QHash<int, QSet<int>> favourites;
};
}
BIND_TO_SELF_SINGLE(core::FavHolder);
