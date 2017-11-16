#pragma once
#include "Interfaces/base.h"
#include "Interfaces/db_interface.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace interfaces {

class RecommendationLists
{
public:
    void Clear();
    void ClearIndex();
    void ClearCache();
    void Reindex();
    void AddToIndex(core::RecPtr);

    bool EnsureList(int listId);
    bool EnsureList(QString name);

    core::RecPtr GetList(int id);
    core::RecPtr GetList(QString name);

    int GetListIdForName(QString name); //! todo do I need to fill index without filling lists
    QString GetListNameForId(int id);

    bool DeleteList(int listId);


    bool ReloadList(int listId);


    bool LoadAuthorRecommendationsIntoList(int authorId, int listId);
    bool LoadAuthorsForRecommendationList(int listId);
    void LoadAvailableRecommendationLists();
    bool LoadAuthorRecommendationStatsIntoDatabase(int listId, core::AuhtorStatsPtr stats);
    bool LoadListIntoDatabase(core::RecPtr);

    core::AuhtorStatsPtr CreateAuthorRecommendationStatsForList(int authorId, int listId);
    bool IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId);
    bool UpdateFicCountInDatabase(int listId);
    bool AddAuthorFavouritesToList(int authorId, int listId, bool reloadLocalData = false);
    bool IsAuthorInCurrentRecommendationSet(QString author);

    QStringList GetAllRecommendationListNames();
    QList<core::AuhtorStatsPtr> GetAuthorStatsForList(int id);
    core::AuhtorStatsPtr GetIndividualAuthorStatsForList(int id, int authorId);
    int GetMatchCountForRecommenderOnList(int authorId, int listId);
    QVector<int> GetAllFicIDs(int listId);
    QStringList GetNamesForListId(int listId);
    QList<QSharedPointer<core::Author>> GetAuthorsForRecommendationList(int listId);
    QList<int> GetRecommendersForFicId(int ficId);

    int GetCurrentRecommendationList() const;
    void SetCurrentRecommendationList(int value);



    QList<core::RecPtr> lists;

    int currentRecommendationList = -1;
    QHash<int, core::RecPtr> idIndex;
    QHash<QString, core::RecPtr> nameIndex;

    QHash<int, QList<core::AuhtorStatsPtr>> cachedAuthorStats;
    QHash<int, QVector<int>> ficsCacheForLists;
    QHash<int, QStringList> authorsCacheForLists;

    QSqlDatabase db;

    QSharedPointer<Authors> authorInterface;
    QSharedPointer<database::IDBWrapper> portableDBInterface;

    QHash<int, core::AuthorPtr> currentRecommenderSet;
private:
    void DeleteLocalList(int listId);

};

}
