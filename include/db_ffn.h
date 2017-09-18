#pragma once
#include <QString>
#include <QSqlDatabase>
#include <functional>
#include "section.h"
#include "db.h"
#include "queryinterfaces.h"

namespace database{
struct WriteStats
{
    QList<core::Fic> requiresInsert;
    QList<core::Fic> requiresUpdate;
    bool HasData(){return requiresInsert.size() > 0 || requiresUpdate.size() > 0;}
};

class FFNFandoms : public IDBFandoms{
public:
    virtual ~FFNFandoms(){}
    virtual bool IsTracked(QString);
    virtual QList<int> AllTracked();
    virtual QList<int> AllTrackedStr();
    virtual int GetID(QString);
    virtual void SetTracked(QString, bool value, bool immediate);
    virtual void IsTracked(QString);
    virtual QStringList ListOfTracked();
    virtual bool IsDataLoaded();
    virtual void Sync();
    QSqlDatabase db;
};

class FFNFanfics : public IDBFanfics{
public:
    virtual ~IDBFanfics(){}
    virtual int GetIDFromWebID(int);
    virtual int GetWebIDFromID(int);
};

class FFN : public IDB
{
    public:
    void BackupDatabase();

    bool ReadDbFile(QString file, QString connectionName = "");
    bool ReindexTable(QString table);
    void SetFandomTracked(QString fandom, bool);
    void PushFandom(QString);
    void RebaseFandoms();


    // writing fics into database
    bool LoadIntoDB(core::Fic & section);
    bool UpdateInDB(core::Fic & section);
    bool InsertIntoDB(core::Fic & section);
    bool WriteFandomsForStory(core::Fic & section, QHash<QString, int> &);

    bool WriteRecommendationIntoDB(core::FavouritesPage &recommender, core::Fic &section);
    bool WriteRecommendation(core::Author &recommender, int id);
    void WriteRecommender(const core::Author &recommender);

    void RemoveAuthor(const core::Author &recommender);
    void RemoveAuthor(int id);

    int GetFicIdByAuthorAndName(QString, QString);
    int GetFicIdByWebId(QString website, int);
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
    bool FetchTrackStateForFandom(QString fandom);
    QStringList FetchTrackedFandoms();

    void DropAllFanficIndexes();
    void RebuildAllFanficIndexes();

    WriteStats ProcessFicsIntoUpdateAndInsert(const QList<core::Fic>&, bool alwaysUpdateIfNotInsert = false);

    void InstallCustomFunctions();
    void EnsureFandomsFilled();
    void EnsureWebIdsFilled();
    void EnsureFFNUrlsShort();
    bool EnsureFandomsNormalized();
    void CalculateFandomAverages();
    void CalculateFandomFicCounts();

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
    //bool WriteListFandoms(int id, QList<>);

    void DeleteTagfromDatabase(QString);

    void AssignTagToFandom(QString tag, QString fandom);
    void PassTagsIntoTagsTable();
    bool HasNoneTagInRecommendations();
    //void WriteRecommendersMetainfo(const Recommender& recommender);
    //bool EnsureTagForRecommendations();
    //int GetFicDBIdByDelimitedSiteId(QString id);
    bool GetAllFandoms(QHash<QString, int>&);
    int CreateIDForFandom(core::Fandom);
    bool EnsureFandomExists(core::Fandom, QHash<QString, int>&);
    int GetLastIdForTable(QString);
    bool IsGenreList(QStringList, QString website);
    bool ReprocessFics(QString where,QString website, std::function<void(int)>);
    void TryDeactivate(QString url, QString website);
    void DeactivateStory(int id, QString website);

    DataInterfaces data;
    QSqlDatabase db;
    QString dbName = "CrawlerDB";
    QSqlDatabase GetDb() const;
    void SetDb(const QSqlDatabase &value);
};
}
