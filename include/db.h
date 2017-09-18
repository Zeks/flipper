#pragma once
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"

namespace database {
class IDBPersistentData{
  public:
    virtual ~IDBPersistentData(){}
    virtual bool IsDataLoaded() = 0;
    virtual void Sync() = 0;
    bool isLoaded = false;
};

class IDBWebIDIndex{
  public:
    virtual ~IDBWebIDIndex(){}
    virtual int GetIDFromWebID(int) = 0;
    virtual int GetWebIDFromID(int) = 0;
    QHash<int, int> indexIDByWebID;
    QHash<int, int> indexWebIDByID;
};

class IDBFandoms{
public:
    virtual ~IDBFandoms(){}
    virtual bool IsTracked(QString) = 0;
    virtual QList<int> AllTracked() = 0;
    virtual QList<int> AllTrackedStr() = 0;
    virtual int GetID(QString) = 0;
    virtual void SetTracked(QString, bool value, bool immediate) = 0;
    virtual void IsTracked(QString) = 0;
    virtual QStringList ListOfTracked() = 0;
    QHash<QString, int> indexFandomsByName;
    QHash<QString, QSharedPointer<core::Fandom>> fandoms;
    QList<QSharedPointer<core::Fandom>> updateQueue;
};


class IDBFanfics : public IDBWebIDIndex, public IDBPersistentData {
public:
    virtual ~IDBFanfics(){}
    // queued by webid
    QHash<int, QSharedPointer<core::Fic>> queuedFics;

};

class IDBAuthors : public IDBWebIDIndex, public IDBPersistentData{
public:
    virtual ~IDBAuthors(){}

    // queued by webid
    QHash<int, QSharedPointer<core::Author>> authors;
    QList<QSharedPointer<core::Author>> queue;
    QHash<int, QList<int>> queuedFicRecommendations; // author ID to fic WEB ID
};


class DataInterfaces
{
    QScopedPointer<IDBFandoms> fandoms;
    QScopedPointer<IDBFanfics> fanfics;
    QScopedPointer<IDBAuthors> authors;
};



class IDB
{
public:
    virtual IDB(){}

    virtual void BackupDatabase() = 0;

    virtual bool ReadDbFile(QString file, QString connectionName = "") = 0;
    virtual bool ReindexTable(QString table) = 0;
    virtual void SetFandomTracked(QString fandom, bool crossover, bool) = 0;
    virtual void PushFandom(QString) = 0;
    virtual void RebaseFandoms() = 0;


    // writing fics into database
    virtual bool LoadIntoDB(core::Fic & section) = 0;
    virtual bool UpdateInDB(core::Fic & section) = 0;
    virtual bool InsertIntoDB(core::Fic & section) = 0;
    virtual bool WriteFandomsForStory(core::Fic & section, QHash<QString, int> &) = 0;

    virtual bool WriteRecommendationIntoDB(core::FavouritesPage &recommender, core::Fic &section) = 0;
    virtual bool WriteRecommendation(core::Author &recommender, int id) = 0;
    virtual void WriteRecommender(const core::Author &recommender) = 0;

    virtual void RemoveAuthor(const core::Author &recommender) = 0;
    virtual void RemoveAuthor(int id) = 0;

    virtual int GetFicIdByAuthorAndName(QString, QString) = 0;
    virtual int GetFicIdByWebId(QString website, int) = 0;
    virtual int GetAuthorIdFromUrl(QString url) = 0;
    virtual int GetMatchCountForRecommenderOnList(int recommender_id, int list) = 0;
    virtual QVector<int> GetAllFicIDsFromRecommendationList(QString tag) = 0;
    virtual QVector<core::Author> GetAllAuthors(QString website) = 0;
    virtual QList<int> GetFulLRecommenderList() = 0;
    virtual QList<core::RecommendationList> GetAvailableRecommendationLists() = 0;
    virtual QList<core::AuthorRecommendationStats> GetRecommenderStatsForList(QString tag, QString sortOn = "author_id", QString order = "asc") = 0;
    virtual QStringList GetIdListForQuery(core::Query) = 0;
    virtual QDateTime GetMaxUpdateDateForFandom(QStringList sections) = 0;
    virtual QStringList GetFandomListFromDB(QString) = 0;


    virtual QStringList FetchRecentFandoms() = 0;
    virtual QHash<QString, core::Author> FetchRecommenders() = 0;
    virtual bool FetchTrackStateForFandom(QString fandom, bool crossover) = 0;
    virtual QStringList FetchTrackedFandoms() = 0;
    virtual QStringList FetchTrackedCrossovers() = 0;

    virtual void DropAllFanficIndexes() = 0;
    virtual void RebuildAllFanficIndexes() = 0;

    virtual WriteStats ProcessFicsIntoUpdateAndInsert(const QList<core::Fic>&, bool alwaysUpdateIfNotInsert = false) = 0;

    virtual void InstallCustomFunctions() = 0;
    virtual void EnsureFandomsFilled() = 0;
    virtual void EnsureWebIdsFilled() = 0;
    virtual void EnsureFFNUrlsShort() = 0;
    virtual bool EnsureFandomsNormalized() = 0;
    virtual void CalculateFandomAverages() = 0;
    virtual void CalculateFandomFicCounts() = 0;

    virtual void ImportTags(QString anotherDatabase) = 0;

    // recommendation stats
    virtual core::AuthorRecommendationStats CreateRecommenderStats(int recommenderId, core::RecommendationList list) = 0;
    virtual void WriteRecommenderStatsForTag(core::AuthorRecommendationStats stats, int listId) = 0;
    virtual bool AssignNewNameForRecommenderId(core::Author recommender) = 0;

    // recommendation ists
    virtual bool CreateOrUpdateRecommendationList(core::RecommendationList&) = 0;
    virtual bool UpdateFicCountForRecommendationList(core::RecommendationList&) = 0;
    virtual int GetRecommendationListIdForName(QString name) = 0;
    //bool WipeRecommendationList(QString tag) = 0;
    virtual bool CopyAllAuthorRecommendationsToList(int recommenderId, QString listName) = 0;
    virtual bool IncrementAllValuesInListMatchingAuthorFavourites(int recommenderId, QString tag) = 0;
    virtual bool DeleteRecommendationList(QString listName) = 0;
    //bool WriteListFandoms(int id, QList<>) = 0;

    virtual void DeleteTagfromDatabase(QString) = 0;

    virtual void AssignTagToFandom(QString tag, QString fandom) = 0;
    virtual void PassTagsIntoTagsTable() = 0;
    virtual bool HasNoneTagInRecommendations() = 0;
    //void WriteRecommendersMetainfo(const Recommender& recommender) = 0;
    //bool EnsureTagForRecommendations() = 0;
    //int GetFicDBIdByDelimitedSiteId(QString id) = 0;
    virtual bool GetAllFandoms(QHash<QString, int>&) = 0;
    virtual int CreateIDForFandom(core::Fandom) = 0;
    virtual bool EnsureFandomExists(core::Fandom, QHash<QString, int>&) = 0;
    virtual int GetLastIdForTable(QString) = 0;
    virtual bool IsGenreList(QStringList, QString website) = 0;
    virtual bool ReprocessFics(QString where,QString website, std::function<void(int)>) = 0;
    virtual void TryDeactivate(QString url, QString website) = 0;
    virtual void DeactivateStory(int id, QString website) = 0;

    DataInterfaces data;
};

}
