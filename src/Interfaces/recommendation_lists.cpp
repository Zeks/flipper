#include "Interfaces/recommendation_lists.h"
#include "Interfaces/authors.h"
#include "Interfaces/db_interface.h"
#include "include/pure_sql.h"
#include <QVector>

namespace database {

int DBRecommendationListsBase::GetListIdForName(QString name)
{
    if(nameIndex.contains(name))
        return nameIndex[name]->id;
    return -1;
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
    auto stats = puresql::GetRecommenderStatsForList(id, "(1/match_ratio)*match_count", "desc", db);
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
        result = puresql::GetAllFicIDsFromRecommendationList(listId, db);
        ficsCacheForLists[listId] = result;
    }
    else
        result = ficsCacheForLists[listId];

    return result;

}

bool DBRecommendationListsBase::DeleteList(int listId)
{
    return puresql::DeleteRecommendationList(listId, db);
}

bool DBRecommendationListsBase::ReloadList(int listId)
{
    QSharedPointer<core::RecommendationList> list;

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

    result->matchesWithReference = puresql::GetMatchesWithListIdInAuthorRecommendations(author->id, listId, db);
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
    return puresql::CopyAllAuthorRecommendationsToList(authorId, listId, db);
}

bool DBRecommendationListsBase::LoadAuthorRecommendationStatsIntoDatabase(int listId, QSharedPointer<core::AuthorRecommendationStats> stats)
{
    return puresql::WriteAuthorRecommendationStatsForList(listId, stats, db);
}

bool DBRecommendationListsBase::LoadListIntoDatabase(QSharedPointer<core::RecommendationList> list)
{
    AddList(list);
    auto timeStamp = portableDBInterface->GetCurrentDateTime();
    return puresql::CreateOrUpdateRecommendationList(list, timeStamp, db);
}

bool DBRecommendationListsBase::UpdateFicCountInDatabase(int listId)
{
    return puresql::UpdateFicCountForRecommendationList(listId, db);
}

bool DBRecommendationListsBase::AddAuthorFavouritesToList(int authorId, int listId, bool reloadLocalData)
{
    auto result = puresql::AddAuthorFavouritesToList(authorId, listId, db);
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
    idIndex[listId] = list;
    if(!list)
        return false;
    return true;
}
}
