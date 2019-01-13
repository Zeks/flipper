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

#include "include/core/section.h"
#include "third_party/roaring/roaring.hh"

namespace thread_boost{
void LoadFicWeightCalcData(QString storage, QVector<core::FicWeightPtr>& fics);
void LoadAuthorsData(QString storage,QList<core::AuthorPtr> &authors);
void LoadFavouritesData(QString storage, QHash<int, QSet<int>>& favourites);
void LoadFavouritesData(QString storage, QHash<int, Roaring>& favourites);
void LoadGenreDataForFavLists(QString storage, QHash<int, std::array<double, 22> >& genreData);
void LoadFandomDataForFavLists(QString storage, QHash<int, core::AuthorFavFandomStatsPtr>& fandomLists);


void LoadData(QString storageFolder, QString fileName, QHash<int, Roaring>& );
void LoadData(QString storageFolder, QString fileName, QHash<int, QSet<int>>& );
void LoadData(QString storageFolder, QString fileName, QHash<int, std::array<double, 22> > &);
void LoadData(QString storageFolder, QString fileName, QHash<int, core::AuthorFavFandomStatsPtr>& );
void LoadData(QString storageFolder, QString fileName, QVector<core::FicWeightPtr>& );
void LoadData(QString storageFolder, QString fileName, QHash<int, core::FicWeightPtr>& );
void LoadData(QString storageFolder, QString fileName, QHash<int, QList<genre_stats::GenreBit>>& );
void LoadData(QString storageFolder, QString fileName, QHash<int, QString>& );
}
