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

