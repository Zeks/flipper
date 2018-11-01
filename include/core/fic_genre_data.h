#pragma once
#include <QList>
#include <QVector>
#include <QStringList>
#include "logger/QsLog.h"

namespace genre_stats
{

struct GenreBit
{
    QStringList genres;
    float relevance;
    void Log(){
        qDebug() << "genres:" << genres << " relevance: " << relevance;
    }
};

struct FicGenreData
{
    void Reset(){
        totalLists = 0;
        ficId = -1;
        ffnId = -1;
        originalGenreString = "";
        strengthHumor =0.0f;
        strengthRomance =0.0f;
        strengthDrama =0.0f;
        strengthBonds =0.0f;
        strengthHurtComfort =0.0f;
        strengthNeutralComposite =0.0f;
        strengthNeutralAdventure =0.0f;
        pureDramaAdvantage = 0.0;
        originalGenres.clear();
        realGenres.clear();
        processedGenres.clear();
        genresToKeep.clear();
    }
    void Log(){
        qDebug() << "Weird genre token";
        qDebug() << "fic: " << ficId;
        qDebug() << "originals: " << originalGenres;
        qDebug() << "kept: " << genresToKeep;
        qDebug() << "dumping real genres:";
        for(auto realGenre: realGenres)
            realGenre.Log();
        qDebug() << " ";
        qDebug() << "dumping processed genres:";
        for(auto genre: processedGenres)
            genre.Log();
    }
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


struct ListMoodData
{
    int listId = -1;
    float strengthNone =0.0f;
    float strengthNeutral=0.0f;
    float strengthNonNeutral=0.0f;

    float strengthFlirty =0.0f;
    float strengthNonFlirty =0.0f;

    float strengthBondy =0.0f;
    float strengthNonBondy =0.0f;

    float strengthFunny =0.0f;
    float strengthNonFunny =0.0f;

    float strengthDramatic =0.0f;
    float strengthNonDramatic =0.0f;

    float strengthShocky =0.0f;
    float strengthNonShocky =0.0f;

    float strengthHurty =0.0f;
    float strengthNonHurty =0.0f;

    float strengthOther =0.0f;
};
}
