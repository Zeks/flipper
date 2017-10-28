#include "Interfaces/recommendation_lists.h"
#include "Interfaces/authors.h"
#include "Interfaces/db_interface.h"
#include "include/pure_sql.h"
#include <QVector>

namespace interfaces {

int DBRecommendationListsBase::GetListIdForName(QString name)
{
    if(!EnsureList(name))
        return -1;
    return nameIndex[name]->id;
}

QString DBRecommendationListsBase::GetListNameForId(int id)
{
    if(idIndex.contains(id))
        return idIndex[id]->name;
    return QString();
}

void DBRecommendationListsBase::Reindex()
{
    ClearIndex();
    IndexLists();
}

void DBRecommendationListsBase::IndexLists()
{
    for(auto list: lists)
    {
        if(!list)
            continue;
        idIndex[list->id] = list;
        nameIndex[list->name] = list;
    }
}

void DBRecommendationListsBase::ClearIndex()
{
    idIndex.clear();
    nameIndex.clear();
}

QList<QSharedPointer<core::AuthorRecommendationStats> > DBRecommendationListsBase::GetAuthorStatsForList(int id)
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

QSharedPointer<core::AuthorRecommendationStats> DBRecommendationListsBase::GetIndividualAuthorStatsForList(int id, int authorId)
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

int DBRecommendationListsBase::GetMatchCountForRecommenderOnList(int authorId, int listId)
{
    auto data = GetIndividualAuthorStatsForList(listId, authorId);
    if(!data)
        return 0;
    return data->matchesWithReference;
}

QVector<int> DBRecommendationListsBase::GetAllFicIDs(int listId)
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

QStringList DBRecommendationListsBase::GetNamesForListId(int listId)
{
    return QStringList(); //! todo
}

bool DBRecommendationListsBase::DeleteList(int listId)
{
    return database::puresql::DeleteRecommendationList(listId, db);
}

bool DBRecommendationListsBase::ReloadList(int listId)
{
    QSharedPointer<core::RecommendationList> list;
    return true; //! todo
}

void DBRecommendationListsBase::AddList(QSharedPointer<core::RecommendationList>)
{

}

QSharedPointer<core::RecommendationList> DBRecommendationListsBase::NewList()
{
    return QSharedPointer<core::RecommendationList>(new core::RecommendationList);
}

QSharedPointer<core::AuthorRecommendationStats> DBRecommendationListsBase::CreateAuthorRecommendationStatsForList(int authorId,int listId)
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

bool DBRecommendationListsBase::LoadAuthorRecommendationsIntoList(int authorId, int listId)
{
    return database::puresql::CopyAllAuthorRecommendationsToList(authorId, listId, db);
}

bool DBRecommendationListsBase::IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId)
{
    return database::puresql::IncrementAllValuesInListMatchingAuthorFavourites(authorId,listId, db);
}

bool DBRecommendationListsBase::LoadAuthorRecommendationStatsIntoDatabase(int listId, QSharedPointer<core::AuthorRecommendationStats> stats)
{
    return database::puresql::WriteAuthorRecommendationStatsForList(listId, stats, db);
}

bool DBRecommendationListsBase::LoadListIntoDatabase(QSharedPointer<core::RecommendationList> list)
{
    AddList(list);
    auto timeStamp = portableDBInterface->GetCurrentDateTime();
    return database::puresql::CreateOrUpdateRecommendationList(list, timeStamp, db);
}

bool DBRecommendationListsBase::UpdateFicCountInDatabase(int listId)
{
    return database::puresql::UpdateFicCountForRecommendationList(listId, db);
}

bool DBRecommendationListsBase::AddAuthorFavouritesToList(int authorId, int listId, bool reloadLocalData)
{
    auto result = database::puresql::AddAuthorFavouritesToList(authorId, listId, db);
    if(result)
        return false;
    if(reloadLocalData)
        ReloadList(listId);
    return result;
}
void DBRecommendationListsBase::Clear()
{
    ClearIndex();
    lists.clear();
}

void DBRecommendationListsBase::LoadAvailableRecommendationLists()
{
    lists = database::puresql::GetAvailableRecommendationLists(db);
    Reindex();
}

bool DBRecommendationListsBase::EnsureList(int listId)
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

bool DBRecommendationListsBase::EnsureList(QString name)
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

bool DBRecommendationListsBase::LoadAuthorsForRecommendationList(int listId)
{
    currentRecommendationList = listId;
// todo implement
    return true;
}

QList<QSharedPointer<core::Author> > DBRecommendationListsBase::GetAuthorsForRecommendationList(int listId)
{
    if(currentRecommendationList != listId)
        LoadAuthorsForRecommendationList(listId);
    return currentRecommenderSet.values();
}

QSharedPointer<core::RecommendationList> DBRecommendationListsBase::GetList(int id)
{
    QSharedPointer<core::RecommendationList> result;
    if(!EnsureList(id))
        return result;
    result = idIndex[id];
    return result;
}

QSharedPointer<core::RecommendationList> DBRecommendationListsBase::GetList(QString name)
{
    auto id = GetListIdForName(name);
    return GetList(id);
}

void DBRecommendationListsBase::SetCurrentRecommendationList(int value)
{
    currentRecommendationList = value;
}

int DBRecommendationListsBase::GetCurrentRecommendationList() const
{
    return currentRecommendationList;
}

QStringList DBRecommendationListsBase::GetAllRecommendationListNames()
{
    if(lists.empty())
        LoadAvailableRecommendationLists();
    return nameIndex.keys();

}
}
