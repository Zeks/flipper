#include "Interfaces/authors.h"
#include "Interfaces/db_interface.h"
#include "include/pure_sql.h"
#include <QSqlQuery>

namespace interfaces {
Authors::~Authors(){}
void Authors::Clear()
{
    authors.clear();
    ClearIndex();
    ClearCache();
}


void Authors::Reindex()
{
    ClearIndex();
    IndexAuthors();
}

void Authors::ClearIndex()
{
    authorsNamesByWebsite.clear();
    authorsByUrl.clear();
    authorsById.clear();
}

void Authors::ClearCache()
{
    cachedAuthorUrls.clear();
    cachedAuthorIds.clear();
    cachedAuthorToTagStats.clear();
}

void Authors::IndexAuthors()
{
    for(auto author : authors)
    {
        authorsById[author->id] = author;
        authorsNamesByWebsite[author->website][author->name] = author;
        authorsByUrl[author->url("ffn")] = author;
    }
}

bool Authors::EnsureAuthorLoaded(QString name, QString website)
{
    if(authorsNamesByWebsite.contains(website) && authorsNamesByWebsite[website].contains(name))
        return true;

    if(!LoadAuthor(name, website))
        return false;

    return true;
}

bool Authors::EnsureAuthorLoaded(QString url)
{
    if(authorsByUrl.contains(url))
        return true;

    if(!LoadAuthor(url))
        return false;

    return true;
}

bool Authors::EnsureAuthorLoaded(int id)
{
    if(authorsById.contains(id))
        return true;

    if(!LoadAuthor(id))
        return false;

    return true;
}

bool Authors::LoadAuthor(QString name, QString website)
{
    auto author = database::puresql::GetAuthorByNameAndWebsite(name, website,db);
    if(!author)
        return false;
    AddAuthorToIndex(author);
    return true;
}

bool Authors::LoadAuthor(QString url)
{
    auto author = database::puresql::GetAuthorByUrl(url,db);
    if(!author)
        return false;
    AddAuthorToIndex(author);
    return true;
}

bool Authors::LoadAuthor(int id)
{
    auto author = database::puresql::GetAuthorById(id,db);
    if(!author)
        return false;
    AddAuthorToIndex(author);
    return true;
}




core::AuthorPtr Authors::GetAuthorByNameAndWebsite(QString name, QString website)
{
    core::AuthorPtr result;
    if(EnsureAuthorLoaded(name, website))
        result = authorsNamesByWebsite[website][name];
    return result;

}

QList<core::AuthorPtr> Authors::GetAllByName(QString name)
{
    QList<core::AuthorPtr> result;
    auto websites = ListWebsites();

    for(auto bit : websites)
    {
        if(EnsureAuthorLoaded(name, bit))
            result.push_back(authorsNamesByWebsite[bit][name]);
    }
    return result;
}

core::AuthorPtr Authors::GetByUrl(QString url)
{
    core::AuthorPtr result;
    if(EnsureAuthorLoaded(url))
        result = authorsByUrl[url];

    return result;
}

QSharedPointer<core::Author > Authors::GetById(int id)
{
    core::AuthorPtr result;
    if(EnsureAuthorLoaded(id))
        result = authorsById[id];

    return result;
}

QList<core::AuthorPtr > Authors::GetAllAuthors(QString website, bool forced)
{
    QList<core::AuthorPtr> result;
    if(!LoadAuthors(website, forced))
        return result;

    if(website.isEmpty())
        return authors;

    if(authorsNamesByWebsite.contains(website))
        result = authorsNamesByWebsite[website].values();

    return result;
}

QStringList Authors::GetAllAuthorsUrls(QString website, bool forced)
{
    QStringList result;
    if(!cachedAuthorUrls.contains(website))
    {
        auto authors = GetAllAuthors(website, forced);
        result.reserve(authors.size());
        for(auto author: authors)
            result.push_back(author->url(website));

        cachedAuthorUrls[website] = result;
    }
    else
        result = cachedAuthorUrls[website];
    return result;
}

QList<int> Authors::GetAllAuthorIds()
{
    //! todo need to regenerate cache
    if(cachedAuthorIds.empty())
        cachedAuthorIds = database::puresql::GetAllAuthorIds(db);
    return cachedAuthorIds;
}

int Authors::GetFicCount(int authorId)
{
    int result = 0;

    if(EnsureAuthorLoaded(authorId))
        result = authorsById[authorId]->id;

    return result;

}

int Authors::GetCountOfRecsForTag(int authorId, QString tag)
{
    auto result = database::puresql::GetCountOfTagInAuthorRecommendations(authorId, tag, db);
    return result;
}

bool Authors::LoadAuthors(QString website, bool )
{
    authors = database::puresql::GetAllAuthors(website, db);
    if(authors.size() == 0)
        return false;
    Reindex();
    return true;
}

bool Authors::EnsureId(core::AuthorPtr author, QString website)
{
    if(!author)
        return false;

    QString url = author->url(website);
    if(authorsByUrl.contains(url) && authorsByUrl[url])
        author = authorsByUrl[url];

    if(author->GetIdStatus() == core::AuthorIdStatus::unassigned)
        author->AssignId(database::puresql::GetAuthorIdFromUrl(author->url(website), db));
    if(author->GetIdStatus() == core::AuthorIdStatus::not_found)
    {
        database::puresql::WriteAuthor(author,portableDBInterface->GetCurrentDateTime(), db);
        author->AssignId(database::puresql::GetAuthorIdFromUrl(author->url(website), db));
    }
    if(author->id < 0)
        return false;

    AddAuthorToIndex(author);
    return true;
}

void Authors::AddAuthorToIndex(core::AuthorPtr author)
{
    authors.push_back(author);

    authorsNamesByWebsite[author->website][author->name] = author;
    authorsById[author->id] = author;
    authorsByUrl[author->url(author->website)] = author;
}

bool Authors::AssignNewNameForAuthor(core::AuthorPtr author, QString name)
{
    if(!author)
        return false;

    return database::puresql::AssignNewNameForAuthor(author, name, db);
}


QSharedPointer<core::AuthorRecommendationStats> Authors::GetStatsForTag(int authorId, QSharedPointer<core::RecommendationList> list)
{
    QSharedPointer<core::AuthorRecommendationStats>result (new core::AuthorRecommendationStats);

    if(!EnsureAuthorLoaded(authorId))
        return result;

    auto author = authorsById[authorId];

    result->listId = list->id;
    result->usedTag = list->tagToUse;;
    result->authorName = author->name;
    result->authorId= author->id;
    result->totalFics= author->ficCount;

    result->matchesWithReference= database::puresql::GetCountOfTagInAuthorRecommendations(author->id, list->tagToUse, db);
    if(result->matchesWithReference == 0)
        result->matchRatio = 999999;
    else
        result->matchRatio = static_cast<double>(result->totalFics)/static_cast<double>(result->matchesWithReference);
    result->isValid = true;

    return result;
}
// those are required for managing recommendation lists and somewhat outdated
// moved them to dump temporarily
//bool  Authors::RemoveAuthor(int id)
//{
//    QSqlDatabase db = QSqlDatabase::database();
//    QSqlQuery q1(db);
//    QString qsl = "delete from recommendations where recommender_id = %1";
//    qsl=qsl.arg(QString::number(id));
//    q1.prepare(qsl);
//    if(!database::puresql::ExecAndCheck(q1))
//        return false;

//    QSqlQuery q2(db);
//    qsl = "delete from recommenders where id = %1";
//    qsl=qsl.arg(id);
//    q2.prepare(qsl);
//    if(!database::puresql::ExecAndCheck(q2))
//        return false;
//    return true;
//}

//bool Authors::RemoveAuthor(core::AuthorPtr author, QString website)
//{
//    int id = database::puresql::GetAuthorIdFromUrl(author->url(website),db);
//    if(id == -1)
//        return false;
//    return RemoveAuthor(id);
//}

QStringList Authors::ListWebsites()
{
    return {"ffn"};//{"ffn", "ao3", "sb", "sv"};
}
}
