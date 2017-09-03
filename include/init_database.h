#pragma once
#include <QString>
#include "section.h"
#include "queryinterfaces.h"

namespace database{

struct WriteStats
{
    QList<core::Fic> requiresInsert;
    QList<core::Fic> requiresUpdate;
};
    void BackupDatabase();

    bool ReadDbFile(QString file, QString connectionName = "");
    bool ReindexTable(QString table);
    void SetFandomTracked(QString fandom, bool crossover, bool);
    void PushFandom(QString);
    void RebaseFandoms();


    // writing fics into database
    bool LoadIntoDB(core::Fic & section);
    bool UpdateInDB(core::Fic & section);
    bool InsertIntoDB(core::Fic & section);

    bool WriteRecommendationIntoDB(core::FavouritesPage &recommender, core::Fic &section);
    bool WriteRecommendation(core::Author &recommender, int id);
    void WriteRecommender(const core::Author &recommender);

    void RemoveAuthor(const core::Author &recommender);
    void RemoveAuthor(int id);

    int GetFicIdByAuthorAndName(QString, QString);
    int GetFicIdByWebId(int);
    int GetAuthorIdFromUrl(QString url);
    int GetMatchCountForRecommenderOnList(int recommender_id, int list);
    QVector<int> GetAllFicIDsFromRecommendationList(QString tag);
    QVector<core::Author> GetAllAuthors(QString website);
    QList<int> GetFulLRecommenderList();
    QList<core::RecommendationList> GetAvailableRecommendationLists();
    QList<core::AuthorRecommendationStats> GetRecommenderStatsForList(QString tag, QString sortOn = "author_id", QString order = "asc");
    QStringList GetIdListForQuery(core::Query);
    QDateTime GetMaxUpdateDateForFandom(QStringList sections);
    QStringList GetFandomListFromDB(QString);


    QStringList FetchRecentFandoms();
    QHash<QString, core::Author> FetchRecommenders();
    bool FetchTrackStateForFandom(QString fandom, bool crossover);
    QStringList FetchTrackedFandoms();
    QStringList FetchTrackedCrossovers();

    void DropAllFanficIndexes();
    void RebuildAllFanficIndexes();

    WriteStats ProcessFicsIntoUpdateAndInsert(const QList<core::Fic>&);

    void InstallCustomFunctions();
    void EnsureFandomsFilled();
    void EnsureWebIdsFilled();
    void EnsureFFNUrlsShort();

    void ImportTags(QString anotherDatabase);

    // recommendation stats
    core::AuthorRecommendationStats CreateRecommenderStats(int recommenderId, core::RecommendationList list);
    void WriteRecommenderStatsForTag(core::AuthorRecommendationStats stats, int listId);
    bool AssignNewNameForRecommenderId(core::Author recommender);

    // recommendation ists
    bool CreateOrUpdateRecommendationList(core::RecommendationList&);
    bool UpdateFicCountForRecommendationList(core::RecommendationList&);
    int GetRecommendationListIdForName(QString name);
    //bool WipeRecommendationList(QString tag);
    bool CopyAllAuthorRecommendationsToList(int recommenderId, QString listName);
    bool IncrementAllValuesInListMatchingAuthorFavourites(int recommenderId, QString tag);
    bool DeleteRecommendationList(QString listName);

    void DeleteTagfromDatabase(QString);

    void AssignTagToFandom(QString tag, QString fandom);
    void PassTagsIntoTagsTable();
    bool HasNoneTagInRecommendations();
    //void WriteRecommendersMetainfo(const Recommender& recommender);
    //bool EnsureTagForRecommendations();
    //int GetFicDBIdByDelimitedSiteId(QString id);


}
