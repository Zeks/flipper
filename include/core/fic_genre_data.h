#pragma once
#include <QList>
#include <QVector>
#include <QStringList>

namespace genre_stats
{

struct GenreBit
{
    QStringList genres;
    float relevance;
};

struct FicGenreData
{
    int totalLists = 0;
    int ficId = -1;
    int ffnId = -1;
    QString originalGenreString;
    float strengthHumor =0.0f;
    float strengthRomance =0.0f;
    float strengthDrama =0.0f;
    float strengthBonds =0.0f;
    float strengthHurtComfort =0.0f;
    float strengthNeutralComposite =0.0f;
    float strengthNeutralAdventure =0.0f;
    float pureDramaAdvantage = 0.0;
    QStringList originalGenres;
    QList<GenreBit> realGenres;
    QVector<GenreBit> processedGenres;
    QStringList genresToKeep;
};
}
