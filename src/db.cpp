#include "include/db.h"
#include "include/pure_sql.h"

namespace database{
void IDBAuthors::Clear()
{
//    QList<QSharedPointer<core::Author>> authors;

//    QHash<QString, QSharedPointer<core::Author>> authorsByname;
//    QHash<QString, QHash<QString, QSharedPointer<core::Author>>> authorsByWebsite;
//    QHash<int, QSharedPointer<core::Author>> authorsByWebId;
//    QHash<int, QSharedPointer<core::Author>> authorsById;


//    QList<QSharedPointer<core::Author>> queue;
//    QHash<int, QSharedPointer<core::Fic>> queuedFicRecommendations;
    authors.clear();
    ClearIndex();
}

void IDBAuthors::Reindex()
{
    ClearIndex();
    IndexAuthors();
}

void IDBAuthors::IndexAuthors()
{
    for(auto author : authors)
    {
        authorsById[author->id] = author;
        authorsByName[author->name] = author;
        authorsNamesByWebsite[author->website][author->name] = author;
        authorsByUrl[author->url("ffn")] = author;
    }
}

void IDBAuthors::ClearIndex()
{
    authorsByName.clear();
    authorsNamesByWebsite.clear();
    authorsByUrl.clear();
    authorsById.clear();
}

QSharedPointer<QSharedPointer<core::Author> > IDBAuthors::GetSingleByName(QString name, QString website)
{
    QSharedPointer<QSharedPointer<core::Author>> result;
    if(authorsNamesByWebsite.contains(website) && authorsNamesByWebsite[website].contains(name))
        result = {authorsNamesByWebsite[website][name]};
    return result;

}

QList<QSharedPointer<QSharedPointer<core::Author>>> IDBAuthors::GetAllByName(QString name)
{
    QList<QSharedPointer<QSharedPointer<core::Author>>> result;

    for(auto websiteHash : authorsNamesByWebsite)
    {
        if(websiteHash.contains(name))
            result.push_back(websiteHash[name]);
    }

    return result;
}

QSharedPointer<core::Author> IDBAuthors::GetByUrl(QString url)
{
    QSharedPointer<QSharedPointer<core::Author>> result;
    if(!authorsByUrl.contains(url))
        return result;
    return authorsByUrl[url];
}

QSharedPointer<core::Author > IDBAuthors::GetById(int id)
{
    QSharedPointer<QSharedPointer<core::Author>> result;
    if(!authorsById.contains(id))
        return result;
    return authorsById[id];
}

QList<QSharedPointer<core::Author> > IDBAuthors::GetAllAuthors(QString website)
{
    if(website.isEmpty())
        return authors;
    if(!authorsNamesByWebsite.contains(website))
        return authors;
    return authorsNamesByWebsite[website];
}

int IDBAuthors::GetFicCount(int authorId)
{
    auto author = GetById(authorId);
    if(!author)
        return 0;
    return author->ficCount;
}

int IDBAuthors::GetCountOfRecsForTag(int authorId, QString tag)
{
    auto result = puresql::GetCountOfTagInAuthorRecommendations(authorId, tag, db);
    return result;
}

QSharedPointer<core::AuthorRecommendationStats> IDBAuthors::GetStatsForTag(authorId, core::RecommendationList list)
{
    QSharedPointer<core::AuthorRecommendationStats>result (new core::AuthorRecommendationStats);
    auto author = GetById(authorId);
    if(!author)
        return result;

    result->listName = list.name;
    result->usedTag = list.tagToUse;;
    result->authorName = author->name;
    result->authorId= author->id;
    result->totalFics= author->ficCount;

    result->matchesWithReference= puresql::GetCountOfTagInAuthorRecommendations(author->id, list.tagToUse, db);
    if(result->matchesWithReference == 0)
        result->matchRatio = 999999;
    else
        result->matchRatio = (double)result->totalFics/(double)result->matchesWithReference;
    result->isValid = true;
}


int IDBRecommendationLists::GetListIdForName(QString name)
{
    if(nameIndex.contains(name))
        return nameIndex[name]->id;
    return -1;
}

int IDBRecommendationLists::GetListNameForId(int id)
{
    if(idIndex.contains(id))
        return idIndex[id]->name;
    return QString();
}

void IDBRecommendationLists::Reindex()
{
    ClearIndex();
    IndexLists();
}

void IDBRecommendationLists::IndexLists()
{
    for(auto list: lists)
    {
        if(!list)
            continue;
        idIndex[list->id] = list;
        nameIndex[list->name] = list;
    }
}

void IDBRecommendationLists::ClearIndex()
{
    idIndex.clear();
    nameIndex.clear();
}

QList<QSharedPointer<core::AuthorRecommendationStats> > IDBRecommendationLists::GetAuthorStatsForList(int id)
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

QSharedPointer<core::AuthorRecommendationStats> IDBRecommendationLists::GetIndividualAuthorStatsForList(int id, int authorId)
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

int IDBRecommendationLists::GetMatchCountForRecommenderOnList(int authorId, int listId)
{
    auto data = GetIndividualAuthorStatsForList(listId, authorId);
    if(!data)
        return 0;
    return data->matchesWithReference;
}

QVector<int> IDBRecommendationLists::GetAllFicIDs(int listId)
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

bool IDBRecommendationLists::DeleteList(int listId)
{
    return puresql::DeleteRecommendationList(listId, db);
}

bool IDBRecommendationLists::ReloadList(int listId)
{

}

void IDBRecommendationLists::AddList(QSharedPointer<core::RecommendationList>)
{

}

QSharedPointer<core::RecommendationList> IDBRecommendationLists::NewList()
{
    return {new core::RecommendationList};
}

QSharedPointer<core::AuthorRecommendationStats> IDBRecommendationLists::CreateAuthorRecommendationStatsForList(int authorId,int listId)
{
    auto preExisting = GetIndividualAuthorStatsForList(listId, authorId);
    if(preExisting)
        return preExisting;

    QSharedPointer<core::AuthorRecommendationStats> result (new core::AuthorRecommendationStats);
    auto list = GetListById(listId);
    auto author = authorInterface->GetById(authorId);
    if(!list || !author)
        return result;

    result->authorId = author->id;
    result->totalFics = author->ficCount;

    result.matchesWithReference = puresql::GetMatchesWithListIdInAuthorRecommendations(author->id, listId, db);
    if(result->matchesWithReference == 0)
        result->matchRatio = 999999;
    else
        result->matchRatio = (double)result->totalFics/(double)result->matchesWithReference;
    result.isValid = true;
    cachedAuthorStats[listId].push_back(result);
    return result;
}

bool IDBRecommendationLists::LoadAuthorRecommendationsIntoList(int authorId, int listId)
{
    return puresql::CopyAllAuthorRecommendationsToList(authorId, listId, db);
}

bool IDBRecommendationLists::LoadAuthorRecommendationStatsIntoDatabase(int listId, QSharedPointer<core::AuthorRecommendationStats> stats)
{
    return puresql::WriteAuthorRecommendationStatsForList(listId, stats, db);
}

bool IDBRecommendationLists::LoadListIntoDatabase(QSharedPointer<core::RecommendationList> list)
{
    AddList(list);
    auto timeStamp = portableDBInterface->GetCurrentDateTime();
    return puresql::CreateOrUpdateRecommendationList(list, timeStamp, db);
}

bool IDBRecommendationLists::UpdateFicCountInDatabase(int listId)
{
    return puresql::UpdateFicCountForRecommendationList(listId, db);
}

bool IDBRecommendationLists::AddAuthorFavouritesToList(int authorId, int listId, bool reloadLocalData)
{
    auto result = puresql::AddAuthorFavouritesToList(authorId, listId, db);
    if(result)
        return false;
    if(reloadLocalData)
        ReloadList(listId);
    return result;
}

bool ITags::DeleteTag(QString tag)
{
    puresql::DeleteTagfromDatabase(tag, db);
}

bool IDBFanfics::ReprocessFics(QString where, QString website, std::function<void (int)> f)
{
    auto list = puresql::GetWebIdList(where, website, db);
    for(auto id : list)
    {
        f(id);
    }
}

bool IDBFanfics::DeactivateFic(int ficId)
{
    return puresql::DeactivateStory(ficId, "ffn", db);
}

}
