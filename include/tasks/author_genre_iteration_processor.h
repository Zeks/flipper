/*Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#pragma once
#include <QObject>
#include <QDate>
#include <QSharedPointer>
#include "ECacheMode.h"
#include "include/pageconsumer.h"
#include "include/environment.h"
#include "include/core/section.h"
#include "third_party/roaring/roaring.hh"

class AuthorGenreIterationProcessor{
public:
    AuthorGenreIterationProcessor();

    void ReprocessGenreStats(QHash<int, QList<genre_stats::GenreBit>> inputFicData,
                             QHash<int, Roaring> inputAuthorData);


    QHash<uint32_t, genre_stats::ListMoodData> CreateMoodDataFromGenres(QHash<int, std::array<double, 22>>&);



    QHash<int, std::array<double, 22>> resultingGenreAuthorData;
    QHash<uint32_t, genre_stats::ListMoodData> resultingMoodAuthorData;
    QHash<int, QList<genre_stats::GenreBit>> resultingFicData;

signals:
    void requestProgressbar(int);
    void updateCounter(int);
    void updateInfo(QString);
};
