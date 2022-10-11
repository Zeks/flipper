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
#include "Interfaces/authors.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/db_interface.h"
#include "include/pure_sql.h"
#include "include/sqlitefunctions.h"
#include "sql_abstractions/sql_query.h"
#include <QDebug>

namespace interfaces {





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
    //authorsByUrl.clear();
    authorsById.clear();
}

void Authors::ClearCache()
{
    cachedAuthorUrls.clear();
    cachedAuthorIds.clear();
    cachedAuthorToTagStats.clear();
}

void Authors::AddPreloadedAuthor(const core::AuthorPtr& author)
{
    if(!author)
        return;
    if(authorsById.contains(author->id))
        return;
    AddAuthorToIndex(author);
}

void Authors::IndexAuthors()
{
    for(const auto& author : qAsConst(authors))
    {
        authorsById[author->id] = author;
        const auto websites = author->GetWebsites();
        for(auto& key : websites)
        {
            authorsNamesByWebsite[key][author->name] = author;
            authorsByWebID[key][author->GetWebID(key)] = author;
        }

        //authorsByUrl[author->url("ffn")] = author;
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

bool Authors::EnsureAuthorLoaded(QString website, int id)
{
    if(authorsByWebID[website].contains(id))
        return true;

    if(!LoadAuthor(website, id))
        return false;

    return true;
}

//bool Authors::EnsureAuthorLoaded(QString url)
//{
//    if(authorsByUrl.contains(url))
//        return true;

//    if(!LoadAuthor(url))
//        return false;

//    return true;
//}

bool Authors::EnsureAuthorLoaded(int id)
{
    if(authorsById.contains(id))
        return true;

    if(!LoadAuthor(id))
        return false;

    return true;
}

bool Authors::UpdateAuthorRecord(const core::AuthorPtr& author)
{
    // todo unnecessary indirection to database tijme, can assign timestamp in the query
    auto result = sql::UpdateAuthorRecord(author,database::sqlite::GetCurrentDateTime(db), db);
    sql::WipeAuthorStatistics(author,db);
    //ConvertFandomsToIds(author, fandomInterface);
    auto f1Result = sql::WriteAuthorFavouriteStatistics(author,db);
    auto f2Result = sql::WriteAuthorFavouriteGenreStatistics(author,db);
    auto f3Result = sql::WriteAuthorFavouriteFandomStatistics(author,db);

    return result.success && f1Result.success && f2Result.success && f3Result.success;
}

bool Authors::UpdateAuthorFavouritesUpdateDate(core::AuthorPtr author)
{
    if(!author)
        return false;

    auto result = sql::UpdateAuthorFavouritesUpdateDate(author->id, QDate(author->stats.favouritesLastUpdated).startOfDay(), db).success;
    return result;
}

bool Authors::LoadAuthor(QString name, QString website)
{
    auto result = sql::GetAuthorByNameAndWebsite(name, website,db);
    auto author = result.data;
    if(!result.success || !author)
        return false;
    bool success = true;
    success = success && sql::LoadAuthorStatistics(author, db).success;
    AddAuthorToIndex(author);
    return success;
}

bool Authors::LoadAuthor(QString website, int id)
{
    auto result = sql::GetAuthorByIDAndWebsite(id, website, db);
    auto author = result.data;
    if(!result.success || !author)
        return false;

    bool success = true;
    success = success && sql::LoadAuthorStatistics(author, db).success;
    AddAuthorToIndex(author);
    return true;
}

bool Authors::LoadAuthor(int id)
{
    auto result = sql::GetAuthorById(id,db);
    auto author = result.data;
    if(!result.success || !author)
        return false;

    bool success = true;
    success = success && sql::LoadAuthorStatistics(author, db).success;
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

    for(auto& bit : qAsConst(websites))
    {
        if(EnsureAuthorLoaded(name, bit))
            result.push_back(authorsNamesByWebsite[bit][name]);
    }
    return result;
}

QSet<int> Authors::GetAllMatchesWithRecsUID(QSharedPointer<core::RecommendationList> params, QString uid)
{
    return sql::GetAllMatchesWithRecsUID(params, uid, db).data;
}

core::AuthorPtr Authors::GetByWebID(QString website, int id)
{
    core::AuthorPtr result;
    if(EnsureAuthorLoaded(website, id))
        result = authorsByWebID[website][id];

    return result;
}

int Authors::GetRecommenderIDByFFNId(int id)
{
    return sql::GetRecommenderIDByFFNId(id, db).data;
}

//core::AuthorPtr Authors::GetByUrl(QString url)
//{
//    core::AuthorPtr result;
//    if(EnsureAuthorLoaded(url))
//        result = authorsByUrl[url];

//    return result;
//}

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

QList<core::AuthorPtr> Authors::GetAllAuthorsLimited(QString website, int limit)
{
    QList<core::AuthorPtr> result;
    result = sql::GetAllAuthors(website, db, limit).data;
    for(auto& author: qAsConst(result))
        sql::LoadAuthorStatistics(author, db);
    return result;
}

QList<core::AuthorPtr> Authors::GetAllAuthorsWithFavUpdateSince(QString website,
                                                                QDateTime date,
                                                                int limit)
{
    QList<core::AuthorPtr> result;
    result = sql::GetAllAuthorsWithFavUpdateSince(website,date, db, limit).data;
    for(const auto& author: qAsConst(result))
        sql::LoadAuthorStatistics(author, db);
    return result;
}

QList<core::AuthorPtr> Authors::GetAllAuthorsWithFavUpdateBetween(QString website,
                                                                 QDateTime dateStart,
                                                                 QDateTime dateEnd, int limit)
{
    QList<core::AuthorPtr> result;
    result = sql::GetAllAuthorsWithFavUpdateBetween(website,dateStart, dateEnd, db, limit).data;
    for(const auto& author: std::as_const(result))
        sql::LoadAuthorStatistics(author, db);
    return result;
}

QStringList Authors::GetAllAuthorsUrls(QString website, bool forced)
{
    QStringList result;
    if(!cachedAuthorUrls.contains(website))
    {
        auto authors = GetAllAuthors(website, forced);
        result.reserve(authors.size());
        for(const auto& author: std::as_const(authors))
            result.push_back(author->url(website));

        cachedAuthorUrls[website] = result;
    }
    else
        result = cachedAuthorUrls[website];
    return result;
}

QStringList Authors::GetAllAuthorsFavourites(int id)
{
    auto result = sql::GetAllAuthorFavourites(id, db);
    return result.data;
}

QList<int> Authors::GetAllAuthorRecommendationIDs(int id)
{
    auto result = sql::GetAllAuthorRecommendationIDs(id, db);
    return result.data;
}

QList<int> Authors::GetAllAuthorIds()
{
    //! todo need to regenerate cache
    if(cachedAuthorIds.empty())
        cachedAuthorIds = sql::GetAllAuthorIds(db).data;
    return cachedAuthorIds;
}

int Authors::GetFicCount(int authorId)
{
    int result = 0;

    if(EnsureAuthorLoaded(authorId))
        result = authorsById[authorId]->id;

    return result;

}

QList<int> Authors::GetFicList(const core::AuthorPtr& author) const
{
    QList<int> result;
    if(!author)
        return result;

    auto sqlResult = sql::GetAllAuthorRecommendations(author->id,  db);
    result = sqlResult.data;
    return result;
}

int Authors::GetCountOfRecsForTag(int authorId, QString tag)
{
    auto result = sql::GetCountOfTagInAuthorRecommendations(authorId, tag, db).data;
    return result;
}

bool Authors::LoadAuthors(QString website, bool )
{
    authors = sql::GetAllAuthors(website, db).data;
    for(const auto& author: std::as_const(authors))
        sql::LoadAuthorStatistics(author, db);
    if(authors.size() == 0)
        return false;
    Reindex();
    return true;
}

QHash<int, QSet<int> > Authors::LoadFullFavouritesHashset()
{
    auto result = sql::LoadFullFavouritesHashset(db).data;
    return result;
}

void LoadIDForAuthor(const core::AuthorPtr& author, sql::Database db)
{
    const auto websites = author->GetWebsites();
    for(const auto& website : websites)
    {
        auto id = sql::GetAuthorIdFromWebID(author->GetWebID(website),website, db).data;
        author->AssignId(id);
        if(id > -1)
            break;
    }
}



bool Authors::CreateAuthorRecord(core::AuthorPtr author)
{


    // todo unnecessary indirection to database tijme, can assign timestamp in the query
    auto result = sql::CreateAuthorRecord(author,database::sqlite::GetCurrentDateTime(db), db);
    sql::WipeAuthorStatistics(author,db);
    //ConvertFandomsToIds(author, fandomInterface);
    auto f1Result = sql::WriteAuthorFavouriteStatistics(author,db);
    auto f2Result = sql::WriteAuthorFavouriteGenreStatistics(author,db);
    auto f3Result = sql::WriteAuthorFavouriteFandomStatistics(author,db);

    return result.success && f1Result.success && f2Result.success && f3Result.success;
}

bool Authors::EnsureId(core::AuthorPtr author, QString website)
{
    if(!author)
        return false;

    //QString url = author->url(website);
    auto webId = author->GetWebID(website);
    const auto& it = authorsByWebID[website].constFind(webId);
    if(it != authorsByWebID[website].cend() && *it)
        author = *it;

    if(author->GetIdStatus() == core::AuthorIdStatus::unassigned)
        LoadIDForAuthor(author, db);

    if(author->GetIdStatus() == core::AuthorIdStatus::not_found)
    {
        CreateAuthorRecord(author);
        LoadIDForAuthor(author, db);
    }
    if(author->id < 0)
        return false;

    AddAuthorToIndex(author);
    return true;
}

void Authors::AddAuthorToIndex(core::AuthorPtr author)
{
    authors.push_back(author);

    const auto websites = author->GetWebsites();
    for(const auto& key : websites)
    {
        authorsNamesByWebsite[key][author->name] = author;
        auto webId = author->GetWebID(key);
        authorsByWebID[key][webId] = author;
    }
    authorsById[author->id] = author;
    //    for(auto key : author->GetWebsites())
    //        authorsByUrl[author->url(key)] = author;

}

bool Authors::AssignNewNameForAuthor(core::AuthorPtr author, QString name)
{
    if(!author)
        return false;

    return sql::AssignNewNameForAuthor(author, name, db).success;
}

QSet<int> Authors::GetAuthorsForFics(QSet<int> fics)
{
    return sql::GetAuthorsForFics(fics, db).data;
}

QSet<int> Authors::GetRecommendersForFics(QSet<int> fics)
{
    return sql::GetRecommendersForFics(fics, db).data;
}

QHash<uint32_t, int> Authors::GetHashAuthorsForFics(QSet<int> fics)
{
    return sql::GetHashAuthorsForFics(fics, db).data;
}

bool Authors::AssignAuthorNamesForWebIDsInFanficTable()
{
    return sql::AssignAuthorNamesForWebIDsInFanficTable(db).data;
}

QHash<int, std::array<double, 22> > Authors::GetListGenreData()
{
    return sql::GetListGenreData(db).data;
}

QHash<int, core::AuthorFavFandomStatsPtr> Authors::GetAuthorListFandomStatistics(QList<int> authors)
{
    return sql::GetAuthorListFandomStatistics(authors, db).data;
}

QHash<uint32_t, genre_stats::ListMoodData> Authors::GetMoodDataForLists()
{
    return sql::GetMoodDataForLists(db).data;
}


QSharedPointer<core::AuthorRecommendationStats> Authors::GetStatsForTag(int authorId, QSharedPointer<core::RecommendationList> list)
{
    QSharedPointer<core::AuthorRecommendationStats>result (new core::AuthorRecommendationStats);

    if(!EnsureAuthorLoaded(authorId))
        return result;

    auto& author = authorsById[authorId];

    result->listId = list->id;
    result->usedTag = list->tagToUse;;
    result->authorName = author->name;
    result->authorId= author->id;
    result->totalRecommendations= author->recCount;

    result->matchesWithReference= sql::GetCountOfTagInAuthorRecommendations(author->id, list->tagToUse, db).data;
    if(result->matchesWithReference == 0)
        result->matchRatio = std::numeric_limits<double>::max();
    else
    {
        //qDebug() << "Have matches for: " << author->name;
        result->matchRatio = static_cast<double>(result->totalRecommendations)/static_cast<double>(result->matchesWithReference);
    }
    result->isValid = true;

    return result;
}

//bool Authors::UploadLinkedAuthorsForAuthor(int authorId, QStringList list)
//{
//    if(!EnsureAuthorLoaded(authorId) || list.isEmpty())
//        return false;
//    return sql::UploadLinkedAuthorsForAuthor(authorId, list, db);
//}

bool Authors::UploadLinkedAuthorsForAuthor(int authorId, QString website , const QList<int>& ids)
{
    if(!EnsureAuthorLoaded(authorId) || ids.isEmpty())
        return false;
    return sql::UploadLinkedAuthorsForAuthor(authorId, website, ids, db).success;
}

bool Authors::DeleteLinkedAuthorsForAuthor(int authorId)
{
    if(!EnsureAuthorLoaded(authorId))
        return false;
    return sql::DeleteLinkedAuthorsForAuthor(authorId, db).success;
}

QVector<int> Authors::GetAllUnprocessedLinkedAuthors()
{
    return sql::GetAllUnprocessedLinkedAuthors(db).data;
}

bool Authors::WipeAuthorStatisticsRecords()
{
    return true;
}

bool Authors::CreateStatisticsRecordsForAuthors()
{
    return true;
}

bool Authors::CalculateSlashStatisticsPercentages(QString fieldUsed)
{
    auto result = sql::CalculateSlashStatisticsPercentages(fieldUsed, db);
    return result.success;
}

// those are required for managing recommendation lists and somewhat outdated
// moved them to dump temporarily
//bool  Authors::RemoveAuthor(int id)
//{
//    sql::Database db = sql::Database::database();
//    sql::Query q1(db);
//    QString qsl = "delete from recommendations where recommender_id = %1";
//    qsl=qsl.arg(QString::number(id));
//    q1.prepare(qsl);
//    if(!sql::ExecAndCheck(q1))
//        return false;

//    sql::Query q2(db);
//    qsl = "delete from recommenders where id = %1";
//    qsl=qsl.arg(id);
//    q2.prepare(qsl);
//    if(!sql::ExecAndCheck(q2))
//        return false;
//    return true;
//}

//bool Authors::RemoveAuthor(core::AuthorPtr author, QString website)
//{
//    int id = sql::GetAuthorIdFromUrl(author->url(website),db);
//    if(id == -1)
//        return false;
//    return RemoveAuthor(id);
//}

QStringList Authors::ListWebsites()
{
    return {"ffn"};//{"ffn", "ao3", "sb", "sv"};
}
}
