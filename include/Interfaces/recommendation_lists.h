/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#pragma once
#include "Interfaces/base.h"
#include "Interfaces/db_interface.h"
#include "core/section.h"
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
    bool RemoveAuthorRecommendationStatsFromDatabase(int listId, int authorId);
    bool LoadListIntoDatabase(core::RecPtr);

    core::AuhtorStatsPtr CreateAuthorRecommendationStatsForList(int authorId, int listId);
    bool IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId);
    bool DecrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId);
    bool UpdateFicCountInDatabase(int listId);
    bool AddAuthorFavouritesToList(int authorId, int listId, bool reloadLocalData = false);
    bool SetFicsAsListOrigin(QList<int> ficIds, int listId);
    bool IsAuthorInCurrentRecommendationSet(QString author);

    QStringList GetAllRecommendationListNames(bool forced = false);
    QList<core::AuhtorStatsPtr> GetAuthorStatsForList(int id, bool forced = false);
    core::AuhtorStatsPtr GetIndividualAuthorStatsForList(int id, int authorId);
    int GetMatchCountForRecommenderOnList(int authorId, int listId);
    QVector<int> GetAllFicIDs(int listId);
    QStringList GetNamesForListId(int listId);
    QList<core::AuthorPtr> GetAuthorsForRecommendationList(int listId);
    QList<int> GetRecommendersForFicId(int ficId);
    QStringList GetLinkedPagesForList(int listId, QString website);

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
