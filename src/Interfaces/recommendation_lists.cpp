/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

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
#include "include/sqlitefunctions.h"
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
    for(const auto& list: std::as_const(lists))
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

    auto list = sql::GetRecommendationList(listId, db).data;

    if(!list)
        return false;

    AddToIndex(list);
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
    auto list = sql::GetRecommendationList(name, db).data;

    if(!list)
        return false;
    AddToIndex(list);

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

QList<core::AuhtorStatsPtr > RecommendationLists::GetAuthorStatsForList(int id, bool forced)
{
    QList<core::AuhtorStatsPtr > result;
    if(!EnsureList(id))
        return result;
    if(forced || !cachedAuthorStats.contains(id))
    {
        //otherwise, need to load it
        auto stats = sql::GetRecommenderStatsForList(id, "(1/match_ratio)*match_count", "desc", db).data;
        cachedAuthorStats[id] = stats;
        result = stats;
    }
    else
        result = cachedAuthorStats[id];
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

QVector<int> RecommendationLists::GetAllFicIDs(int listId ,core::StoryFilter::ESourceListLimiter limiter)
{
    QVector<int> result;
    if(!EnsureList(listId))
        return result;

    if(!ficsCacheForLists.contains(listId))
    {
        result = sql::GetAllFicIDsFromRecommendationList(listId, limiter, db).data;
        ficsCacheForLists[listId] = result;
    }
    else
        result = ficsCacheForLists[listId];

    return result;

}

QVector<int> RecommendationLists::GetAllSourceFicIDs(int listId)
{
    auto result = sql::GetAllSourceFicIDsFromRecommendationList(listId,db).data;
    return result;
}

core::RecommendationListFicSearchToken RecommendationLists::GetAllFicsHash(core::ReclistFilter filter)
{
    core::RecommendationListFicSearchToken result;
    if(!EnsureList(filter.mainListId))
        return result;

    result = sql::GetRelevanceScoresInFilteredReclist(filter ,db).data;

    return result;
}

QStringList RecommendationLists::GetNamesForListId(int listId)
{
    QStringList result;
    if(!EnsureList(listId))
        return result;

    if(!authorsCacheForLists.contains(listId))
    {
        result = sql::GetAllAuthorNamesForRecommendationList(listId, db).data;
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

    bool result = sql::DeleteRecommendationList(listId, db).success;
    DeleteLocalList(listId);
    return result;

}

bool RecommendationLists::DeleteListData(int listId)
{
    if(listId == -1 || !EnsureList(listId))
        return true;

    bool result = sql::DeleteRecommendationListData(listId, db).success;
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
    auto opResult = sql::GetMatchesWithListIdInAuthorRecommendations(author->id, listId, db);
    if(!opResult.success)
        return result;

    result->matchesWithReference = opResult.data;
    if(result->matchesWithReference == 0)
        result->matchRatio = std::numeric_limits<double>::max();
    else
        result->matchRatio = static_cast<double>(result->totalRecommendations)/static_cast<double>(result->matchesWithReference);
    result->isValid = true;
    cachedAuthorStats[listId].push_back(result);
    return result;
}

bool RecommendationLists::LoadAuthorRecommendationsIntoList(int authorId, int listId)
{
    return sql::CopyAllAuthorRecommendationsToList(authorId, listId, db).success;
}

bool RecommendationLists::IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId)
{
    return sql::IncrementAllValuesInListMatchingAuthorFavourites(authorId,listId, db).success;
}

bool RecommendationLists::DecrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId)
{
    return sql::DecrementAllValuesInListMatchingAuthorFavourites(authorId,listId, db).success;
}

bool RecommendationLists::LoadAuthorRecommendationStatsIntoDatabase(int listId, core::AuhtorStatsPtr stats)
{
    return sql::WriteAuthorRecommendationStatsForList(listId, stats, db).success;
}

bool RecommendationLists::RemoveAuthorRecommendationStatsFromDatabase(int listId, int authorId)
{
    return sql::RemoveAuthorRecommendationStatsFromDatabase(listId, authorId, db).success;
}

bool RecommendationLists::LoadListIntoDatabase(core::RecPtr list)
{
    auto timeStamp = database::sqlite::GetCurrentDateTime(db);
    // todo unnecessary access to datetime extrenally
    auto result = sql::CreateOrUpdateRecommendationList(list, timeStamp, db);
    if(!result.success)
        return false;
    AddToIndex(list);
    return result.success;
}

bool RecommendationLists::LoadListAuxDataIntoDatabase(core::RecPtr list)
{
     return sql::WriteAuxParamsForReclist(list, db).success;
}

bool RecommendationLists::LoadListFromServerIntoDatabase(int listId,
                                                         const QVector<int> &fics,
                                                         const QVector<int> &matches,
                                                         const QSet<int> &origins)
{
    bool result = true;
    result = result && sql::FillFicDataForList(listId, fics, matches, origins, db).success;
    return result;
}
bool RecommendationLists::LoadListFromServerIntoDatabase(QSharedPointer<core::RecommendationList> list)
{
    bool result = true;
    result = result && sql::FillFicDataForList(list, db).success;
    return result;
}

bool RecommendationLists::UpdateFicCountInDatabase(int listId)
{
    return sql::UpdateFicCountForRecommendationList(listId, db).success;
}

