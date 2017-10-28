#include "Interfaces/recommendation_lists.h"
#include "Interfaces/authors.h"
#include "Interfaces/db_interface.h"
#include "include/pure_sql.h"
#include <QVector>

namespace interfaces {

int RecommendationLists::GetListIdForName(QString name)
{
    if(!EnsureList(name))
        return -1;
    return nameIndex[name]->id;
}

QString RecommendationLists::GetListNameForId(int id)
{
    if(idIndex.contains(id))
        return idIndex[id]->name;
    return QString();
}

void RecommendationLists::Reindex()
{
    ClearIndex();
    IndexLists();
}

void RecommendationLists::IndexLists()
{
    for(auto list: lists)
    {
        if(!list)
            continue;
        idIndex[list->id] = list;
        nameIndex[list->name] = list;
    }
}

void RecommendationLists::ClearIndex()
{
    idIndex.clear();
    nameIndex.clear();
}

QList<QSharedPointer<core::AuthorRecommendationStats> > RecommendationLists::GetAuthorStatsForList(int id)
{
    QList<QSharedPointer<core::AuthorRecommendationStats> > result;
    if(!idIndex.contains(id))
        return result;
    if(cachedAuthorStats.contains(id))
        result = cachedAuthorStats[id];
    //otherwise, need to load it
    auto stats = database::puresql::GetRecommenderStatsForList(id, "(1/match_ratio)*match_count", "desc", db);
    cachedAuthorStats[id] = stats;
    result = stats;
    return result;
}

QSharedPointer<core::AuthorRecommendationStats> RecommendationLists::GetIndividualAuthorStatsForList(int id, int authorId)
{
    QSharedPointer<core::AuthorRecommendationStats> result;
    auto temp = GetAuthorStatsForList(id);
    if(temp.size() == 0)
        return result;
    auto it = std::find_if(std::begin(temp),std::end(temp),[authorId](QSharedPointer<core::AuthorRecommendationStats> st){
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
    return QStringList(); //! todo
}

bool RecommendationLists::DeleteList(int listId)
{
    return database::puresql::DeleteRecommendationList(listId, db);
}

bool RecommendationLists::ReloadList(int listId)
{
    QSharedPointer<core::RecommendationList> list;
    return true; //! todo
}

void RecommendationLists::AddList(QSharedPointer<core::RecommendationList>)
{

}

QSharedPointer<core::RecommendationList> RecommendationLists::NewList()
{
    return QSharedPointer<core::RecommendationList>(new core::RecommendationList);
}

QSharedPointer<core::AuthorRecommendationStats> RecommendationLists::CreateAuthorRecommendationStatsForList(int authorId,int listId)
{
    auto preExisting = GetIndividualAuthorStatsForList(listId, authorId);
    if(preExisting)
        return preExisting;

    QSharedPointer<core::AuthorRecommendationStats> result (new core::AuthorRecommendationStats);
    if(!EnsureList(listId))
        return result;

    auto list = idIndex[listId];
    auto author = authorInterface->GetById(authorId);
    if(!list || !author)
        return result;

    result->authorId = author->id;
    result->totalFics = author->ficCount;

    result->matchesWithReference = database::puresql::GetMatchesWithListIdInAuthorRecommendations(author->id, listId, db);
    if(result->matchesWithReference == 0)
        result->matchRatio = 999999;
    else
        result->matchRatio = (double)result->totalFics/(double)result->matchesWithReference;
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

bool RecommendationLists::LoadAuthorRecommendationStatsIntoDatabase(int listId, QSharedPointer<core::AuthorRecommendationStats> stats)
{
    return database::puresql::WriteAuthorRecommendationStatsForList(listId, stats, db);
}

bool RecommendationLists::LoadListIntoDatabase(QSharedPointer<core::RecommendationList> list)
{
    AddList(list);
    auto timeStamp = portableDBInterface->GetCurrentDateTime();
    return database::puresql::CreateOrUpdateRecommendationList(list, timeStamp, db);
}

bool RecommendationLists::UpdateFicCountInDatabase(int listId)
{
    return database::puresql::UpdateFicCountForRecommendationList(listId, db);
}

bool RecommendationLists::AddAuthorFavouritesToList(int authorId, int listId, bool reloadLocalData)
{
    auto result = database::puresql::AddAuthorFavouritesToList(authorId, listId, db);
    if(result)
        return false;
    if(reloadLocalData)
        ReloadList(listId);
    return result;
}
void RecommendationLists::Clear()
{
    ClearIndex();
    lists.clear();
}

void RecommendationLists::LoadAvailableRecommendationLists()
{
    lists = database::puresql::GetAvailableRecommendationLists(db);
    Reindex();
}

bool RecommendationLists::EnsureList(int listId)
{
    if(idIndex.contains(listId) && !idIndex[listId])
        return false;
    if(idIndex.contains(listId))
        return true;
    auto list = database::puresql::GetRecommendationList(listId, db);
    if(!list)
        return false;
    idIndex[listId] = list;
    Reindex();
    return true;
}

bool RecommendationLists::EnsureList(QString name)
{
    if(nameIndex.contains(name) && !nameIndex[name])
        return false;
    if(nameIndex.contains(name))
        return true;
    auto list = database::puresql::GetRecommendationList(name, db);
    if(!list)
        return false;
    idIndex[list->id] = list;
    Reindex();
    return true;
}

bool RecommendationLists::LoadAuthorsForRecommendationList(int listId)
{
    currentRecommendationList = listId;
// todo implement
    return true;
}

QList<QSharedPointer<core::Author> > RecommendationLists::GetAuthorsForRecommendationList(int listId)
{
    if(currentRecommendationList != listId)
        LoadAuthorsForRecommendationList(listId);
    return currentRecommenderSet.values();
}

QSharedPointer<core::RecommendationList> RecommendationLists::GetList(int id)
{
    QSharedPointer<core::RecommendationList> result;
    if(!EnsureList(id))
        return result;
    result = idIndex[id];
    return result;
}

QSharedPointer<core::RecommendationList> RecommendationLists::GetList(QString name)
{
    auto id = GetListIdForName(name);
    return GetList(id);
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
