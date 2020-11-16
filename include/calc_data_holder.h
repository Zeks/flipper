/*
Flipper is a recommendation and search engine for fanfiction.net
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
struct ListWithIdentifier{
    int id;
    QSet<int>  favourites;
};
struct CalcDataHolder{
    QVector<core::FicWeightPtr> fics;
    QList<core::AuthorPtr>  authors;

    QHash<int, QSet<int> > favourites;
    QHash<int, std::array<double, 22> > genreData;
    QHash<int, core::AuthorFavFandomStatsPtr> fandomLists;
    QVector<ListWithIdentifier> filteredFavourites;
    QFile data;
    QDataStream out;
    int fileCounter = 0;
    void Clear();

    void ResetFileCounter();


    void SaveToFile();
    void LoadFromFile();

    void SaveFicsData();
    void LoadFicsData();
};