// currently unused
bool RecommendationLists::AddAuthorFavouritesToList(int authorId, int listId, bool reloadLocalData)
{
    auto result = sql::AddAuthorFavouritesToList(authorId, listId, db).success;
    if(result)
        return false;
    if(reloadLocalData)
        ReloadList(listId);
    return result;
}

bool RecommendationLists::SetFicsAsListOrigin(QVector<int> ficIds, int listId)
{
    if(listId == -1)
        return false;
    return sql::SetFicsAsListOrigin(ficIds,listId, db).success;
}

bool RecommendationLists::CreateRecommendationList(QString name, QHash<int, int> fics)
{
    QSharedPointer<core::RecommendationList> dummyParams(new core::RecommendationList);
    dummyParams->name = name;
    dummyParams->ficCount = fics.size();
    dummyParams->created = QDateTime::currentDateTime();
    auto listId = GetListIdForName(name);
    dummyParams->id = listId;
    dummyParams->Log();
    DeleteListData(listId);
    LoadListIntoDatabase(dummyParams);
    listId = GetListIdForName(name);
    qDebug() << "list Id: " << listId;
    if(listId < 0)
        return false;
    qDebug() << "filling fics";
    sql::FillRecommendationListWithData(listId, fics, db);
    return true;
}


void RecommendationLists::LoadAvailableRecommendationLists()
{
    lists = sql::GetAvailableRecommendationLists(db).data;
    Reindex();
}

bool RecommendationLists::LoadAuthorsForRecommendationList(int listId)
{
    currentRecommendationList = listId;
    currentRecommenderSet.clear();
    auto opResult = sql::GetAuthorsForRecommendationList(listId, db);
    if(!opResult.success)
        return false;
    auto authors = opResult.data;

    for(const auto& author: std::as_const(authors))
    {
        if(!author)
            continue;
        currentRecommenderSet.insert(author->id, author);
        authorInterface->AddPreloadedAuthor(author);
    }
    return true;
}

QList<core::AuthorPtr> RecommendationLists::GetAuthorsForRecommendationList(int listId)
{
    if(currentRecommendationList != listId || currentRecommenderSet.isEmpty())
        LoadAuthorsForRecommendationList(listId);
    return currentRecommenderSet.values();
}

QString RecommendationLists::GetAuthorsForRecommendationListClient(int listId)
{
    return sql::GetAuthorsForRecommendationListClient(listId, db).data;
}

QList<int> RecommendationLists::GetRecommendersForFicId(int ficId)
{
    QList<int> result;
    result = sql::GetRecommendersForFicIdAndListId(ficId, db).data;
    return result;
}

QStringList RecommendationLists::GetLinkedPagesForList(int listId, QString website)
{
    QStringList result;
    if(!EnsureList(listId))
        return result;
    result = sql::GetLinkedPagesForList(listId, website,  db).data;
    return result;
}

QHash<int, int> RecommendationLists::GetMatchesForUID(QString uid)
{
    return  sql::GetMatchesForUID(uid,  db).data;
}

bool RecommendationLists::SetUserProfile(int id)
{
    return  sql::SetUserProfile(id,  db).success;
}

int RecommendationLists::GetUserProfile()
{
    return sql::GetUserProfile(db).data;
}

void RecommendationLists::SetCurrentRecommendationList(int value)
{
    currentRecommendationList = value;
}

int RecommendationLists::GetCurrentRecommendationList() const
{
    return currentRecommendationList;
}

QStringList RecommendationLists::GetAllRecommendationListNames(bool forced)
{
    if(forced || lists.empty())
        LoadAvailableRecommendationLists();
    auto list = nameIndex.keys();
    std::sort(list.begin(), list.end());
    return list;
}


void RecommendationLists::FetchRecommendationsBreakdown(QVector<core::Fanfic> *fics, int listId)
{
    sql::FetchRecommendationsBreakdown(fics, listId, db);
}

void RecommendationLists::FetchRecommendationScoreForFics(QVector<core::Fanfic> *fics, core::ReclistFilter filter)
{
    QHash<int, int> scores;
    for(const auto& fic: std::as_const(*fics))
        scores[fic.identity.id] = 0;

    sql::FetchRecommendationScoreForFics(scores, filter, db);
    for(auto& fic: *fics){
        fic.recommendationsData.recommendationsMainList = scores[fic.identity.id];
    }
}

void RecommendationLists::LoadPlaceAndRecommendationsData(QVector<core::Fanfic> *fics, core::ReclistFilter filter)
{
    sql::LoadPlaceAndRecommendationsData(fics, filter, db);
}

QSharedPointer<core::RecommendationList> RecommendationLists::FetchParamsForRecList(QString name)
{
    auto id = GetListIdForName(name);
    if(id == -1)
        return QSharedPointer<core::RecommendationList>();

    return sql::FetchParamsForRecList(id, db).data;
}

bool RecommendationLists::WriteFicRecommenderRelationsForRecList(int listId, QHash<uint32_t, QVector<uint32_t> > data)
{
    return sql::WriteFicRecommenderRelationsForRecList(listId,data, db).success;
}

bool RecommendationLists::WriteAuthorStatsForRecList(int listId, QVector<core::AuthorResult> data)
{
    return sql::WriteAuthorStatsForRecList(listId,data, db).success;
}


}

