/*
FFSSE is a replacement search engine for fanfiction.net search results
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
#include "Interfaces/recommendation_lists.h"
#include "Interfaces/authors.h"
#include "Interfaces/db_interface.h"
#include "include/pure_sql.h"
#include <QVector>

namespace interfaces {


void RecommendationLists::Clear()
{
    ClearIndex();
    ClearCache();
    lists.clear();
    currentRecommenderSet.clear();
}

void RecommendationLists::ClearIndex()
{
    idIndex.clear();
    nameIndex.clear();
}

void RecommendationLists::ClearCache()
{
    cachedAuthorStats.clear();
    ficsCacheForLists.clear();
}


void RecommendationLists::Reindex()
{
    ClearIndex();
    for(auto list: lists)
    {
        if(!list)
            continue;
        idIndex[list->id] = list;
        nameIndex[list->name] = list;
    }
}

void RecommendationLists::AddToIndex(core::RecPtr list)
{
    idIndex[list->id] = list;
    nameIndex[list->name] = list;
}

bool RecommendationLists::EnsureList(int listId)
{
    // means there was an attept to access the list and it was not found
    // hence an empty record was inserted
    // needed so that I don't try to access non existing lists repeatedly
    if(idIndex.contains(listId) && !idIndex[listId])
        return false;

    if(idIndex.contains(listId))
        return true;

    auto list = database::puresql::GetRecommendationList(listId, db);

    AddToIndex(list);

    if(!list)
        return false;

    return true;
}

bool RecommendationLists::EnsureList(QString name)
{
    // means there was an attept to access the list and it was not found
    // hence an empty record was inserted
    // needed so that I don't try to access non existing lists repeatedly

    if(nameIndex.contains(name) && !nameIndex[name])
        return false;
    if(nameIndex.contains(name))
        return true;
    auto list = database::puresql::GetRecommendationList(name, db);
    if(!list)
        return false;
    AddToIndex(list);

    if(!list)
        return false;

    return true;
}

core::RecPtr RecommendationLists::GetList(int id)
{
    core::RecPtr result;
    if(EnsureList(id))
        result = idIndex[id];
    return result;
}

core::RecPtr RecommendationLists::GetList(QString name)
{
    core::RecPtr result;
    if(EnsureList(name))
        result = nameIndex[name];
    return result;
}

int RecommendationLists::GetListIdForName(QString name)
{
    int result = -1;
    if(EnsureList(name))
        result = nameIndex[name]->id;

    return result;
}

QString RecommendationLists::GetListNameForId(int id)
{

    QString result;
    if(EnsureList(id))
        result = idIndex[id]->name;

    return result;
}

QList<core::AuhtorStatsPtr > RecommendationLists::GetAuthorStatsForList(int id)
{
    QList<core::AuhtorStatsPtr > result;
    if(!EnsureList(id))
        return result;
    if(cachedAuthorStats.contains(id))
        result = cachedAuthorStats[id];
    else
    {
        //otherwise, need to load it
        auto stats = database::puresql::GetRecommenderStatsForList(id, "(1/match_ratio)*match_count", "desc", db);
        cachedAuthorStats[id] = stats;
        result = stats;
    }
    return result;
}

core::AuhtorStatsPtr RecommendationLists::GetIndividualAuthorStatsForList(int id, int authorId)
{
    core::AuhtorStatsPtr result;
    auto temp = GetAuthorStatsForList(id);
    if(temp.size() == 0)
        return result;
    auto it = std::find_if(std::begin(temp),std::end(temp),[authorId](core::AuhtorStatsPtr st){
        if(st && st->authorId == authorId)
            return true;
        return false;
    });
    if(it != std::end(temp))
        result = *it;
    return result;
}

int RecommendationLists::GetMatchCountForRecommenderOnList(int authorId, int listId)
{
    auto data = GetIndividualAuthorStatsForList(listId, authorId);
    if(!data)
        return 0;
    return data->matchesWithReference;
}

QVector<int> RecommendationLists::GetAllFicIDs(int listId)
{
    QVector<int> result;
    if(!EnsureList(listId))
        return result;

    if(!ficsCacheForLists.contains(listId))
    {
        result = database::puresql::GetAllFicIDsFromRecommendationList(listId, db);
        ficsCacheForLists[listId] = result;
    }
    else
        result = ficsCacheForLists[listId];

    return result;

}

QStringList RecommendationLists::GetNamesForListId(int listId)
{
    QStringList result;
    if(!EnsureList(listId))
        return result;

    if(!authorsCacheForLists.contains(listId))
    {
        result = database::puresql::GetAllAuthorNamesForRecommendationList(listId, db);
        authorsCacheForLists[listId] = result;
    }
    else
        result = authorsCacheForLists[listId];

    return result;

}

bool RecommendationLists::DeleteList(int listId)
{
    if(listId == -1 || !EnsureList(listId))
        return true;

    bool result = database::puresql::DeleteRecommendationList(listId, db);
    DeleteLocalList(listId);
    return result;

}

void RecommendationLists::DeleteLocalList(int listId)
{
    if(!idIndex.contains(listId))
        return;

    auto list = idIndex[listId];
    idIndex.remove(list->id);
    nameIndex.remove(list->name);
    ficsCacheForLists.remove(list->id);
    authorsCacheForLists.remove(list->id);
    cachedAuthorStats.remove(list->id);
    if(currentRecommendationList == list->id)
        currentRecommendationList = -1;

    auto it = std::remove_if(std::begin(lists), std::end(lists), [listId](auto item){
                                 return item && item->id == listId;});
    lists.erase(it, std::end(lists));
}



bool RecommendationLists::ReloadList(int listId)
{
    DeleteLocalList(listId);
    if(EnsureList(listId))
        return true;
    return false;
}

core::AuhtorStatsPtr RecommendationLists::CreateAuthorRecommendationStatsForList(int authorId,int listId)
{
    core::AuhtorStatsPtr  result;
    if(!EnsureList(listId) || !authorInterface->EnsureAuthorLoaded(authorId))
        return result;


    auto preExisting = GetIndividualAuthorStatsForList(listId, authorId);
    if(preExisting)
        return preExisting;

    result = core::AuthorRecommendationStats::NewAuthorStats();

    auto list = idIndex[listId];
    auto author = authorInterface->GetById(authorId);
    if(!list || !author)
        return result;

    result->authorId = author->id;
    result->totalRecommendations= author->recCount;

    result->matchesWithReference = database::puresql::GetMatchesWithListIdInAuthorRecommendations(author->id, listId, db);
    if(result->matchesWithReference == 0)
        result->matchRatio = 999999;
    else
        result->matchRatio = static_cast<double>(result->totalRecommendations)/static_cast<double>(result->matchesWithReference);
    result->isValid = true;
    cachedAuthorStats[listId].push_back(result);
    return result;
}

bool RecommendationLists::LoadAuthorRecommendationsIntoList(int authorId, int listId)
{
    return database::puresql::CopyAllAuthorRecommendationsToList(authorId, listId, db);
}

bool RecommendationLists::IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId)
{
    return database::puresql::IncrementAllValuesInListMatchingAuthorFavourites(authorId,listId, db);
}

bool RecommendationLists::LoadAuthorRecommendationStatsIntoDatabase(int listId, core::AuhtorStatsPtr stats)
{
    return database::puresql::WriteAuthorRecommendationStatsForList(listId, stats, db);
}

bool RecommendationLists::LoadListIntoDatabase(core::RecPtr list)
{
    AddToIndex(list);
    auto timeStamp = portableDBInterface->GetCurrentDateTime();
    return database::puresql::CreateOrUpdateRecommendationList(list, timeStamp, db);
}

bool RecommendationLists::UpdateFicCountInDatabase(int listId)
{
    return database::puresql::UpdateFicCountForRecommendationList(listId, db);
}

// currently unused
bool RecommendationLists::AddAuthorFavouritesToList(int authorId, int listId, bool reloadLocalData)
{
    auto result = database::puresql::AddAuthorFavouritesToList(authorId, listId, db);
    if(result)
        return false;
    if(reloadLocalData)
        ReloadList(listId);
    return result;
}

bool RecommendationLists::SetFicsAsListOrigin(QList<int> ficIds, int listId)
{
    if(listId == -1)
        return false;
    return database::puresql::SetFicsAsListOrigin(ficIds,listId, db);
}


void RecommendationLists::LoadAvailableRecommendationLists()
{
    lists = database::puresql::GetAvailableRecommendationLists(db);
    Reindex();
}

bool RecommendationLists::LoadAuthorsForRecommendationList(int listId)
{
    currentRecommendationList = listId;
    currentRecommenderSet.clear();
    auto authors = database::puresql::GetAuthorsForRecommendationList(listId, db);

    for(auto author: authors)
    {
        if(!author)
            continue;
        currentRecommenderSet.insert(author->id, author);
        authorInterface->AddPreloadedAuthor(author);
    }
    return true;
}

QList<QSharedPointer<core::Author> > RecommendationLists::GetAuthorsForRecommendationList(int listId)
{
    if(currentRecommendationList != listId || currentRecommenderSet.isEmpty())
        LoadAuthorsForRecommendationList(listId);
    return currentRecommenderSet.values();
}

QList<int> RecommendationLists::GetRecommendersForFicId(int ficId)
{
    QList<int> result;
    result = database::puresql::GetRecommendersForFicIdAndListId(ficId, db);
    return result;
}

QStringList RecommendationLists::GetLinkedPagesForList(int listId)
{
    QStringList result;
    if(!EnsureList(listId))
        return result;
    result = database::puresql::GetLinkedPagesForList(listId, db);
    return result;
}

void RecommendationLists::SetCurrentRecommendationList(int value)
{
    currentRecommendationList = value;
}

int RecommendationLists::GetCurrentRecommendationList() const
{
    return currentRecommendationList;
}

QStringList RecommendationLists::GetAllRecommendationListNames()
{
    if(lists.empty())
        LoadAvailableRecommendationLists();
    return nameIndex.keys();

}
}
