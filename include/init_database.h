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
    QHash<QString, FavouritesPage> FetchRecommenders();
    bool FetchTrackStateForFandom(QString fandom, bool crossover);
    QStringList FetchTrackedFandoms();
    QStringList FetchTrackedCrossovers();
    void BackupDatabase();
    bool LoadIntoDB(Fic & section);

    bool UpdateInDB(Fic & section);
    bool InsertIntoDB(Fic & section);

    bool LoadRecommendationIntoDB(FavouritesPage &recommender, Fic &section);
    bool WriteRecommendation(Author &recommender, int id);
    void WriteRecommender(const FavouritesPage& recommender);
    //void WriteRecommendersMetainfo(const Recommender& recommender);
    void RemoveAuthor(const Author &recommender);
    void RemoveAuthor(int id);
    int GetFicIdByAuthorAndName(QString, QString);
    int GetFicIdByWebId(int);
    int GetAuthorIdFromUrl(QString url);
    int GetMatchCountForRecommenderOnList(int recommender_id, int list);
    void DropFanficIndexes();
    void RebuildFanficIndexes();
    void DropAllFanficIndexes();
    void RebuildAllFanficIndexes();
    WriteStats ProcessFicsIntoUpdateAndInsert(const QList<Fic>&);
    QDateTime GetMaxUpdateDateForFandom(QStringList sections);
    void InstallCustomFunctions();
    void EnsureFandomsFilled();
    void EnsureWebIdsFilled();
    void EnsureFFNUrlsShort();
    void ImportTags(QString anotherDatabase);
    //bool EnsureTagForRecommendations();
    QStringList ReadAvailableRecommendationLists();
    AuthorRecommendationStats CreateRecommenderStats(int recommenderId, int tag);
    QList<AuthorRecommendationStats> GetRecommenderStatsForTag(QString tag, QString sortOn = "author_id", QString order = "asc");
    bool HasNoneTagInRecommendations();
    bool WipeCurrentRecommenderRecsOnTag(QString tag);
    bool CopyAllRecommenderFicsToTag(int recommenderId, QString tag);
    QList<int>GetFulLRecommenderList();
    void WriteRecommenderStatsForTag(AuthorRecommendationStats stats);
    bool AssignNewNameForRecommenderId(FavouritesPage recommender);
    QVector<FavouritesPage> GetAllAuthors(QString website);
    QVector<int> GetAllFicIDsFromRecommendations(QString tag);
    bool UpdateTagStatsPerFic(QString tag);
    int GetFicDBIdByDelimitedSiteId(QString id);
    QStringList ObtainIdList(core::Query);
    void PassTagsIntoTagsTable();


}
