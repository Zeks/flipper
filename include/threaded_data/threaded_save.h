#pragma once
#include <QVector>
#include <QList>
#include <QHash>
#include <QFile>
#include <QDataStream>
#include <array>
#include "include/core/section.h"

namespace thread_boost{
void SaveFicWeightCalcData(QString storageFolder,QVector<core::FicWeightPtr>& fics);
void SaveAuthorsData(QString storageFolder,QList<core::AuthorPtr>& authors);
void SaveFavouritesData(QString storageFolder, QHash<int, QSet<int>>& favourites);
void SaveGenreDataForFavLists(QString storageFolder, QHash<int, std::array<double, 22> > &genreData);
void SaveFandomDataForFavLists(QString storageFolder, QHash<int, core::AuthorFavFandomStatsPtr>& fandomLists);
}
