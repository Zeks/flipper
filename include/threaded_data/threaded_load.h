#pragma once
#include <QVector>
#include <QList>
#include <QHash>
#include <QFile>
#include <QDataStream>

#include "include/core/section.h"

namespace thread_boost{
void LoadFicWeightCalcData(QString storage, QVector<core::FicWeightPtr>& fics);
void LoadAuthorsData(QString storage,QList<core::AuthorPtr> &authors);
void LoadFavouritesData(QString storage, QHash<int, QSet<int>>& favourites);
void LoadGenreDataForFavLists(QString storage, QHash<int, std::array<double, 22> >& genreData);
void LoadFandomDataForFavLists(QString storage, QHash<int, core::AuthorFavFandomStatsPtr>& fandomLists);
}
