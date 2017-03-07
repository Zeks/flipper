#pragma once
#include <QString>
#include "section.h"

namespace database{

struct WriteStats
{
    QList<Section> requiresInsert;
    QList<Section> requiresUpdate;
};
    bool ReadDbFile();
    bool ReindexTable(QString table);
    void SetFandomTracked(QString fandom, bool crossover, bool);
    void PushFandom(QString);
    void RebaseFandoms();
    QStringList FetchRecentFandoms();
    QHash<QString, Recommender> FetchRecommenders(int limitingWave = 0);
    bool FetchTrackStateForFandom(QString fandom, bool crossover);
    QStringList FetchTrackedFandoms();
    QStringList FetchTrackedCrossovers();
    void BackupDatabase();
    bool LoadIntoDB(Section & section);

    bool UpdateInDB(Section & section);
    bool InsertIntoDB(Section & section);

    bool LoadRecommendationIntoDB(Recommender &recommender, Section &section);
    bool WriteRecommendation( Recommender& recommender, int id);
    void WriteRecommender(const Recommender& recommender);
    void RemoveRecommender(const Recommender& recommender);
    void RemoveRecommender(int id);
    int GetFicIdByAuthorAndName(QString, QString);
    int GetRecommenderId(QString url);
    int FilterRecommenderByRecField(int, int);
    void DropFanficIndexes();
    void RebuildFanficIndexes();
    void DropAllFanficIndexes();
    void RebuildAllFanficIndexes();
    WriteStats ProcessSectionsIntoUpdateAndInsert(const QList<Section>&);

}
