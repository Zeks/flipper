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

QSharedPointer<QSharedPointer<core::Author> > database::IDBAuthors::GetByUrl(QString url)
{
    QSharedPointer<QSharedPointer<core::Author>> result;
    if(!authorsByUrl.contains(url))
        return result;
    return authorsByUrl[url];
}

QSharedPointer<QSharedPointer<core::Author> > database::IDBAuthors::GetById(int id)
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
