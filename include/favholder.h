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
#include <QDir>
#include <QSettings>
#include <utility>
#include "GlobalHeaders/SingletonHolder.h"
#include "third_party/roaring/roaring.hh"
#include "include/core/section.h"
#include "threaded_data/threaded_load.h"
#include "threaded_data/threaded_save.h"
#include "include/Interfaces/authors.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/genres.h"
#include "include/reclist_author_result.h"
#include "include/data_code/data_holders.h"
#include "include/data_code/rec_calc_data.h"
#include "include/rec_calc/rec_calculator_base.h"


namespace core{



class RecommendationList;
class RecCalculator
{
public:
    RecCalculator(QString settingsFile = "", QSharedPointer<interfaces::Authors> authors = {}, QSharedPointer<interfaces::Fanfics> fanfics = {})
        : holder(settingsFile, authors, fanfics){}
    void CreateTempDataDir();
    void LoadFavourites(QSharedPointer<interfaces::Authors> authorInterface);
    void LoadFics(QSharedPointer<interfaces::Fanfics> fanficsInterface);
    void LoadFavouritesDataFromDatabase(QSharedPointer<interfaces::Authors> authorInterface);
    void LoadStoredFavouritesData();
    void SaveFavouritesData();
    MatchedFics GetMatchedFics(UserMatchesInput user1, int user2);

    RecommendationListResult GetMatchedFicsForFavList(QHash<uint32_t, FicWeightPtr> fetchedFics,
                                                      QSharedPointer<core::RecommendationList> params,
                                                      genre_stats::GenreMoodData moodData = {});
    DataHolder holder;
};



}
BIND_TO_SELF_SINGLE(core::RecCalculator);
