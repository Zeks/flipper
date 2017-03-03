#pragma once
#include <QString>
#include "section.h"

namespace database{
    bool ReadDbFile();
    bool ReindexTable(QString table);
    void SetFandomTracked(QString fandom, bool crossover, bool);
    void PushFandom(QString);
    void RebaseFandoms();
    QStringList FetchRecentFandoms();
    bool FetchTrackStateForFandom(QString fandom, bool crossover);
    QStringList FetchTrackedFandoms();
    QStringList FetchTrackedCrossovers();
    void BackupDatabase();
    bool LoadIntoDB(Section & section);
    bool LoadRecommendationIntoDB(const Recommender& recommender, Section &section);
    bool WriteRecommendation(const Recommender& recommender, int id);
    void WriteRecommender(const Recommender& recommender);
    int GetFicIdByAuthorAndName(QString, QString);
    int GetRecommenderId(QString);
}
