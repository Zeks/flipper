#pragma once
#include "Interfaces/base.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace database {

class DBRecommendationListsBase : public IDBPersistentData
{
public:
    int GetListIdForName(QString name); //! todo do I need to fill index without filling lists
    QString GetListNameForId(int id);
    void Reindex();
    void IndexLists();
    void ClearIndex();
    void Clear();
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
    void LoadAvailableRecommendationLists();
    QList<QSharedPointer<core::RecommendationList>> lists;
    QHash<int, QSharedPointer<core::RecommendationList>> idIndex;
    QHash<QString, QSharedPointer<core::RecommendationList>> nameIndex;
    QHash<int, QList<QSharedPointer<core::AuthorRecommendationStats>>> cachedAuthorStats;
    QHash<int, QVector<int>> ficsCacheForLists;
    QSqlDatabase db;

    QSharedPointer<DBAuthorsBase> authorInterface;
    QSharedPointer<IDBWrapper> portableDBInterface;
};

}
