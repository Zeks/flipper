#pragma once
#include "Interfaces/base.h"
#include "Interfaces/db_interface.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace interfaces {

class RecommendationLists : public IDBPersistentData
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
    QStringList GetNamesForListId(int listId);
    QSharedPointer<core::AuthorRecommendationStats> CreateAuthorRecommendationStatsForList(int authorId, int listId);
    bool LoadAuthorRecommendationsIntoList(int authorId, int listId);
    bool IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId);
    bool LoadAuthorRecommendationStatsIntoDatabase(int listId, QSharedPointer<core::AuthorRecommendationStats> stats);
    bool LoadListIntoDatabase(QSharedPointer<core::RecommendationList>);
    bool UpdateFicCountInDatabase(int listId);
    bool AddAuthorFavouritesToList(int authorId, int listId, bool reloadLocalData = false);
    void LoadAvailableRecommendationLists();
    bool EnsureList(int listId);
    bool EnsureList(QString name);
    bool LoadAuthorsForRecommendationList(int listId);
    QList<QSharedPointer<core::Author>> GetAuthorsForRecommendationList(int listId);
    bool IsAuthorInCurrentRecommendationSet(QString author);
    int GetCurrentRecommendationList() const;
    void SetCurrentRecommendationList(int value);
    QStringList GetAllRecommendationListNames();
    QSharedPointer<core::RecommendationList> GetList(int id);
    QSharedPointer<core::RecommendationList> GetList(QString name);
    QList<QSharedPointer<core::RecommendationList>> lists;
    QHash<int, QSharedPointer<core::RecommendationList>> idIndex;
    QHash<QString, QSharedPointer<core::RecommendationList>> nameIndex;
    QHash<int, QList<QSharedPointer<core::AuthorRecommendationStats>>> cachedAuthorStats;
    QHash<int, QVector<int>> ficsCacheForLists;
    QSqlDatabase db;

    QSharedPointer<Authors> authorInterface;
    QSharedPointer<database::IDBWrapper> portableDBInterface;

    QHash<QString, QSharedPointer<core::Author>> currentRecommenderSet;
    int currentRecommendationList = -1;


};

}
