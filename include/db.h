#pragma once
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace database {



class IDBWrapper;
class IDBPersistentData{
  public:
    virtual ~IDBPersistentData(){}
    virtual bool IsDataLoaded() = 0;
    virtual bool Sync(bool forcedSync = false) = 0;
    virtual bool Load() = 0;
    virtual void Clear() = 0;
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

class IDbService
{
public:
    void DropAllFanficIndexes() = 0;
    void RebuildAllFanficIndexes() = 0;
    void EnsureWebIdsFilled() = 0;
    void EnsureFFNUrlsShort() = 0;

};


class ITags{
public:
    bool DeleteTag(QString);
    bool AddTag();
    virtual bool AssignTagToFandom(QString, QString tag) = 0;

    QSharedPointer<IDBFandoms> fandomInterface;
    QSqlDatabase db;
};

class IDBFandoms : public IDBPersistentData{
public:
    virtual ~IDBFandoms(){}
    virtual bool IsTracked(QString) = 0;
    virtual QList<int> AllTracked() = 0;
    virtual QStringList AllTrackedStr() = 0;
    virtual int GetID(QString) = 0;
    virtual void SetTracked(QString, bool value, bool immediate) = 0;
    virtual bool IsTracked(QString) = 0;
    virtual QStringList ListOfTracked() = 0;
    virtual QString LoadFandom(QString name) = 0;
    virtual bool EnsureFandom(QString name) = 0;
    virtual bool AssignTagToFandom(QString, QString tag) = 0;
    //recent fandoms
    virtual QStringList PushFandomToTopOfRecent(QString) = 0;
    //virtual QStringList FetchRecentFandoms() = 0;
    //virtual void RebaseFandomsToZero() = 0;
    virtual QSharedPointer<core::Fandom> GetFandom(QString) = 0;
    virtual bool CreateFandom(QSharedPointer<core::Fandom>) = 0;

    virtual void CalculateFandomAverages() = 0;
    virtual void CalculateFandomFicCounts() = 0;

    QHash<QString, int> indexFandomsByName;
    QHash<QString, QSharedPointer<core::Fandom>> fandoms;
    QList<QSharedPointer<core::Fandom>> updateQueue;
    QList<QSharedPointer<core::Fandom>> recentFandoms;
    QSqlDatabase db;

};


class IDBFanfics : public IDBWebIDIndex, public IDBPersistentData {
public:
    virtual ~IDBFanfics(){}
    void ClearQueues() {
        updateQueue.clear();
        insertQueue.clear();
    }
    virtual void ProcessIntoDataQueues(QList<QSharedPointer<core::Fic>> fics, bool alwaysUpdateIfNotInsert = false) = 0;
    virtual void AddRecommendations(QList<core::FicRecommendation> recommendations) = 0;
    virtual void FlushDataQueues() = 0;
    virtual void EmptyQueues() { return !(updateQueue.size() || insertQueue.size());}

    bool ReprocessFics(QString where, QString website, std::function<void(int)> f);
    bool DeactivateFic(int ficId);
    // queued by webid
    QReadWriteLock mutex;
    QHash<int, QSharedPointer<core::Fic>> updateQueue;
    QHash<int, QSharedPointer<core::Fic>> insertQueue;
    QList<core::FicRecommendation> ficRecommendations;
    QSharedPointer<IDBAuthors> authorInterface;
    QSqlDatabase db;

};

class IDBAuthors : public IDBPersistentData{
public:
    virtual ~IDBAuthors(){}
    void Reindex();
    void IndexAuthors();
    void ClearIndex();
    virtual bool EnsureId(QSharedPointer<core::Author>) = 0;
    virtual bool LoadAuthors(QString website, bool additionMode = false) = 0;

    //for the future, not strictly necessary atm
//    bool LoadAdditionalInfo(QSharedPointer<core::Author>) = 0;
//    bool LoadAdditionalInfo() = 0;

    QSharedPointer<QSharedPointer<core::Author>> GetSingleByName(QString name, QString website);
    QList<QSharedPointer<QSharedPointer<core::Author>>> GetAllByName(QString name);
    QSharedPointer<core::Author> GetByUrl(QString url);
    QSharedPointer<core::Author> GetById(int id);
    QList<QSharedPointer<core::Author>> GetAllAuthors(QString website) = 0;
    int GetFicCount(int authorId);
    int GetCountOfRecsForTag(int authorId, QString tag);
    QSharedPointer<core::AuthorRecommendationStats> GetStatsForTag(authorId, core::RecommendationList list);


    // queued by webid
    QList<QSharedPointer<core::Author>> authors;

    //QMultiHash<QString, QSharedPointer<core::Author>> authorsByName;
    QHash<QString, QHash<QString, QSharedPointer<core::Author>>> authorsNamesByWebsite;
    QHash<int, QSharedPointer<core::Author>> authorsById;
    QHash<int, QSharedPointer<core::Author>> authorsByUrl;

    QHash<int, QList<QSharedPointer<core::AuthorRecommendationStats>>> cachedAuthorToTagStats;
    QSqlDatabase db;
};

class IDBRecommendationLists : public IDBPersistentData
{
public:
    int GetListIdForName(QString name); //! todo do I need to fill index without filling lists
    int GetListNameForId(int id);
    void Reindex();
    void IndexLists();
    void ClearIndex();
    bool DeleteList(int listId);
    bool ReloadList(int listId);
    void AddList(QSharedPointer<core::RecommendationList>);
    QSharedPointer<core::RecommendationList> NewList();
    QList<QSharedPointer<core::AuthorRecommendationStats>> GetAuthorStatsForList(int id);
    QSharedPointer<core::AuthorRecommendationStats> GetIndividualAuthorStatsForList(int id, int authorId);
    int GetMatchCountForRecommenderOnList(int authorId, int listId);
    QVector<int> GetAllFicIDs(int listId);
    QSharedPointer<core::AuthorRecommendationStats> CreateAuthorRecommendationStatsForList(int authorId, int listId);
    bool LoadAuthorRecommendationsIntoList(int authorId, int listId);
    bool LoadAuthorRecommendationStatsIntoDatabase(int listId, QSharedPointer<core::AuthorRecommendationStats> stats);
    bool LoadListIntoDatabase(QSharedPointer<core::RecommendationList>);
    bool UpdateFicCountInDatabase(int listId);
    bool AddAuthorFavouritesToList(int authorId, int listId, bool reloadLocalData = false);

    QList<QSharedPointer<core::RecommendationList>> lists;
    QHash<int, QSharedPointer<core::RecommendationList>> idIndex;
    QHash<QString, QSharedPointer<core::RecommendationList>> nameIndex;
    QHash<int, QList<QSharedPointer<core::AuthorRecommendationStats>>> cachedAuthorStats;
    QHash<int, QVector<int>> ficsCacheForLists;
    QSqlDatabase db;

    QSharedPointer<IDBAuthors> authorInterface;
    QSharedPointer<IDBWrapper> portableDBInterface;
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
    virtual void PushFandomToTopOfRecent(QString) = 0;
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

void RemoveAuthor(const core::Author &recommender)
{
    int id = GetAuthorIdFromUrl(recommender.url("ffn"));
    RemoveAuthor(id);
}
void RemoveAuthor(int id)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = "delete from recommendations where recommender_id = %1";
    qsl=qsl.arg(QString::number(id));
    q1.prepare(qsl);
    ExecAndCheck(q1);

    QSqlQuery q2(db);
    qsl = "delete from recommenders where id = %1";
    qsl=qsl.arg(id);
    q2.prepare(qsl);
    ExecAndCheck(q2);
}
