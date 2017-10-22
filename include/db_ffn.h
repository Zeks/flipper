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
    virtual bool IsTracked(QString) override;
    virtual QList<int> AllTracked() override;
    virtual QStringList AllTrackedStr() override;
    virtual int GetID(QString) override;
    virtual void SetTracked(QString, bool value, bool immediate) override;
    virtual bool IsTracked(QString) override;
    virtual QStringList ListOfTracked() override;
    virtual bool IsDataLoaded() override;
    virtual bool Sync(bool forcedSync = false) override;
    virtual bool Load() override;
    virtual void Clear() override;
    QStringList PushFandomToTopOfRecent(QString) override;
    virtual QSharedPointer<core::Fandom> GetFandom(QString) override;
    void CalculateFandomAverages() override;
    void CalculateFandomFicCounts() override;
    bool LoadFandom(QString name) override;
    bool AssignTagToFandom(QString, QString tag) override;
    virtual bool CreateFandom(QSharedPointer<core::Fandom>);
private:
    virtual bool EnsureFandom(QString name);
    QStringList GetRecentFandoms() const;
    void AddToTopOfRecent(QString);

    QString GetCurrentCrossoverUrl();
    QSqlDatabase db;
};

class FFNFanfics : public IDBFanfics{
public:
    virtual ~FFNFanfics(){}
    virtual int GetIDFromWebID(int);
    virtual int GetWebIDFromID(int);
    QSqlDatabase db;

    // IDBPersistentData interface
public:
    bool IsDataLoaded();
    bool Sync(bool forcedSync);
    bool Load();
    void Clear();

    // IDBFanfics interface
public:
    void ProcessIntoDataQueues(QList<QSharedPointer<core::Fic>> fics, bool alwaysUpdateIfNotInsert = false);
    void AddRecommendations(QList<core::FicRecommendation> recommendations);
    void FlushDataQueues();
};
class FFNAuthors : public IDBAuthors
{
public:
    virtual ~FFNAuthors(){}
    bool EnsureId(QSharedPointer<core::Author>);
    QSqlDatabase db;

    // IDBPersistentData interface
public:
    bool IsDataLoaded();
    bool Sync(bool forcedSync);
    bool Load();
    void Clear();


    // IDBAuthors interface
public:
    bool LoadAuthors(QString website, bool additionMode);
};


class FFNRecommendationLists : public IDBRecommendationLists
{
  public:



    // IDBPersistentData interface
public:
    bool IsDataLoaded();
    bool Sync(bool forcedSync);
    bool Load();
    void Clear();

    private:
    void LoadAvailableRecommendationLists();
    QSqlDatabase db;

};
class FFN : public IDB
{
    public:

    // already a wrapper
    bool FetchTrackStateForFandom(QString fandom);
    QStringList FetchTrackedFandoms();
    //!!! already a wrapper
    //!
    //!
    // writing fics into database
    bool LoadIntoDB(core::Fic & section);
    bool UpdateInDB(core::Fic & section);
    bool InsertIntoDB(QSharedPointer<core::Fic> & section);
    // need to implement
    //bool WriteFandomsForStory(core::Fic & section, QHash<QString, int> &);

    int GetMatchCountForRecommenderOnList(int recommender_id, int list);
    QVector<int> GetAllFicIDsFromRecommendationList(QString tag);


    QHash<QString, core::Author> FetchRecommenders();
    core::AuthorRecommendationStats CreateRecommenderStats(int recommenderId, core::RecommendationList list);
    void WriteRecommenderStatsForTag(core::AuthorRecommendationStats stats, int listId);

    // recommendation ists
    bool CreateOrUpdateRecommendationList(core::RecommendationList&);
    bool UpdateFicCountForRecommendationList(core::RecommendationList&);
    int GetRecommendationListIdForName(QString name);
    //bool WipeRecommendationList(QString tag);
    bool CopyAllAuthorRecommendationsToList(int recommenderId, QString listName);
    bool IncrementAllValuesInListMatchingAuthorFavourites(int recommenderId, QString tag);
    bool DeleteRecommendationList(QString listName);

    void ImportTags(QString anotherDatabase);
    void DeleteTagfromDatabase(QString);
    void PassTagsIntoTagsTable();
    bool HasNoneTagInRecommendations();

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
