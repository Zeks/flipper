#include "include/db.h"
#include "include/pure_sql.h"
void database::IDBAuthors::Clear()
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

void database::IDBAuthors::Reindex()
{
    ClearIndex();
    IndexAuthors();
}

void database::IDBAuthors::IndexAuthors()
{
    for(auto author : authors)
    {
        authorsById[author->id] = author;
        authorsByName[author->name] = author;
        authorsNamesByWebsite[author->website][author->name] = author;
        authorsByUrl[author->url("ffn")] = author;
    }
}

void database::IDBAuthors::ClearIndex()
{
    authorsByName.clear();
    authorsNamesByWebsite.clear();
    authorsByUrl.clear();
    authorsById.clear();
}

QSharedPointer<QSharedPointer<core::Author> > database::IDBAuthors::GetSingleByName(QString name, QString website)
{
    QSharedPointer<QSharedPointer<core::Author>> result;
    if(authorsNamesByWebsite.contains(website) && authorsNamesByWebsite[website].contains(name))
        result = {authorsNamesByWebsite[website][name]};
    return result;

}

QList<QSharedPointer<QSharedPointer<core::Author>>> database::IDBAuthors::GetAllByName(QString name)
{
    QList<QSharedPointer<QSharedPointer<core::Author>>> result;

    for(auto websiteHash : authorsNamesByWebsite)
    {
        if(websiteHash.contains(name))
            result.push_back(websiteHash[name]);
    }

    return result;
}

QSharedPointer<core::Author> database::IDBAuthors::GetByUrl(QString url)
{
    QSharedPointer<QSharedPointer<core::Author>> result;
    if(!authorsByUrl.contains(url))
        return result;
    return authorsByUrl[url];
}

QSharedPointer<core::Author > database::IDBAuthors::GetById(int id)
{
    QSharedPointer<QSharedPointer<core::Author>> result;
    if(!authorsById.contains(id))
        return result;
    return authorsById[id];
}

QList<QSharedPointer<core::Author> > database::IDBAuthors::GetAllAuthors(QString website)
{
    if(website.isEmpty())
        return authors;
    if(!authorsNamesByWebsite.contains(website))
        return authors;
    return authorsNamesByWebsite[website];
}

int database::IDBAuthors::GetFicCount(int authorId)
{
    auto author = GetById(authorId);
    if(!author)
        return 0;
    return author->ficCount;
}

int database::IDBAuthors::GetCountOfRecsForTag(int authorId, QString tag)
{
    auto result = database::puresql::GetCountOfTagInAuthorRecommendations(authorId, tag, db);
    return result;
}

QSharedPointer<core::AuthorRecommendationStats> database::IDBAuthors::GetStatsForTag(authorId, core::RecommendationList list)
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

    result->matchesWithReference= database::puresql::GetCountOfTagInAuthorRecommendations(author->id, list.tagToUse, db);
    if(result->matchesWithReference == 0)
        result->matchRatio = 999999;
    else
        result->matchRatio = (double)result->totalFics/(double)result->matchesWithReference;
    result->isValid = true;
}


int database::IDBRecommendationLists::GetListIdForName(QString name)
{
    if(nameIndex.contains(name))
        return nameIndex[name]->id;
    return -1;
}

int database::IDBRecommendationLists::GetListNameForId(int id)
{
    if(idIndex.contains(id))
        return idIndex[id]->name;
    return QString();
}

void database::IDBRecommendationLists::Reindex()
{
    ClearIndex();
    IndexLists();
}

void database::IDBRecommendationLists::IndexLists()
{
    for(auto list: lists)
    {
        if(!list)
            continue;
        idIndex[list->id] = list;
        nameIndex[list->name] = list;
    }
}

void database::IDBRecommendationLists::ClearIndex()
{
    idIndex.clear();
    nameIndex.clear();
}

QList<QSharedPointer<core::AuthorRecommendationStats> > database::IDBRecommendationLists::GetAuthorStatsForList(int id)
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

QSharedPointer<core::AuthorRecommendationStats> database::IDBRecommendationLists::GetIndividualAuthorStatsForList(int id, int authorId)
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

int database::IDBRecommendationLists::GetMatchCountForRecommenderOnList(int authorId, int listId)
{
    auto data = GetIndividualAuthorStatsForList(listId, authorId);
    if(!data)
        return 0;
    return data->matchesWithReference;
}

QVector<int> database::IDBRecommendationLists::GetAllFicIDs(int listId)
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

bool database::IDBRecommendationLists::DeleteList(int listId)
{
    return database::puresql::DeleteRecommendationList(listId, db);
}

bool database::IDBRecommendationLists::ReloadList(int listId)
{

}

void database::IDBRecommendationLists::AddList(QSharedPointer<core::RecommendationList>)
{

}

QSharedPointer<core::RecommendationList> database::IDBRecommendationLists::NewList()
{
    return {new core::RecommendationList};
}

QSharedPointer<core::AuthorRecommendationStats> database::IDBRecommendationLists::CreateAuthorRecommendationStatsForList(int authorId,int listId)
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

    result.matchesWithReference = database::puresql::GetMatchesWithListIdInAuthorRecommendations(author->id, listId, db);
    if(result->matchesWithReference == 0)
        result->matchRatio = 999999;
    else
        result->matchRatio = (double)result->totalFics/(double)result->matchesWithReference;
    result.isValid = true;
    cachedAuthorStats[listId].push_back(result);
    return result;
}

bool database::IDBRecommendationLists::LoadAuthorRecommendationsIntoList(int authorId, int listId)
{
    return database::puresql::CopyAllAuthorRecommendationsToList(authorId, listId, db);
}

bool database::IDBRecommendationLists::LoadAuthorRecommendationStatsIntoDatabase(int listId, QSharedPointer<core::AuthorRecommendationStats> stats)
{
    return database::puresql::WriteAuthorRecommendationStatsForList(listId, stats, db);
}

bool database::IDBRecommendationLists::LoadListIntoDatabase(QSharedPointer<core::RecommendationList> list)
{
    AddList(list);
    auto timeStamp = portableDBInterface->GetCurrentDateTime();
    return database::puresql::CreateOrUpdateRecommendationList(list, timeStamp, db);
}

bool database::IDBRecommendationLists::UpdateFicCountInDatabase(int listId)
{
    return database::puresql::UpdateFicCountForRecommendationList(listId, db);
}

bool database::IDBRecommendationLists::AddAuthorFavouritesToList(int authorId, int listId, bool reloadLocalData)
{
    auto result = database::puresql::AddAuthorFavouritesToList(authorId, listId, db);
    if(result)
        return false;
    if(reloadLocalData)
        ReloadList(listId);
    return result;
}

bool database::ITags::DeleteTag(QString tag)
{
    database::puresql::DeleteTagfromDatabase(tag, db);
}

bool database::IDBFanfics::ReprocessFics(QString where, QString website, std::function<void (int)> f)
{
    auto list = database::puresql::GetWebIdList(where, website, db);
    for(auto id : list)
    {
        f(id);
    }
}

bool database::IDBFanfics::DeactivateFic(int ficId)
{
    return database::puresql::DeactivateStory(ficId, "ffn", db);
}
