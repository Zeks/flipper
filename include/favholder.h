/*
Flipper is a replacement search engine for fanfiction.net search results
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
