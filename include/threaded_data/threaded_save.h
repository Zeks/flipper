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
#include <QVector>
#include <QList>
#include <QHash>
#include <QFile>
#include <QDataStream>
#include <array>
#include "include/core/section.h"
#include "third_party/roaring/roaring.hh"
namespace thread_boost{

void SaveFicWeightCalcData(QString storageFolder,QVector<core::FicWeightPtr>& fics);
void SaveAuthorsData(QString storageFolder,QList<core::AuthorPtr>& authors);
void SaveFavouritesData(QString storageFolder, QHash<int, QSet<int>>& favourites);
void SaveFavouritesData(QString storageFolder, QHash<int, Roaring>& favourites);
void SaveGenreDataForFavLists(QString storageFolder, QHash<int, std::array<double, 22> > &genreData);
void SaveFandomDataForFavLists(QString storageFolder, QHash<int, core::AuthorFavFandomStatsPtr>& fandomLists);

void SaveData(QString storageFolder, QString fileName, QHash<int, Roaring>& favourites);
void SaveData(QString storageFolder, QString fileName, QHash<int, QSet<int>>& favourites);
void SaveData(QString storageFolder, QString fileName, QHash<int, std::array<double, 22> > &genreData);
void SaveData(QString storageFolder, QString fileName, QHash<int, core::AuthorFavFandomStatsPtr>& fandomLists);
void SaveData(QString storageFolder, QString fileName, QVector<core::FicWeightPtr>& fics);
void SaveData(QString storageFolder, QString fileName, QHash<int, core::FicWeightPtr>& fics);
void SaveData(QString storageFolder, QString fileName, QHash<int, QList<genre_stats::GenreBit>>& fics);
void SaveData(QString storageFolder, QString fileName, QHash<int, QString>& fics);
void SaveData(QString storageFolder, QString fileName, QHash<uint32_t, genre_stats::ListMoodData>& moods);

}
