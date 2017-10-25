#include "Interfaces/authors.h"

namespace database {

void DBAuthorsBase::Clear()
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

void DBAuthorsBase::Reindex()
{
    ClearIndex();
    IndexAuthors();
}

void DBAuthorsBase::IndexAuthors()
{
    for(auto author : authors)
    {
        authorsById[author->id] = author;
        authorsByName[author->name] = author;
        authorsNamesByWebsite[author->website][author->name] = author;
        authorsByUrl[author->url("ffn")] = author;
    }
}

void DBAuthorsBase::ClearIndex()
{
    authorsByName.clear();
    authorsNamesByWebsite.clear();
    authorsByUrl.clear();
    authorsById.clear();
}

QSharedPointer<QSharedPointer<core::Author> > DBAuthorsBase::GetSingleByName(QString name, QString website)
{
    QSharedPointer<QSharedPointer<core::Author>> result;
    if(authorsNamesByWebsite.contains(website) && authorsNamesByWebsite[website].contains(name))
        result = {authorsNamesByWebsite[website][name]};
    return result;

}

QList<QSharedPointer<QSharedPointer<core::Author>>> DBAuthorsBase::GetAllByName(QString name)
{
    QList<QSharedPointer<QSharedPointer<core::Author>>> result;

    for(auto websiteHash : authorsNamesByWebsite)
    {
        if(websiteHash.contains(name))
            result.push_back(websiteHash[name]);
    }

    return result;
}

QSharedPointer<core::Author> DBAuthorsBase::GetByUrl(QString url)
{
    QSharedPointer<QSharedPointer<core::Author>> result;
    if(!authorsByUrl.contains(url))
        return result;
    return authorsByUrl[url];
}

QSharedPointer<core::Author > DBAuthorsBase::GetById(int id)
{
    QSharedPointer<QSharedPointer<core::Author>> result;
    if(!authorsById.contains(id))
        return result;
    return authorsById[id];
}

QList<QSharedPointer<core::Author> > DBAuthorsBase::GetAllAuthors(QString website)
{
    if(website.isEmpty())
        return authors;
    if(!authorsNamesByWebsite.contains(website))
        return authors;
    return authorsNamesByWebsite[website];
}

int DBAuthorsBase::GetFicCount(int authorId)
{
    auto author = GetById(authorId);
    if(!author)
        return 0;
    return author->ficCount;
}

int DBAuthorsBase::GetCountOfRecsForTag(int authorId, QString tag)
{
    auto result = puresql::GetCountOfTagInAuthorRecommendations(authorId, tag, db);
    return result;
}
bool DBAuthorsBase::LoadAuthors(QString website, bool additionMode)
{
    if(!additionMode)
        Clear();
    authors = database::puresql::GetAllAuthors(website, db);
    Reindex();

}
bool DBAuthorsBase::EnsureId(QSharedPointer<core::Author> author, QString website)
{
    if(!author)
        return false;

    if(author->GetIdStatus() == core::AuthorIdStatus::unassigned)
        author->AssignId(database::puresql::GetAuthorIdFromUrl(author->url(website), db));
    if(author->GetIdStatus() == core::AuthorIdStatus::not_found)
    {
        database::sqlite::WriteAuthor(author, db);
        author->AssignId(database::puresql::GetAuthorIdFromUrl(author->url(website), db));
    }
    if(author->id < 0)
        return false;
    return true;
}


QSharedPointer<core::AuthorRecommendationStats> DBAuthorsBase::GetStatsForTag(authorId, core::RecommendationList list)
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

void  DBAuthorsBase::RemoveAuthor(int id)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = "delete from recommendations where recommender_id = %1";
    qsl=qsl.arg(QString::number(id));
    q1.prepare(qsl);
    ExecAndCheck(q1);

    QSqlQuery q2(db);
    qsl = "delete from recommenders where id = %1";
    qsl=qsl.arg(id);
    q2.prepare(qsl);
    ExecAndCheck(q2);
}

void DBAuthorsBase::RemoveAuthor(QSharedPointer<core::Author> author, QString website)
{
    int id = GetAuthorIdFromUrl(author.url(website));
    RemoveAuthor(id);
}
}
