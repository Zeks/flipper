#pragma once
#include <QString>
#include "section.h"
#include "queryinterfaces.h"

namespace database{

struct WriteStats
{
    QList<Fic> requiresInsert;
    QList<Fic> requiresUpdate;
};
    bool ReadDbFile(QString file, QString connectionName = "");
    bool ReindexTable(QString table);
    void SetFandomTracked(QString fandom, bool crossover, bool);
    void PushFandom(QString);
    void RebaseFandoms();
    QStringList GetFandomListFromDB(QString);

    void AssignTagToFandom(QString tag, QString fandom);



    QStringList FetchRecentFandoms();
    QHash<QString, Recommender> FetchRecommenders(int limitingWave = 0, QString tag = "core");
    bool FetchTrackStateForFandom(QString fandom, bool crossover);
    QStringList FetchTrackedFandoms();
    QStringList FetchTrackedCrossovers();
    void BackupDatabase();
    bool LoadIntoDB(Fic & section);

    bool UpdateInDB(Fic & section);
    bool InsertIntoDB(Fic & section);

    bool LoadRecommendationIntoDB(Recommender &recommender, Fic &section);
    bool WriteRecommendation( Recommender& recommender, int id, QString tag = "none");
    void WriteRecommender(const Recommender& recommender);
    //void WriteRecommendersMetainfo(const Recommender& recommender);
    void RemoveRecommender(const Recommender& recommender);
    void RemoveRecommender(int id);
    int GetFicIdByAuthorAndName(QString, QString);
    int GetFicIdByWebId(int);
    int GetRecommenderId(QString url);
    int GetMatchCountForRecommenderOnTag(int recommender_id, QString tag);
    void DropFanficIndexes();
    void RebuildFanficIndexes();
    void DropAllFanficIndexes();
    void RebuildAllFanficIndexes();
    WriteStats ProcessSectionsIntoUpdateAndInsert(const QList<Fic>&);
    QDateTime GetMaxUpdateDateForSection(QStringList sections);
    void InstallCustomFunctions();
    void EnsureFandomsFilled();
    void EnsureWebIdsFilled();
    void EnsureFFNUrlsShort();
    void ImportTags(QString anotherDatabase);
    bool EnsureTagForRecommendations();
    QStringList ReadAvailableRecTagGroups();
    RecommenderStats CreateRecommenderStats(int recommenderId, QString tag);
    QList<RecommenderStats> GetRecommenderStatsForTag(QString tag, QString sortOn = "author_id", QString order = "asc");
    bool HasNoneTagInRecommendations();
    bool WipeCurrentRecommenderRecsOnTag(QString tag);
    bool CopyAllRecommenderFicsToTag(int recommenderId, QString tag);
    QList<int>GetFulLRecommenderList();
    void WriteRecommenderStatsForTag(RecommenderStats stats);
    bool AssignNewNameForRecommenderId(Recommender recommender);
    QVector<Recommender> GetAllAuthors(QString website);
    QVector<int> GetAllFicIDsFromRecommendations(QString tag);
    bool UpdateTagStatsPerFic(QString tag);
    int GetFicDBIdByDelimitedSiteId(QString id);
    QStringList ObtainIdList(core::Query);
    void PassTagsIntoTagsTable();


}
