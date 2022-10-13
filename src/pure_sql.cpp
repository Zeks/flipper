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
#include "pure_sql.h"
#include "transaction.h"
#include "core/section.h"
#include "pagetask.h"
#include "url_utils.h"
#include "Interfaces/genres.h"

#include "include/in_tag_accessor.h"
#include "GlobalHeaders/snippets_templates.h"
#include "fmt/format.h"
#include "sql_abstractions/sql_query.h"
#include "sql_abstractions/sql_database.h"
#include <QSqlError>
#include <QVector>
#include <QVariant>
#include <QDebug>
#include <algorithm>
#include<QSqlDriver>

namespace sql{

template <typename T>
bool NullPtrGuard(T item)
{
    if(!item)
    {
        qDebug() << "attempted to fill a nullptr";
        return false;
    }
    return true;
}
#define DATAQ [&](auto& data, auto q)
#define DATAQN [&](auto& , auto q)
#define COMMAND(NAME)  { #NAME, NAME}

#define BP1(X) {COMMAND(X)}
#define BP2(X, Y) {COMMAND(X), COMMAND(Y)}
#define BP3(X, Y, Z) {COMMAND(X), COMMAND(Y), COMMAND(Z)}
#define BP4(X, Y, Z, W) {COMMAND(X), COMMAND(Y), COMMAND(Z), COMMAND(W)}

//#define BP4(X, Y, Z, W) {{"X", X}, {"Y", Y},{"Z", Z},{"W", W}}

static DiagnosticSQLResult<FicIdHash> GetGlobalIDHash(sql::Database db, QString where)
{
    std::string qs = "select id, ffn_id, ao3_id, sb_id, sv_id from fanfics ";
    if(!where.isEmpty())
        qs+=where.toStdString();

    SqlContext<FicIdHash> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data.ids["ffn"][q.value("ffn_id").toInt()] = q.value("id").toInt();
        ctx.result.data.ids["sb"][q.value("sb_id").toInt()] = q.value("id").toInt();
        ctx.result.data.ids["sv"][q.value("sv_id").toInt()] = q.value("id").toInt();
        ctx.result.data.ids["ao3"][q.value("ao3_id").toInt()] = q.value("id").toInt();
        FanficIdRecord rec;
        rec.ids["ffn"] = q.value("ffn_id").toInt();
        rec.ids["sb"] = q.value("sb_id").toInt();
        rec.ids["sv"] = q.value("sv_id").toInt();
        rec.ids["ao3"] = q.value("ao3_id").toInt();
        ctx.result.data.records[q.value("id").toInt()] = rec;
    });
    return std::move(ctx.result);
}

bool CheckExecution(sql::Query& q)
{
    if(q.lastError().isValid())
    {
        qDebug() << "SQLERROR: "<< q.lastError().text();
        qDebug() << q.lastQuery();
        return false;
    }
    return true;
}


#define SQLPARAMS
#define CTXA [&](auto ctx)
DiagnosticSQLResult<bool> SetFandomTracked(int id, bool tracked,  sql::Database db)
{
    return SqlContext<bool> (db,
                             " UPDATE fandomindex SET tracked = :tracked where id = :id",
                             {{"tracked",tracked ? "1" : "0"},
                                       {"id", id}})();
}

DiagnosticSQLResult<bool> CalculateFandomsFicCounts(sql::Database db)
{
    std::string qs = "update fandomindex set fic_count = (select count(fic_id) from ficfandoms where fandom_id = fandoms.id)";
    return SqlContext<bool> (db, std::move(qs))();
}

DiagnosticSQLResult<bool> UpdateFandomStats(int fandomId, sql::Database db)
{
    std::string qs = "update fandomsources set fic_count = "
                         " (select count(fic_id) from ficfandoms where fandom_id = :fandom_id)"
                         //! todo
                         //" average_faves_top_3 = (select sum(favourites)/3 from fanfics f where f.fandom = fandoms.fandom and f.id "
                         //" in (select id from fanfics where fanfics.fandom = fandoms.fandom order by favourites desc limit 3))"
                         " where fandoms.id = :fandom_id";

    SqlContext<bool> ctx(db, std::move(qs), {{"fandom_id",fandomId}});
    if(fandomId == -1)
        return std::move(ctx.result);
    return ctx();
}

DiagnosticSQLResult<bool> WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom, sql::Database db)
{
    auto result = Internal::WriteMaxUpdateDateForFandom(fandom, "having count(*) = 1", db,
                                                        [](QSharedPointer<core::Fandom> f, QDateTime dt){
            f->lastUpdateDate = dt.date();
});
    return result;
}



DiagnosticSQLResult<bool>  Internal::WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom,
                                                                 QString condition,
                                                                 sql::Database db,
                                                                 std::function<void(QSharedPointer<core::Fandom>,QDateTime)> writer                                           )
{
    std::string qs = "select max(updated) as updated from fanfics where id in (select distinct fic_id from FicFandoms where fandom_id = :fandom_id {0}";
    qs=fmt::format(qs,condition.toStdString());

    DiagnosticSQLResult<bool> opResult;
    SqlContext<QString> ctx(db, std::move(qs));
    ctx.FetchSingleValue<QString>("updated", "");
    opResult.success = ctx.result.success;
    if(!opResult.success)
        return opResult;

    QDateTime result;
    result = QDateTime::fromString(ctx.result.data, "yyyy-MM-ddThh:mm:ss.000");
    writer(fandom, result);
    return opResult;
}
//! todo  requires refactor. fandoms need to have a tag table attached to them instead of forced sections
DiagnosticSQLResult<QStringList> GetFandomListFromDB(sql::Database db)
{
    std::string qs = "select name from fandomindex";

    SqlContext<QStringList> ctx(db);
    ctx.result.data.push_back("");
    ctx.FetchLargeSelectIntoList<QString>("name", std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> AssignTagToFandom(QString tag, int fandom_id, sql::Database db, bool includeCrossovers)
{
    std::string qs = "INSERT INTO FicTags(fic_id, tag) SELECT fic_id, '{0}' as tag from FicFandoms f WHERE fandom_id = :fandom_id "
                 " and NOT EXISTS(SELECT 1 FROM FicTags WHERE fic_id = f.fic_id and tag = '{0}')";
    if(!includeCrossovers)
        qs+=" and (select count(distinct fandom_id) from ficfandoms where fic_id = f.fic_id) = 1";
    qs=fmt::format(qs,tag.toStdString());

    SqlContext<bool> ctx(db, std::move(qs), {{"fandom_id", fandom_id}});
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}



DiagnosticSQLResult<bool> AssignTagToFanfic(QString tag, int fic_id, sql::Database db)
{
    std::string qs = "INSERT INTO FicTags(fic_id, tag, added) values(:fic_id, :tag, date('now'))";
    SqlContext<bool> ctx(db, std::move(qs), {{"tag", tag},{"fic_id", fic_id}});
    ctx.ExecAndCheck(true);
    //    ctx.ReplaceQuery("update fanfics set hidden = 1 where id = :fic_id";
    //    ctx.bindValue("fic_id", fic_id);
    //    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}


DiagnosticSQLResult<bool> RemoveTagFromFanfic(QString tag, int fic_id, sql::Database db)
{
    std::string qs = "delete from FicTags where fic_id = :fic_id and tag = :tag";
    SqlContext<bool> ctx (db, std::move(qs), {{"tag", tag},{"fic_id", fic_id}});
    ctx();

    //TODO need to  fix this... probably
//    ctx.ReplaceQuery("update fanfics set hidden = case "
//                     " when (select count(fic_id) from fictags where fic_id = :fic_id) > 0 then 1 "
//                     " else 0 end "
//                     " where id = :fic_id_";
//    ctx.bindValue("fic_id", fic_id);
//    ctx.bindValue("fic_id_", fic_id);
//    ctx.ExecAndCheck();
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> AssignSlashToFanfic(int fic_id, int source, sql::Database db)
{
    std::string qs = "update fanfics set slash_probability = 1, slash_source = :source where id = :fic_id";
    return SqlContext<bool> (db, std::move(qs), {{"source", source},{"fic_id", fic_id}})();
}

DiagnosticSQLResult<bool> AssignQueuedToFanfic(int fic_id, sql::Database db)
{
    std::string qs = "update fanfics set queued_for_action = 1 where id = :fic_id";
    return SqlContext<bool> (db, std::move(qs), {{"fic_id", fic_id}})();
}

DiagnosticSQLResult<bool> AssignChapterToFanfic(int fic_id, int chapter, sql::Database db)
{
    std::string qs = "INSERT INTO FicReadingTracker(fic_id, at_chapter) values(:fic_id, :at_chapter) "
                 "on conflict (fic_id) do update set at_chapter = :at_chapter_ where fic_id = :fic_id_";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("fic_id", fic_id);
    ctx.bindValue("at_chapter", chapter);
    ctx.bindValue("at_chapter_", chapter);
    ctx.bindValue("fic_id_", fic_id);
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}
DiagnosticSQLResult<bool> AssignScoreToFanfic(int score, int fic_id, sql::Database db)
{
    if(score > 0)
    {
        std::string qs = "INSERT INTO FicSCores(fic_id, score) values(:fic_id, :score) on conflict (fic_id) do update set score = :score where fic_id = :fic_id";
        SqlContext<bool> ctx(db, std::move(qs), BP4(fic_id, score, score, fic_id));
        ctx.ExecAndCheck(true);
        return std::move(ctx.result);
    }
    else {
        std::string qs = " delete from FicScores where fic_id  = :fic_id";
        return SqlContext<bool>(db, std::move(qs), BP1(fic_id))();
    }
}

DiagnosticSQLResult<int> GetLastFandomID(sql::Database db){
    std::string qs = "Select max(id) as maxid from fandomindex";

    SqlContext<int> ctx(db);
    //qDebug() << "Db open: " << db.isOpen() << " " << db.connectionName();
    ctx.FetchSingleValue<int>("maxid", -1, true, std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteFandomUrls(core::FandomPtr fandom, sql::Database db)
{
    std::string qs = "insert into fandomurls(global_id, url, website, custom) values(:id, :url, :website, :custom)";

    SqlContext<bool> ctx(db, std::move(qs));
    if(fandom->urls.size() == 0)
        fandom->urls.push_back(core::Url(QStringLiteral(""), "ffn"));
    ctx.ExecuteWithKeyListAndBindFunctor<QList,core::Url>(fandom->urls, [&](core::Url&& url, sql::Query& q){
        q.bindValue("id", fandom->id);
        q.bindValue("url", url.GetUrl());
        q.bindValue("website", fandom->source);
        q.bindValue("custom", fandom->section);
    }, true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> CreateFandomInDatabase(core::FandomPtr fandom, sql::Database db, bool writeUrls, bool useSuppliedIds)
{
    SqlContext<bool> ctx(db);


    int newFandomId = 0;
    if(useSuppliedIds)
        newFandomId  = fandom->id;
    else
    {
        auto lastId = GetLastFandomID(db);

        if(!lastId.success)
            ctx.result.success = false;
        newFandomId = lastId.data + 1;
    }
    if(newFandomId == -1)
        return std::move(ctx.result);

    std::string qs = "insert into fandomindex(id, name) "
                         " values(:id, :name)";
    ctx.ReplaceQuery(std::move(qs));
    ctx.bindValue("name", fandom->GetName());
    ctx.bindValue("id", newFandomId);
    //qDebug() << db.isOpen();
    if(!ctx.ExecAndCheck())
        return std::move(ctx.result);

    qDebug() << "new fandom: " << fandom->GetName();

    fandom->id = newFandomId;
    if(writeUrls)
        ctx.result.success = WriteFandomUrls(fandom, db).success;
    return std::move(ctx.result);

}
DiagnosticSQLResult<int> GetFicIdByAuthorAndName(QString author, QString title, sql::Database db)
{
    std::string qs = " select id from fanfics where author = :author and title = :title";

    SqlContext<int> ctx(db, std::move(qs), {{"author", author},{"title", title}});
    ctx.FetchSingleValue<int>("id", -1);
    return std::move(ctx.result);
}

DiagnosticSQLResult<int> GetFicIdByWebId(QString website, int webId, sql::Database db)
{
    std::string qs = " select id from fanfics where {0}_id = :site_id";
    qs=fmt::format(qs,website.toStdString());

    SqlContext<int> ctx(db, std::move(qs), {{"site_id",webId}});
    ctx.FetchSingleValue<int>("id", -1);
    return std::move(ctx.result);
}

core::FicPtr LoadFicFromQuery(const sql::Query& q1, QString website = "ffn")
{
    Q_UNUSED(website)
    auto fic = core::Fanfic::NewFanfic();
    fic->userData.atChapter = q1.value("AT_CHAPTER").toInt();
    fic->complete  = q1.value("COMPLETE").toInt();

    fic->identity.id = q1.value("ID").toInt();
    fic->wordCount = QString::fromStdString(q1.value("WORDCOUNT").toString());
    //fic->chapters = QString::fromStdString(q1.value("CHAPTERS").toString());
    fic->reviews = QString::fromStdString(q1.value("REVIEWS").toString());
    fic->favourites = QString::fromStdString(q1.value("FAVOURITES").toString());
    fic->follows = QString::fromStdString(q1.value("FOLLOWS").toString());
    fic->rated = QString::fromStdString(q1.value("RATED").toString());
    fic->fandom = QString::fromStdString(q1.value("FANDOM").toString());
    fic->title = QString::fromStdString(q1.value("TITLE").toString());
    fic->genres = QString::fromStdString(q1.value("GENRES").toString()).split("##");
    fic->summary = QString::fromStdString(q1.value("SUMMARY").toString());
    fic->published = q1.value("PUBLISHED").toDateTime();
    fic->updated = q1.value("UPDATED").toDateTime();
    fic->characters = QString::fromStdString(q1.value("CHARACTERS").toString()).split(",");
    //fic->authorId = q1.value("AUTHOR_ID".toInt();
    fic->author->name = QString::fromStdString(q1.value("AUTHOR").toString());
    fic->identity.web.ffn = q1.value("FFN_ID").toInt();
    fic->identity.web.ao3 = q1.value("AO3_ID").toInt();
    fic->identity.web.sb = q1.value("SB_ID").toInt();
    fic->identity.web.sv = q1.value("SV_ID").toInt();
    return fic;
}

DiagnosticSQLResult<core::FicPtr> GetFicByWebId(QString website, int webId, sql::Database db)
{
    std::string qs = fmt::format(" select * from fanfics where {0}_id = :site_id",website.toStdString());
    SqlContext<core::FicPtr> ctx(db, std::move(qs), {{"site_id",webId}});
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data =  LoadFicFromQuery(q);
        ctx.result.data->webSite = website;
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<core::FicPtr> GetFicById( int ficId, sql::Database db)
{
    std::string qs = " select * from fanfics where id = :fic_id";
    SqlContext<core::FicPtr> ctx(db, std::move(qs), {{"fic_id",ficId}});
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data =  LoadFicFromQuery(q);
    });
    return std::move(ctx.result);
}


DiagnosticSQLResult<bool> SetUpdateOrInsert(QSharedPointer<core::Fanfic> fic, sql::Database db, bool alwaysUpdateIfNotInsert)
{
    auto getKeyQuery = fmt::format("Select ( select count(*) from FANFICS where  {0}_id = :site_id1) as COUNT_NAMED,"
                                  " ( select count(*) from FANFICS where  {0}_id = :site_id2 and (updated < :updated or updated is null)) as count_updated,"
                                  " ( select count(*) from FANFICS where  {0}_id = :site_id3 and (favourites < :favourites or favourites is null)) as count_faved"
                                   ,fic->webSite.toStdString());

    SqlContext<bool> ctx(db, std::move(getKeyQuery));
    ctx.bindValue("site_id1", fic->identity.web.GetPrimaryId());
    ctx.bindValue("site_id2", fic->identity.web.GetPrimaryId());
    ctx.bindValue("updated", fic->updated);
    ctx.bindValue("site_id3", fic->identity.web.GetPrimaryId());
    ctx.bindValue("favourites", fic->favourites);
    if(!fic)
        return std::move(ctx.result);

    int countNamed = 0;
    int countUpdated = 0;
    int countUpdatedFaves = 0;
    ctx.ForEachInSelect([&](sql::Query& q){
        countNamed = q.value("COUNT_NAMED").toInt();
        countUpdated = q.value("count_updated").toInt();
        countUpdatedFaves = q.value("count_faved").toInt();
    });
    if(!ctx.Success())
        return std::move(ctx.result);

    bool requiresInsert = countNamed == 0;
    bool requiresUpdate = countUpdated > 0 || countUpdatedFaves > 0;
//    if(fic->fandom.contains("Greatest Showman"))
//        qDebug() << fic->fandom;
    if(alwaysUpdateIfNotInsert || (!requiresInsert && requiresUpdate))
        fic->updateMode = core::UpdateMode::update;
    if(requiresInsert)
        fic->updateMode = core::UpdateMode::insert;

    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> InsertIntoDB(QSharedPointer<core::Fanfic> section, sql::Database db)
{
    std::string query = "INSERT INTO FANFICS ({0}_id, FANDOM, AUTHOR, TITLE,WORDCOUNT, CHAPTERS, FAVOURITES, REVIEWS, "
                    " CHARACTERS, COMPLETE, RATED, SUMMARY, GENRES, PUBLISHED, UPDATED, AUTHOR_ID,"
                    " wcr, reviewstofavourites, age, daysrunning, at_chapter, lastupdate, fandom1, fandom2, author_id ) "
                    "VALUES ( :site_id,  :fandom, :author, :title, :wordcount, :CHAPTERS, :FAVOURITES, :REVIEWS, "
                    " :CHARACTERS, :COMPLETE, :RATED, :summary, :genres, :published, :updated, :author_id,"
                    " :wcr, :reviewstofavourites, :age, :daysrunning, 0, date('now'), :fandom1, :fandom2, :author_id)";

    query=fmt::format(query,section->webSite.toStdString());

    SqlContext<bool> ctx(db, std::move(query));
    ctx.bindValue("site_id",section->identity.web.GetPrimaryId()); //?
    ctx.bindValue("fandom",section->fandom);
    ctx.bindValue("author",section->author->name); //?
    ctx.bindValue("author_id",section->author->GetWebID("ffn"));
    ctx.bindValue("title",section->title);
    ctx.bindValue("wordcount",section->wordCount.toInt());
    ctx.bindValue("CHAPTERS",section->chapters.trimmed().toInt());
    ctx.bindValue("FAVOURITES",section->favourites.toInt());
    ctx.bindValue("REVIEWS",section->reviews.toInt());
    ctx.bindValue("CHARACTERS",section->charactersFull);
    ctx.bindValue("RATED",section->rated);
    ctx.bindValue("summary",section->summary);
    ctx.bindValue("COMPLETE",section->complete);
    ctx.bindValue("genres",section->genreString);
    ctx.bindValue("published",section->published);
    ctx.bindValue("updated",section->updated);
    ctx.bindValue("wcr",section->statistics.wcr);
    ctx.bindValue("reviewstofavourites",section->statistics.reviewsTofavourites);
    ctx.bindValue("age",section->statistics.age);
    ctx.bindValue("daysrunning",section->statistics.daysRunning);
    if(section->fandomIds.size() > 0)
        ctx.bindValue("fandom1",section->fandomIds.at(0));
    else
        ctx.bindValue("fandom1",-1);
    if(section->fandomIds.size() > 1)
        ctx.bindValue("fandom2",section->fandomIds.at(1));
    else
        ctx.bindValue("fandom2",-1);

    return ctx();
}
DiagnosticSQLResult<bool>  UpdateInDB(QSharedPointer<core::Fanfic> section, sql::Database db)
{
    std::string query = "UPDATE FANFICS set fandom = :fandom, wordcount= :wordcount, CHAPTERS = :CHAPTERS,  "
                    "COMPLETE = :COMPLETE, FAVOURITES = :FAVOURITES, REVIEWS= :REVIEWS, CHARACTERS = :CHARACTERS, RATED = :RATED, "
                    "summary = :summary, genres= :genres, published = :published, updated = :updated, author_id = :author_id,"
                    "wcr= :wcr,  author= :author, title= :title, reviewstofavourites = :reviewstofavourites, "
                    "age = :age, daysrunning = :daysrunning, lastupdate = date('now'),"
                    " fandom1 = :fandom1, fandom2 = :fandom2 "
                    " where {0}_id = :site_id";
    query=fmt::format(query,section->webSite.toStdString());
    SqlContext<bool> ctx(db, std::move(query));
    ctx.bindValue("fandom",section->fandom);
    ctx.bindValue("author",section->author->name);
    ctx.bindValue("author_id",section->author->GetWebID("ffn"));
    ctx.bindValue("title",section->title);
    ctx.bindValue("wordcount",section->wordCount.toInt());
    ctx.bindValue("CHAPTERS",section->chapters.trimmed().toInt());
    ctx.bindValue("FAVOURITES",section->favourites.toInt());
    ctx.bindValue("REVIEWS",section->reviews.toInt());
    ctx.bindValue("CHARACTERS",section->charactersFull);
    ctx.bindValue("RATED",section->rated);
    ctx.bindValue("summary",section->summary);
    ctx.bindValue("COMPLETE",section->complete);
    ctx.bindValue("genres",section->genreString);
    ctx.bindValue("published",section->published);
    ctx.bindValue("updated",section->updated);
    ctx.bindValue("site_id",section->identity.web.GetPrimaryId());
    ctx.bindValue("wcr",section->statistics.wcr);
    ctx.bindValue("reviewstofavourites",section->statistics.reviewsTofavourites);
    ctx.bindValue("age",section->statistics.age);
    ctx.bindValue("daysrunning",section->statistics.daysRunning);
    if(section->fandomIds.size() > 0)
        ctx.bindValue("fandom1",section->fandomIds.at(0));
    else
        ctx.bindValue("fandom1",-1);
    if(section->fandomIds.size() > 1)
        ctx.bindValue("fandom2",section->fandomIds.at(1));
    else
        ctx.bindValue("fandom2",-1);
    //ctx.bindValue("author_id",section->author->GetWebID("ffn"));
    ctx.ExecAndCheck();
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteRecommendation(core::AuthorPtr author, int fic_id, sql::Database db)
{
    // atm this pairs favourite story with an author
    std::string qs = " insert into recommendations (recommender_id, fic_id) values(:recommender_id,:fic_id); ";
    SqlContext<bool> ctx(db, std::move(qs), {{"recommender_id", author->id},{"fic_id", fic_id}});
    if(!author || author->id < 0)
        return std::move(ctx.result);
    ctx.SetExpectedErrors({sql::ESqlErrors::se_unique_row_violation});
    return ctx(true);
}

DiagnosticSQLResult<bool> WriteFicRelations(QList<core::FicWeightResult> ficRelations, sql::Database db)
{
    std::string qs = " insert into FicRelations (fic1, fic2, fic1_list_count, fic2_list_count, meeting_list_count, same_fandom, attraction, repullsion, final_attraction)"
                 " values(:fic1, :fic2, :fic1_list_count, :fic2_list_count, :meeting_list_count, :same_fandom, :attraction, :repullsion, :final_attraction); ";
    SqlContext<bool> ctx(db);
    ctx.Prepare(std::move(qs));
    DiagnosticSQLResult<bool> result;
    for(auto token : ficRelations)
    {
        ctx.bindValue("fic1",token.ficId1);
        ctx.bindValue("fic2",token.ficId2);
        ctx.bindValue("fic1_list_count",token.ficListCount1);
        ctx.bindValue("fic2_list_count",token.ficListCount2);
        ctx.bindValue("meeting_list_count",token.meetingCount);
        ctx.bindValue("same_fandom",token.sameFandom);
        ctx.bindValue("attraction",token.attraction);
        ctx.bindValue("repullsion",token.repulsion);
        ctx.bindValue("final_attraction",token.finalAttraction);
        ctx();
        if(!ctx.result.success)
        {
            result.success = false;
            result.sqlError = ctx.result.sqlError;
            break;
        }
    }
    return result;
}

DiagnosticSQLResult<bool> WriteAuthorsForFics(QHash<uint32_t, uint32_t> data,  sql::Database db)
{
    std::string qs = " insert into FicAuthors (fic_id, author_id)"
                 " values(:fic_id, :author_id); ";
    SqlContext<bool> ctx(db);
    ctx.Prepare(std::move(qs));
    DiagnosticSQLResult<bool> result;
    for(auto i = data.cbegin(); i != data.cend(); i++)
    {
        ctx.bindValue("fic_id",i.key());
        ctx.bindValue("author_id",i.value());
        ctx();
        if(!ctx.result.success)
        {
            result.success = false;
            result.sqlError = ctx.result.sqlError;
            break;
        }
    }
    return result;
}


DiagnosticSQLResult<int>  GetAuthorIdFromUrl(QString url, sql::Database db)
{
    std::string qsl = " select id from recommenders where url like '%{0}%' ";
    qsl = fmt::format(qsl, url.toStdString());

    SqlContext<int> ctx(db, std::move(qsl));
    ctx.FetchSingleValue<int>("id", -1);
    return std::move(ctx.result);
}
DiagnosticSQLResult<int>  GetAuthorIdFromWebID(int id, QString website, sql::Database db)
{
    std::string qs = "select id from recommenders where {0}_id = :id";
    qs=fmt::format(qs,website.toStdString());

    SqlContext<int> ctx(db, std::move(qs), {{"id", id}});
    ctx.FetchSingleValue<int>("id", -1, false);
    return std::move(ctx.result);
}


DiagnosticSQLResult<QSet<int> > GetAuthorsForFics(QSet<int> fics, sql::Database db)
{
    auto* userThreadData = ThreadData::GetUserData();
    userThreadData->ficsForAuthorSearch = fics;
    std::string qs = "select distinct author_id from fanfics where cfInFicsForAuthors(id) > 0";
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>(std::move(qs), [](sql::Query& q){
        return q.value("author_id").toInt();
    });
    ctx.result.data.remove(0);
    ctx.result.data.remove(-1);
    return std::move(ctx.result);

}
DiagnosticSQLResult<QSet<int>> GetRecommendersForFics(QSet<int> fics, sql::Database db)
{
    std::string qs = "select distinct recommender_id from  recommendations where fic_id in ({0})";
    QStringList list;
    list.reserve(fics.size());
    for(auto fic: fics)
        list.push_back(QString::number(fic));
    qs = fmt::format(qs, "'" + list.join("','").toStdString() + "'");
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>(std::move(qs), [](sql::Query& q){
        return q.value("recommender_id").toInt();
    });
    ctx.result.data.remove(0);
    ctx.result.data.remove(-1);
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<uint32_t, int>> GetHashAuthorsForFics(QSet<int> fics, sql::Database db)
{
    auto* userThreadData = ThreadData::GetUserData();
    userThreadData->ficsForAuthorSearch = fics;
    std::string qs = "select author_id, id from fanfics where cfInFicsForAuthors(id) > 0";
    SqlContext<QHash<uint32_t, int>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data[q.value("id").toUInt()] = q.value("author_id").toInt();
    });

    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> AssignNewNameToAuthorWithId(core::AuthorPtr author, sql::Database db)
{
    std::string qs = " UPDATE recommenders SET name = :name where id = :id";
    SqlContext<bool> ctx(db, std::move(qs), {{"name",author->name}, {"id",author->id}});
    if(author->GetIdStatus() != core::AuthorIdStatus::valid)
        return std::move(ctx.result);
    return ctx();
}

void ProcessIdsFromQuery(core::AuthorPtr author, const sql::Query& q)
{
    auto ffnId = q.value("ffn_id").toInt();
    auto ao3Id = q.value("ao3_id").toInt();
    auto sbId = q.value("sb_id").toInt();
    auto svId = q.value("sv_id").toInt();
    if(ffnId != -1)
        author->SetWebID("ffn", ffnId);
    if(ao3Id != -1)
        author->SetWebID("ao3", ao3Id);
    if(sbId != -1)
        author->SetWebID("sb", sbId);
    if(svId != -1)
        author->SetWebID("sv", svId);
}

core::AuthorPtr AuthorFromQuery(sql::Query& q)
{
    core::AuthorPtr result(new core::Author);
    result->AssignId(q.value("id").toInt());
    result->name = QString::fromStdString(q.value("name").toString());
    result->recCount = q.value("rec_count").toInt();
    result->stats.favouritesLastUpdated = q.value("last_favourites_update").toDateTime().date();
    result->stats.favouritesLastChecked = q.value("last_favourites_checked").toDateTime().date();
    ProcessIdsFromQuery(result, q);
    return result;
}


DiagnosticSQLResult<QList<core::AuthorPtr>> GetAllAuthors(QString website,  sql::Database db, int limit)
{
    std::string qs = "select distinct id,name, url, ffn_id, ao3_id,sb_id, sv_id,  "
                         "(select count(fic_id) from recommendations where recommender_id = recommenders.id) as rec_count,"
                         " last_favourites_update, last_favourites_checked "
                         " from recommenders where website_type = :site order by id {0}";
    if(limit > 0)
        qs = fmt::format(qs, QString(" LIMIT {0} ").arg(QString::number(limit)).toStdString());
    else
        qs = fmt::format(qs, "");

    //!!! bindvalue incorrect for first query?
    SqlContext<QList<core::AuthorPtr>> ctx(db, {{"site",website}});
    ctx.FetchLargeSelectIntoList<core::AuthorPtr>(std::move(qs), [](sql::Query& q){
        return AuthorFromQuery(q);
    }, "select count(*) from recommenders where website_type = :site");
    return std::move(ctx.result);
}


DiagnosticSQLResult<QList<core::AuthorPtr>> GetAllAuthorsWithFavUpdateSince(QString website,
                                                                            QDateTime date,
                                                                            sql::Database db,
                                                                            int limit)
{
    //todo fix reccount, needs to be precalculated in recommenders table

    std::string qs = "select distinct id,name, url, "
                         "ffn_id, ao3_id,sb_id, sv_id, "
                         " last_favourites_update, last_favourites_checked, "
                         "(select count(fic_id) from recommendations where recommender_id = recommenders.id) as rec_count "
                         " from recommenders where website_type = :site "
                         " and last_favourites_update > :date "
                         "order by id {0}";
    if(limit > 0)
        qs = fmt::format(qs,QString(" LIMIT {0} ").arg(QString::number(limit)).toStdString());
    else
        qs = fmt::format(qs, "");


    SqlContext<QList<core::AuthorPtr>> ctx(db);
    ctx.bindValue("site",website);
    ctx.bindValue("date",date);
    ctx.FetchLargeSelectIntoList<core::AuthorPtr>(std::move(qs),
                                                  [](sql::Query& q){
        return AuthorFromQuery(q);
    },"select count(*) from recommenders where website_type = :site");


    return std::move(ctx.result);
}

DiagnosticSQLResult<QList<core::AuthorPtr>> GetAllAuthorsWithFavUpdateBetween(QString website,
                                                                             QDateTime dateStart,
                                                                             QDateTime dateEnd,
                                                                             sql::Database db,
                                                                             int limit)
{
    //todo fix reccount, needs to be precalculated in recommenders table

    std::string qs = "select distinct id,name, url, "
                         "ffn_id, ao3_id,sb_id, sv_id, "
                         " last_favourites_update, last_favourites_checked, "
                         "(select count(fic_id) from recommendations where recommender_id = recommenders.id) as rec_count "
                         " from recommenders where "
                         " last_favourites_update <= :date_start "
                         " and  last_favourites_update >= :date_end "
                         " and website_type = :site "
                         "order by id {0}";
    if(limit > 0)
        qs = fmt::format(qs,QString(" LIMIT {0} ").arg(QString::number(limit)).toStdString());
    else
        qs = fmt::format(qs, "");

    qDebug() << "fething authors between " << dateEnd << " and " << dateStart;
    SqlContext<QList<core::AuthorPtr>> ctx(db);
    ctx.bindValue("date_start",dateStart);
    ctx.bindValue("date_end",dateEnd);
    ctx.bindValue("site",website);
    ctx.FetchLargeSelectIntoList<core::AuthorPtr>(std::move(qs),
                                                  [](sql::Query& q){
        return AuthorFromQuery(q);
    }, "select count(*) from recommenders where website_type = :site");


    return std::move(ctx.result);
}



DiagnosticSQLResult<QList<core::AuthorPtr>> GetAuthorsForRecommendationList(int listId,  sql::Database db)
{
    std::string qs = "select id,name, url, ffn_id, ao3_id,sb_id, sv_id, "
                         "(select count(fic_id) from recommendations where recommender_id = recommenders.id) as rec_count, "
                         " last_favourites_update, last_favourites_checked "
                         " from recommenders "
                         "where id in ( select author_id from RecommendationListAuthorStats where list_id = :list_id )";

    SqlContext<QList<core::AuthorPtr>> ctx(db, std::move(qs), {{"list_id",listId}});
    ctx.ForEachInSelect([&](sql::Query& q){
        auto author = AuthorFromQuery(q);
        ctx.result.data.push_back(author);
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QString> GetAuthorsForRecommendationListClient(int list_id,  sql::Database db)
{
    std::string qs = "select sources from recommendationlists where id = :list_id";
    SqlContext<QString> ctx(db, std::move(qs), BP1(list_id));
    ctx.FetchSingleValue<QString>("sources", "");
    return std::move(ctx.result);
}


DiagnosticSQLResult<core::AuthorPtr> GetAuthorByNameAndWebsite(QString name, QString website, sql::Database db)
{
    core::AuthorPtr result;
    std::string qs = "select id,"
                         "name, url, website_type, ffn_id, ao3_id,sb_id, sv_id,"
                         " last_favourites_update, last_favourites_checked "
                         " from recommenders where {0}_id is not null and name = :name";
    qs=fmt::format(qs,website.toStdString());

    SqlContext<core::AuthorPtr> ctx(db, std::move(qs), {{"site",website},{"name",name}});
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data = AuthorFromQuery(q);
    });
    return std::move(ctx.result);
}
DiagnosticSQLResult<core::AuthorPtr> GetAuthorByIDAndWebsite(int id, QString website, sql::Database db)
{
    std::string qs = "select r.id,r.name, r.url, r.website_type, r.ffn_id, r.ao3_id,r.sb_id, r.sv_id, "
                         " (select count(fic_id) from recommendations where recommender_id = r.id) as rec_count,"
                         " last_favourites_update, last_favourites_checked "
                         "from recommenders r where {0}_id is not null and {0}_id = :id";
    qs=fmt::format(qs,website.toStdString());
    SqlContext<core::AuthorPtr> ctx(db, std::move(qs), {{"site",website},{"id",id}});
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data = AuthorFromQuery(q);
    });
    return std::move(ctx.result);
}

void AuthorStatisticsFromQuery(const sql::Query& q,  core::AuthorPtr author)
{
    core::FavListDetails& stats = author->stats.favouriteStats;
    stats.favourites = q.value("favourites").toInt();
    stats.ficWordCount = q.value("favourites_wordcount").toInt();
    stats.averageLength = q.value("average_words_per_chapter").toInt();
    stats.esrbType = static_cast<core::FavListDetails::ESRBType>(q.value("esrb_type").toInt());
    stats.prevalentMood = static_cast<core::FavListDetails::MoodType>(q.value("prevalent_mood").toInt());
    stats.mostFavouritedSize = static_cast<core::EntitySizeType>(q.value("most_favourited_size").toInt());
    stats.sectionRelativeSize= static_cast<core::EntitySizeType>(q.value("favourites_type").toInt());
    stats.averageLength = q.value("average_favourited_length").toDouble();
    stats.fandomsDiversity = q.value("favourite_fandoms_diversity").toDouble();

    stats.explorerFactor = q.value("explorer_factor").toDouble();
    stats.megaExplorerFactor= q.value("mega_explorer_factor").toDouble();

    stats.crossoverFactor  = q.value("crossover_factor").toDouble();
    stats.unfinishedFactor = q.value("unfinished_factor").toDouble();

    stats.esrbUniformityFactor = q.value("esrb_uniformity_factor").toDouble();
    stats.esrbKiddy= q.value("esrb_kiddy").toDouble();
    stats.esrbMature= q.value("esrb_mature").toDouble();

    stats.genreDiversityFactor = q.value("genre_diversity_factor").toDouble();
    stats.moodUniformity = q.value("mood_uniformity_factor").toDouble();

    stats.moodSad= q.value("mood_sad").toDouble();
    stats.moodNeutral= q.value("mood_neutral").toDouble();
    stats.moodHappy= q.value("mood_happy").toDouble();

    stats.crackRatio= q.value("crack_factor").toDouble();
    stats.smutRatio= q.value("smut_factor").toDouble();
    stats.slashRatio= q.value("slash_factor").toDouble();
    stats.notSlashRatio= q.value("not_slash_factor").toDouble();
    stats.prevalentGenre = QString::fromStdString(q.value("prevalent_genre").toString());


    stats.sizeFactors[0] = q.value("size_tiny").toDouble();
    stats.sizeFactors[1] =  q.value("size_medium").toDouble();
    stats.sizeFactors[2] =  q.value("size_large").toDouble();
    stats.sizeFactors[3] = q.value("size_huge").toDouble();
    stats.firstPublished = q.value("first_published").toDate();
    stats.lastPublished= q.value("last_published").toDate();
}

DiagnosticSQLResult<bool> LoadAuthorStatistics(core::AuthorPtr author, sql::Database db)
{
    std::string qs = "select * from AuthorFavouritesStatistics  where author_id = :id";

    SqlContext<bool> ctx(db, std::move(qs), {{"id",author->id}});
    ctx.ForEachInSelect([&](sql::Query& q){
        AuthorStatisticsFromQuery(q, author);
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<int, QSet<int>>> LoadFullFavouritesHashset(sql::Database db)
{
    std::string qs = "select * from recommendations order by recommender_id";

    SqlContext<QHash<int, QSet<int>>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data[q.value("recommender_id").toInt()].insert(q.value("fic_id").toInt());
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<core::AuthorPtr> GetAuthorByUrl(QString url, sql::Database db)
{
    std::string qs = "select r.id,name, r.url, r.ffn_id, r.ao3_id, r.sb_id, r.sv_id, "
                         " (select count(fic_id) from recommendations where recommender_id = r.id) as rec_count,"
                         " last_favourites_update, last_favourites_checked "
                         " from recommenders r where url = :url";

    SqlContext<core::AuthorPtr> ctx(db, std::move(qs), {{"url",url}});
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data = AuthorFromQuery(q);
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<core::AuthorPtr> GetAuthorById(int id, sql::Database db)
{
    std::string qs = "select id,name, url, ffn_id, ao3_id,sb_id, sv_id, "
                         "(select count(fic_id) from recommendations where recommender_id = :id) as rec_count, "
                         " last_favourites_update, last_favourites_checked "
                         "from recommenders where id = :id";

    SqlContext<core::AuthorPtr> ctx(db, std::move(qs), {{"id",id}});
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data = AuthorFromQuery(q);
    });
    return std::move(ctx.result);
}
DiagnosticSQLResult<bool> AssignAuthorNamesForWebIDsInFanficTable(sql::Database db){

    std::string qs = " UPDATE fanfics SET author = (select name from recommenders rs where rs.ffn_id = fanfics.author_id) where exists (select name from recommenders rs where rs.ffn_id = fanfics.author_id)";
    SqlContext<bool> ctx(db, std::move(qs));
    return ctx();
}
DiagnosticSQLResult<QList<QSharedPointer<core::RecommendationList>>> GetAvailableRecommendationLists(sql::Database db)
{
    std::string qs = "select * from RecommendationLists order by name";
    SqlContext<QList<QSharedPointer<core::RecommendationList>>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        QSharedPointer<core::RecommendationList>  list(new core::RecommendationList);
        list->alwaysPickAt = q.value("always_pick_at").toInt();
        list->minimumMatch = q.value("minimum").toInt();
        list->maxUnmatchedPerMatch= q.value("pick_ratio").toInt();
        list->id= q.value("id").toInt();
        list->name= QString::fromStdString(q.value("name").toString());
        list->ficCount= q.value("fic_count").toInt();
        ctx.result.data.push_back(list);
    });
    return std::move(ctx.result);

}
// LIMIT
DiagnosticSQLResult<QSharedPointer<core::RecommendationList>> GetRecommendationList(int listId, sql::Database db)
{
    std::string qs = "select * from RecommendationLists where id = :list_id";
    SqlContext<QSharedPointer<core::RecommendationList>> ctx(db, std::move(qs), {{"list_id", listId}});
    ctx.ForEachInSelect([&](sql::Query& q){
        QSharedPointer<core::RecommendationList>  list(new core::RecommendationList);
        list->alwaysPickAt = q.value("always_pick_at").toInt();
        list->minimumMatch = q.value("minimum").toInt();
        list->maxUnmatchedPerMatch= q.value("pick_ratio").toInt();
        list->id= q.value("id").toInt();
        list->name= QString::fromStdString(q.value("name").toString());
        list->ficCount= q.value("fic_count").toInt();
        ctx.result.data = list;
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QSharedPointer<core::RecommendationList>> GetRecommendationList(QString name, sql::Database db)
{
    std::string qs = "select * from RecommendationLists where name = :list_name";

    SqlContext<QSharedPointer<core::RecommendationList>> ctx(db, std::move(qs), {{"list_name", name}});
    ctx.ForEachInSelect([&](sql::Query& q){
        QSharedPointer<core::RecommendationList>  list(new core::RecommendationList);
        list->alwaysPickAt = q.value("always_pick_at").toInt();
        list->minimumMatch = q.value("minimum").toInt();
        list->maxUnmatchedPerMatch= q.value("pick_ratio").toInt();
        list->id= q.value("id").toInt();
        list->name= QString::fromStdString(q.value("name").toString());
        list->ficCount= q.value("fic_count").toInt();
        ctx.result.data = list;
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QList<core::AuhtorStatsPtr>> GetRecommenderStatsForList(int listId, QString sortOn, QString order, sql::Database db)
{
    std::string qs = "select rts.match_count as match_count,"
                         "rts.match_ratio as match_ratio,"
                         "rts.author_id as author_id,"
                         "rts.fic_count as fic_count,"
                         "r.name as name"
                         "  from RecommendationListAuthorStats rts, recommenders r "
                         " where rts.author_id = r.id and list_id = :list_id order by {0} {1}";
    qs=fmt::format(qs,sortOn.toStdString(),order.toStdString());
    SqlContext<QList<core::AuhtorStatsPtr>> ctx(db, std::move(qs), {{"list_id", listId}});
    ctx.ForEachInSelect([&](sql::Query& q){
        core::AuhtorStatsPtr stats(new core::AuthorRecommendationStats);
        stats->isValid = true;
        stats->matchesWithReference = q.value("match_count").toInt();
        stats->matchRatio= q.value("match_ratio").toDouble();
        stats->authorId= q.value("author_id").toInt();
        stats->listId= listId;
        stats->totalRecommendations= q.value("fic_count").toInt();
        stats->authorName = QString::fromStdString(q.value("name").toString());
        ctx.result.data.push_back(stats);
    });
    return std::move(ctx.result);

}

DiagnosticSQLResult<int> GetMatchCountForRecommenderOnList(int authorId, int list, sql::Database db)
{
    std::string qs = "select fic_count from RecommendationListAuthorStats where list_id = :list_id and author_id = :author_id";
    SqlContext<int> ctx(db, std::move(qs), {{"list_id", list}, {"author_id",authorId}});
    ctx.FetchSingleValue<int>("fic_count", -1);
    return std::move(ctx.result);
}

DiagnosticSQLResult<QVector<int>> GetAllFicIDsFromRecommendationList(int listId,  core::StoryFilter::ESourceListLimiter limiter, sql::Database db)
{
    std::string qs = "select fic_id from RecommendationListData where list_id = :list_id";

    if(limiter == core::StoryFilter::sll_above_average)
        qs+=" and (votes_uncommon > 0 or votes_rare > 0 or votes_unique> 0)";
    else if(limiter == core::StoryFilter::sll_very_close)
        qs+=" and (votes_rare > 0 or votes_unique > 0)";
    if(limiter == core::StoryFilter::sll_exceptional)
        qs+=" and (votes_unique > 0)";
    SqlContext<QVector<int>> ctx(db);
    ctx.bindValue("list_id",listId);
    ctx.FetchLargeSelectIntoList<int>("fic_id", std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<QVector<int>> GetAllSourceFicIDsFromRecommendationList(int listId,  sql::Database db)
{
    std::string qs = "select fic_id from RecommendationListData where list_id = :list_id and is_origin = 1";
    SqlContext<QVector<int>> ctx(db);
    ctx.bindValue("list_id",listId);
    ctx.FetchLargeSelectIntoList<int>("fic_id", std::move(qs));
    return std::move(ctx.result);
}




DiagnosticSQLResult<core::RecommendationListFicSearchToken> GetRelevanceScoresInFilteredReclist(const core::ReclistFilter& filter, sql::Database db)
{
    std::string qs = "select fic_id, "
                     " (votes_common + votes_uncommon + votes_rare + votes_unique) as pure_votes, "
                     "{0} from RecommendationListData where list_id = :list_id";
    std::string pointsField = filter.scoreType == core::StoryFilter::st_points ? "match_count" : "no_trash_score";
    //std::string pureMatchesField = filter.sortMode == core::StoryFilter::st_points ? "match_count" : "no_trash_score";
    qs = fmt::format(qs,pointsField);


    if(filter.minMatchCount != 0)
    {
        qs += fmt::format(" and {0} > :match_count",pointsField);
    }

    if(filter.limiter == core::StoryFilter::sll_above_average)
        qs+=" and (votes_uncommon > 0 or votes_rare > 0 or votes_unique> 0)";
    else if(filter.limiter == core::StoryFilter::sll_very_close)
        qs+=" and (votes_rare > 0 or votes_unique > 0)";
    if(filter.limiter == core::StoryFilter::sll_exceptional)
        qs+=" and (votes_unique > 0)";

    if(!filter.displayPurged)
        qs+=" and purged = 0";
    else
        qs+=" and purged = 1";

    //qDebug() << "purged query:" << qs;

    SqlContext<core::RecommendationListFicSearchToken> ctx(db, std::move(qs));
    ctx.bindValue("list_id",filter.mainListId);
    if(filter.minMatchCount != 0)
        ctx.bindValue("match_count",filter.minMatchCount);
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data.ficToScore[q.value("fic_id").toInt()] = q.value(pointsField).toInt();
        ctx.result.data.ficToPureVotes[q.value("fic_id").toInt()] = q.value("pure_votes").toInt();
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QStringList> GetAllAuthorNamesForRecommendationList(int listId, sql::Database db)
{
    std::string qs = "select name from recommenders where id in (select author_id from RecommendationListAuthorStats where list_id = :list_id)";
    SqlContext<QStringList> ctx(db);
    ctx.bindValue("list_id",listId);
    ctx.FetchLargeSelectIntoList<QString>("name", std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<int>  GetCountOfTagInAuthorRecommendations(int authorId, QString tag, sql::Database db)
{
    std::string qs = "select count(distinct fic_id) as ficcount from FicTags ft where ft.tag = :tag and exists"
                         " (select 1 from Recommendations where ft.fic_id = fic_id and recommender_id = :recommender_id)";

    SqlContext<int> ctx(db, std::move(qs), {{"tag", tag}, {"recommender_id",authorId}});
    ctx.FetchSingleValue<int>("ficcount", 0);
    return std::move(ctx.result);
}

//!todo needs check if the query actually works
DiagnosticSQLResult<int> GetMatchesWithListIdInAuthorRecommendations(int authorId, int listId, sql::Database db)
{
    std::string qs = "select count(fic_id) as ficcount from Recommendations r where recommender_id = :author_id and exists "
                         " (select 1 from RecommendationListData rld where rld.list_id = :list_id and fic_id = rld.fic_id)";
    SqlContext<int> ctx(db, std::move(qs), {{"author_id", authorId}, {"list_id",listId}});
    ctx.FetchSingleValue<int>("ficcount", 0);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> DeleteRecommendationList(int listId, sql::Database db )
{
    SqlContext<bool> ctx(db);
    if(listId == 0)
        return std::move(ctx.result);
    ctx.bindValue("list_id", listId);
    ctx.ExecuteList({"delete from RecommendationLists where id = :list_id",
                     "delete from RecommendationListAuthorStats where list_id = :list_id",
                     "delete from RecommendationListData where list_id = :list_id"});

    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> DeleteRecommendationListData(int listId, sql::Database db)
{
    SqlContext<bool> ctx(db);
    if(listId == 0)
        return std::move(ctx.result);
    ctx.bindValue("list_id", listId);
    ctx.ExecuteList({"delete from RecommendationListAuthorStats where list_id = :list_id",
                     "delete from RecommendationListData where list_id = :list_id"});

    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> CopyAllAuthorRecommendationsToList(int authorId, int listId, sql::Database db )
{
    std::string qs = "insert into RecommendationListData (fic_id, list_id)"
                         " select fic_id, {0} as list_id from recommendations r where r.recommender_id = :author_id and "
                         " not exists( select 1 from RecommendationListData where list_id=:list_id and fic_id = r.fic_id) ";
    qs=fmt::format(qs,listId);
    return SqlContext<bool>(db, std::move(qs), {{"author_id",authorId},{"list_id",listId}})();
}
DiagnosticSQLResult<bool> WriteAuthorRecommendationStatsForList(int listId, core::AuhtorStatsPtr stats, sql::Database db)
{
    std::string qs = "insert into RecommendationListAuthorStats (author_id, fic_count, match_count, match_ratio, list_id) "
                         "values(:author_id, :fic_count, :match_count, :match_ratio, :list_id)";

    SqlContext<bool> ctx(db, std::move(qs));
    if(!stats)
        return std::move(ctx.result);
    ctx.bindValue("author_id",stats->authorId);
    ctx.bindValue("fic_count",stats->totalRecommendations);
    ctx.bindValue("match_count",stats->matchesWithReference);
    ctx.bindValue("match_ratio",stats->matchRatio);
    ctx.bindValue("list_id",listId);
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}
DiagnosticSQLResult<bool> CreateOrUpdateRecommendationList(QSharedPointer<core::RecommendationList> list, QDateTime creationTimestamp, sql::Database db)
{
    std::string qs;
    SqlContext<bool> ctx(db);
    //int freeId = -2;
    {
        qs = "select max(id) as id from RecommendationLists";
        ctx.ReplaceQuery(std::move(qs));
        if(!ctx.ExecAndCheckForData())
        {
            return std::move(ctx.result);
        }
        //freeId = ctx.value("id").toInt() + 1;
        qDebug() << "At this moment max id is: " << ctx.value("id").toInt();
    }
    qDebug() << "List's name is: " << list->name;
    qs = fmt::format("insert into RecommendationLists(name) select '{0}' "
                         " where not exists(select 1 from RecommendationLists where name = '{0}')", list->name.toStdString());
    ctx.ReplaceQuery(std::move(qs));
    if(!ctx.ExecAndCheck())
    {
        list->id = -1;
        return std::move(ctx.result);
    }
    qs = "select id from RecommendationLists where name = :name";
    SqlContext<bool> ctx2(db, std::move(qs));
    ctx2.bindValue("name",list->name);
    if(!ctx2.ExecAndCheckForData())
    {
        list->id = -1;
        return ctx2.result;
    }
    list->id = ctx2.value("id").toInt();
    qDebug() << "Created new list with id: " << list->id;
    ctx.result.data = list->id > 0;

    qs = "update RecommendationLists set minimum = :minimum, pick_ratio = :pick_ratio, is_automatic = :is_automatic, "
                 " always_pick_at = :always_pick_at,  created = :created,"
                 "  quadratic_deviation = :quadratic_deviation, ratio_median = :ratio_median, "
                 "  distance_to_double_sigma = :distance_to_double_sigma,has_aux_data = :has_aux_data,"
                 "  use_weighting = :use_weighting, use_mood_adjustment = :use_mood_adjustment,"
                 "  use_dislikes = :use_dislikes, use_dead_fic_ignore = :use_dead_fic_ignore,"
                 " sources = :sources where name = :name";
    ctx.ReplaceQuery(std::move(qs));
    ctx.bindValue("minimum",list->minimumMatch);
    ctx.bindValue("pick_ratio",list->maxUnmatchedPerMatch);
    ctx.bindValue("is_automatic",list->isAutomatic);
    ctx.bindValue("always_pick_at",list->alwaysPickAt);
    ctx.bindValue("created",creationTimestamp);
    ctx.bindValue("quadratic_deviation",list->quadraticDeviation);
    ctx.bindValue("ratio_median",list->ratioMedian);
    ctx.bindValue("distance_to_double_sigma",list->sigma2Distance);
    ctx.bindValue("has_aux_data",list->hasAuxDataFilled);
    ctx.bindValue("use_weighting",list->useWeighting);
    ctx.bindValue("use_mood_adjustment",list->useMoodAdjustment);
    ctx.bindValue("use_dislikes",list->useDislikes);
    ctx.bindValue("use_dead_fic_ignore",list->useDeadFicIgnore);
    QStringList authors;
    authors.reserve(list->ficData->authorIds.size());
    for(auto id : std::as_const(list->ficData->authorIds))
        authors.push_back(QString::number(id));
    ctx.bindValue("sources",authors.join(","));
    ctx.bindValue("name",list->name);
    if(!ctx.ExecAndCheck())
    {
        list->id = -1;
        return std::move(ctx.result);
    }


    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteAuxParamsForReclist(QSharedPointer<core::RecommendationList> list, sql::Database db)
{
    std::string qs = "update RecommendationLists set  "
                 "  quadratic_deviation = :quadratic_deviation, ratio_median = :ratio_median, "
                 "  distance_to_double_sigma = :distance_to_double_sigma,has_aux_data = :has_aux_data"
                 " where name = :name";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("quadratic_deviation",list->quadraticDeviation);
    ctx.bindValue("ratio_median",list->ratioMedian);
    ctx.bindValue("distance_to_double_sigma",list->sigma2Distance);
    ctx.bindValue("has_aux_data",list->hasAuxDataFilled);
    ctx.bindValue("name",list->name);
    return ctx();
}



DiagnosticSQLResult<bool> UpdateFicCountForRecommendationList(int listId, sql::Database db)
{
    std::string qs = "update RecommendationLists set fic_count=(select count(fic_id) "
                         " from RecommendationListData where list_id = :list_id) where id = :list_id";
    SqlContext<bool> ctx(db, std::move(qs),{{"list_id",listId}});
    if(listId == -1 || !ctx.ExecAndCheck())
        return std::move(ctx.result);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> DeleteTagFromDatabase(QString tag, sql::Database db)
{
    SqlContext<bool> ctx(db);
    ctx.bindValue("tag", tag);
    ctx.ExecuteList({"delete from FicTags where tag = :tag",
                     "delete from tags where tag = :tag"});
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool>  CreateTagInDatabase(QString tag, sql::Database db)
{
    std::string qs = "INSERT INTO TAGS(TAG) VALUES(:tag)";
    return SqlContext<bool>(db, std::move(qs),{{"tag",tag}})();
}

DiagnosticSQLResult<int>  GetRecommendationListIdForName(QString name, sql::Database db)
{
    std::string qs = "select id from RecommendationLists where name = :name";
    SqlContext<int> ctx(db, std::move(qs), {{"name", name}});
    ctx.FetchSingleValue<int>("id", 0);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool>  AddAuthorFavouritesToList(int authorId, int listId, sql::Database db)
{
    std::string qs = " update RecommendationListData set match_count = match_count+1 where "
                         " list_id = :list_id "
                         " and fic_id in (select fic_id from recommendations r where recommender_id = :author_id)";
    return  SqlContext<bool>(db, std::move(qs),{{"author_id",authorId},{"list_id",listId}})();
}

DiagnosticSQLResult<bool>  ShortenFFNurlsForAllFics(sql::Database db)
{
    std::string qs = "update fanfics set url = cfReturnCapture('(/s/\\d+/)', url)";
    return  SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> IsGenreList(QStringList list, QString website, sql::Database db)
{
    std::string qs = "select count(*) as idcount from genres where genre in({0}) and website = :website";
    qs = fmt::format(qs, "'" + list.join("','").toStdString() + "'");

    SqlContext<int> ctx(db, std::move(qs), {{"website", website}});
    ctx.FetchSingleValue<int>("idcount", 0);

    DiagnosticSQLResult<bool> realResult;
    realResult.success = ctx.result.success;
    realResult.sqlError = ctx.result.sqlError;
    realResult.data = ctx.result.data > 0;
    return realResult;

}

DiagnosticSQLResult<QVector<int>> GetWebIdList(QString where, QString website, sql::Database db)
{
    //QVector<int> result;
    std::string fieldName = website.toStdString() + "_id";
    std::string qs = fmt::format("select {0}_id from fanfics {1}",website.toStdString(),where.toStdString());
    SqlContext<QVector<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>(std::move(fieldName), std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<QVector<int>> GetIdList(QString where, sql::Database db)
{
    std::string qs = fmt::format("select id from fanfics {0} order by id asc",where.toStdString());
    SqlContext<QVector<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("id", std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> DeactivateStory(int id, QString website, sql::Database db)
{
    std::string qs = "update fanfics set alive = 0 where {0}_id = :id";
    qs=fmt::format(qs,website.toStdString());
    return SqlContext<bool>(db, std::move(qs), {{"id",id}})();
}

DiagnosticSQLResult<bool> CreateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, sql::Database db)
{

    std::string qs = " insert into recommenders(name, url, favourites, fics, page_updated, ffn_id, ao3_id,sb_id, sv_id, "
                 "page_creation_date, info_updated, last_favourites_update,last_favourites_checked) "
                 "values(:name, :url, :favourites, :fics,  :time, :ffn_id, :ao3_id,:sb_id, :sv_id, "
                 ":page_creation_date, :info_updated,:last_favourites_update,:last_favourites_checked) ";
    SqlContext<bool> ctx(db, std::move(qs));
    if(author->name.isEmpty())
        author->name = QUuid::createUuid().toString();
    ctx.bindValue("name", author->name);
    ctx.bindValue("url", author->url("ffn"));
    ctx.bindValue("favourites", author->favCount);
    ctx.bindValue("fics", author->ficCount);
    ctx.bindValue("time", timestamp);
    ctx.bindValue("ffn_id", author->GetWebID("ffn"));
    ctx.bindValue("ao3_id", author->GetWebID("ao3"));
    ctx.bindValue("sb_id", author->GetWebID("sb"));
    ctx.bindValue("sv_id", author->GetWebID("sv"));
    ctx.bindValue("page_creation_date", author->stats.pageCreated);
    ctx.bindValue("info_updated", author->stats.bioLastUpdated);
    ctx.bindValue("last_favourites_update", author->stats.favouritesLastUpdated);
    ctx.bindValue("last_favourites_checked", author->stats.favouritesLastChecked);
    ctx.ExecAndCheck();
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool>  UpdateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, sql::Database db)
{

    std::string qs = " update recommenders set name = :name, url = :url, favourites = :favourites, fics = :fics, page_updated = :page_updated, "
                 "page_creation_date= :page_creation_date, info_updated= :info_updated, "
                 " ffn_id = :ffn_id, ao3_id = :ao3_id, sb_id  =:sb_id, sv_id = :sv_id,"
                 " last_favourites_update = :last_favourites_update, "
                 " last_favourites_checked = :last_favourites_checked "
                 " where id = :id ";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("name", author->name);
    ctx.bindValue("url", author->url("ffn"));
    ctx.bindValue("favourites", author->stats.favouriteStats.favourites);
    ctx.bindValue("fics", author->ficCount);
    ctx.bindValue("page_updated", timestamp);
    ctx.bindValue("page_creation_date", author->stats.pageCreated);
    ctx.bindValue("info_updated", author->stats.bioLastUpdated);
    ctx.bindValue("ffn_id", author->GetWebID("ffn"));
    ctx.bindValue("ao3_id", author->GetWebID("ao3"));
    ctx.bindValue("sb_id", author->GetWebID("sb"));
    ctx.bindValue("sv_id", author->GetWebID("sv"));
    ctx.bindValue("last_favourites_update", author->stats.favouritesLastUpdated);
    ctx.bindValue("last_favourites_checked", author->stats.favouritesLastChecked);
    ctx.bindValue("id", author->id);
    ctx.ExecAndCheck();

    //not reading those yet
    //q1.bindValue("info_wordcount", author->ficCount);
    //    q1.bindValue("last_published_fic_date", author->ficCount);
    //    q1.bindValue("first_published_fic_date", author->ficCount);
    //    q1.bindValue("own_wordcount", author->ficCount);
    //    q1.bindValue("own_favourites", author->ficCount);
    //    q1.bindValue("own_finished_ratio", author->ficCount);
    //    q1.bindValue("most_written_size", author->ficCount);

    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> UpdateAuthorFavouritesUpdateDate(int authorId, QDateTime date, sql::Database db)
{
    std::string qs = " update recommenders set"
                 " last_favourites_update = :last_favourites_update, "
                 " last_favourites_checked = :last_favourites_checked "
                 "where id = :id ";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("id", authorId);
    ctx.bindValue("last_favourites_update", date);
    ctx.bindValue("last_favourites_checked", QDateTime::currentDateTime());
    ctx.ExecAndCheck();

    return std::move(ctx.result);
}


DiagnosticSQLResult<QStringList> ReadUserTags(sql::Database db)
{
    DiagnosticSQLResult<QStringList> result;
    QSet<QString> tags;
    {
    std::string qs = "Select tag from tags";
    SqlContext<QStringList> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("tag", std::move(qs));
    if(!ctx.result.success)
        return std::move(ctx.result);
    tags = QSet<QString>(ctx.result.data.cbegin(), ctx.result.data.cend());
    }
    {
    std::string qs = "select distinct tag from fictags";
    SqlContext<QStringList> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("tag", std::move(qs));
    if(!ctx.result.success)
        return std::move(ctx.result);
    tags = QSet<QString>(ctx.result.data.cbegin(), ctx.result.data.cend());
    }
    auto values = tags.values();
    std::sort(values.begin(), values.end());
    result.data = values;
    result.success = true;
    return result;

}

DiagnosticSQLResult<bool>  PushTaglistIntoDatabase(QStringList tagList, sql::Database db)
{
    int counter = 0;
    std::string qs = "INSERT INTO TAGS (TAG, id) VALUES (:tag, :id)";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.ExecuteWithKeyListAndBindFunctor<QList, QString>(tagList, [&](QString&& tag, sql::Query q){
        q.bindValue("tag", tag);
        q.bindValue("id", counter);
        counter++;
    }, true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool>  AssignNewNameForAuthor(core::AuthorPtr author, QString name, sql::Database db)
{
    std::string qs = " UPDATE recommenders SET name = :name where id = :id";
    SqlContext<bool> ctx(db, std::move(qs), {{"name", name},{"id", author->id}});
    if(author->GetIdStatus() != core::AuthorIdStatus::valid)
        return std::move(ctx.result);
    return ctx();
}

DiagnosticSQLResult<QList<int>> GetAllAuthorIds(sql::Database db)
{
    std::string qs = "select distinct id from recommenders";

    SqlContext<QList<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("id", std::move(qs));
    return std::move(ctx.result);
}

// not working, no idea why
//DiagnosticSQLResult<QSet<int> > GetAllMatchesWithRecsUID(QSharedPointer<core::RecommendationList> params, QString uid, sql::Database db)
//{
//    SqlContext<QSet<int>> ctx(db);
//    ctx.bindValue("uid", uid);
//    ctx.bindValue("ratio", params->pickRatio);
//    ctx.bindValue("always_pick_at", params->alwaysPickAt);
//    ctx.bindValue("min", params->minimumMatch);
//    QLOG_INFO() << "always: " << params->alwaysPickAt;
//    QLOG_INFO() << "ratio: " << params->pickRatio;
//    QLOG_INFO() << "min: " << params->minimumMatch;
//    ctx.FetchLargeSelectIntoList<int>("id",
//                                      "select distinct id from recommenders rs where  ( "
//                                      " (select count(recommender_id) from recommendations where cfInRecommendations(fic_id, :uid) = 1 and recommender_id = rs.id) >= :min "
//                                      " and ( "
//                                      " (cast((select count(recommender_id) from recommendations where recommender_id = rs.id)  as float) "
//                                      " / "
//                                      " (cast((select count(recommender_id) from recommendations where cfInRecommendations(fic_id, :uid) = 1 and recommender_id = rs.id) as float))) <= :ratio)"
//                                      ") "
//                                      );
//    return std::move(ctx.result);
//}

DiagnosticSQLResult<QSet<int> > GetAllMatchesWithRecsUID(QSharedPointer<core::RecommendationList> params, QString uid, sql::Database db)
{
    QLOG_INFO() << "always: " << params->alwaysPickAt;
    QLOG_INFO() << "ratio: " << params->maxUnmatchedPerMatch;
    QLOG_INFO() << "min: " << params->minimumMatch;
    QLOG_INFO() << "///////////FIRST FETCH////////";
    SqlContext<QSet<int>> ctx(db);
    ctx.bindValue("uid1", uid);
    ctx.bindValue("uid2", uid);
    ctx.bindValue("ratio", params->maxUnmatchedPerMatch);
    ctx.bindValue("min", params->minimumMatch);
    ctx.bindValue("always_pick_at", params->alwaysPickAt);
    //qDebug() << "feature avail: " << db.driver()->hasFeature(QSqlDriver::NamedPlaceholders);
    ctx.FetchLargeSelectIntoList<int>("id",
                                      "select id,  "
                                      " (select count(recommender_id) from recommendations rs where "
                                      " cfInRecommendations(rs.fic_id, :uid1) = 1 and rs.recommender_id = id ) as matches, "

                                      " ("
                                      " (cast((select count(recommender_id) from recommendations rs1 where rs1.recommender_id = id)  as float) "
                                      " / "
                                      " (cast((select count(recommender_id) from recommendations rs2 where"
                                      " cfInRecommendations(rs2.fic_id, :uid2) = 1"
                                      " and rs2.recommender_id = id) as float)))"
                                      " )  as ratio"

                                      " from recommenders  "
                                      " where (ratio <= :ratio and  matches >= :min ) or matches >= :always_pick_at "
                                      );

    return std::move(ctx.result);
}


DiagnosticSQLResult<QSet<int> > ConvertFFNSourceFicsToDB(QString uid, sql::Database db)
{
    SqlContext<QSet<int>> ctx(db);
    ctx.bindValue("uid", uid);
    ctx.FetchLargeSelectIntoList<int>("id",
                                      "select id from fanfics where cfInSourceFics(ffn_id)");

    return std::move(ctx.result);
}

static auto getFicWeightPtrFromQuery = [](auto& q){
    core::FicWeightPtr fw(new core::FanficDataForRecommendationCreation);
    fw->adult = QString::fromStdString(q.value("Rated").toString()) == "M";
    fw->authorId = q.value("author_id").toInt();
    fw->complete = q.value("complete").toBool();
    //fw->chapterCount = q.value("chapters").toInt();
    fw->dead = !fw->complete  && q.value("updated").toDateTime().daysTo(QDateTime::currentDateTime()) > 365;
    fw->fandoms.push_back(q.value("fandom1").toInt());
    fw->fandoms.push_back(q.value("fandom2").toInt());
    fw->favCount = q.value("favourites").toInt();
    fw->published = q.value("published").toDate();
    fw->updated = q.value("updated").toDate();
    fw->genreString = QString::fromStdString(q.value("genres").toString());
    fw->id = q.value("id").toInt();
    fw->reviewCount = q.value("reviews").toInt();
    fw->slash = q.value("filter_pass_1").toBool();
    fw->wordCount = q.value("wordcount").toInt();
    return fw;
};

DiagnosticSQLResult<QHash<uint32_t, core::FicWeightPtr>> GetFicsForRecCreation(sql::Database db)
{
    SqlContext<QHash<uint32_t, core::FicWeightPtr>> ctx(db);
    std::string qs = "select id,rated, chapters, author_id, complete, updated, "
                         "fandom1,fandom2,favourites, published, updated,"
                         "  genres, reviews, filter_pass_1, wordcount"
                         "  from fanfics where cfInSourceFics(ffn_id) order by id asc";
    ctx.FetchSelectFunctor(std::move(qs), DATAQ{
                               auto fw = getFicWeightPtrFromQuery(q);
                               if(fw)
                                data[static_cast<uint32_t>(fw->id)] = fw;
                           });
    return std::move(ctx.result);


}
DiagnosticSQLResult<bool> ConvertFFNTaggedFicsToDB(QHash<int, int>& hash, sql::Database db)
{
    SqlContext<int> ctx(db);
    QLOG_INFO() << "keys list:" << hash.keys();
    std::vector<uint32_t> idsToRemove;
    for(auto i = hash.begin(); i != hash.end(); i++)
    {
        ctx.bindValue("ffn_id", i.key());
        ctx.FetchSingleValue<int>("id", -1,false, "select id from fanfics where ffn_id = :ffn_id");
        auto  error = ctx.result.sqlError;
        if(error.isValid() && error.getActualErrorType() != ESqlErrors::se_no_data_to_be_fetched)
        {
            QLOG_ERROR() << "///ERROR///" << QString::fromStdString(error.text());
            ctx.result.success = false;
            break;
        }
        if(ctx.result.data != -1)
            i.value() = ctx.result.data; // todo check this iterator usage is valid
        else
            idsToRemove.push_back(i.key());
    }
    for(auto id: idsToRemove)
        hash.remove(id);

    QLOG_INFO() << "Resulting list:" << hash;

    SqlContext<bool> ctx1(db);
    ctx1.result.success = ctx.result.success;
    ctx1.result.sqlError = ctx.result.sqlError;
    return ctx1.result;
}

DiagnosticSQLResult<bool> ConvertDBFicsToFFN(QHash<int, int>& hash, sql::Database db)
{
    SqlContext<int> ctx(db);
    QHash<int, int>::iterator it = hash.begin();
    QHash<int, int>::iterator itEnd = hash.end();
    while(it != itEnd){
        ctx.bindValue("id", it.key());
        ctx.FetchSingleValue<int>("ffn_id", -1, true, "select ffn_id from fanfics where id = :id");
        auto error = ctx.result.sqlError;
        if(error.isValid() && error.getActualErrorType() != ESqlErrors::se_no_data_to_be_fetched)
        {
            ctx.result.success = false;
            break;
        }
        it.value() = ctx.result.data; // todo check this iterator usage is valid
        it++;
    }

    SqlContext<bool> ctx1(db);
    ctx1.result.success = ctx.result.success;
    ctx1.result.sqlError = ctx.result.sqlError;
    return ctx1.result;
}



DiagnosticSQLResult<bool> ResetActionQueue(sql::Database db)
{
    std::string qs = "update fanfics set queued_for_action = 0";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx();
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteDetectedGenres(QVector<genre_stats::FicGenreData> fics, sql::Database db)
{

    std::string qsClenaup = "update fanfics set "
                                "true_genre1 = '', "
                                "true_genre1_percent = 0,"
                                "true_genre2 = '', "
                                "true_genre2_percent = 0,"
                                "true_genre3 = '',"
                                "true_genre3_percent = 0,"
                                "max_genre_percent = 0, "
                                "kept_genres = ''";
    SqlContext<bool> cleanup(db, std::move(qsClenaup));
    cleanup();

    std::string qs = "update fanfics set "
                         " true_genre1 = :true_genre1, "
                         " true_genre1_percent = :true_genre1_percent,"
                         " true_genre2 = :true_genre2, "
                         " true_genre2_percent = :true_genre2_percent,"
                         " true_genre3 = :true_genre3, "
                         " true_genre3_percent = :true_genre3_percent,"
                         " max_genre_percent = :max_genre_percent,"
                         " kept_genres =:kept_genres where id = :fic_id";
    SqlContext<bool> ctx(db, std::move(qs));
    for(const auto& fic : fics)
    {
        for(int i = 0; i < 3; i++)
        {
            genre_stats::GenreBit genre;
            if(i < fic.processedGenres.size())
                genre = fic.processedGenres.at(i);
            else
                genre.relevance = 0;

            QString writtenGenre = genre.genres.join(",");
            if(writtenGenre.isEmpty())
                genre.relevance = 0;

            ctx.bindValue("true_genre" + QString::number(i+1).toStdString(), writtenGenre);
            ctx.bindValue("true_genre" + QString::number(i+1).toStdString() + "_percent", genre.relevance > 1 ? 1 : genre.relevance );


        }
        ctx.bindValue("max_genre_percent", fic.maxGenrePercent);
//        if(fic.maxGenrePercent > 0)
//            qDebug() << "Writing max_genre_percent: " << fic.maxGenrePercent;
        ctx.bindValue("kept_genres", fic.keptToken);
        ctx.bindValue("fic_id", fic.ficId);
        if(!ctx.ExecAndCheck())
            return std::move(ctx.result);
    }
    return std::move(ctx.result);
}


DiagnosticSQLResult<bool> WriteDetectedGenresIteration2(QVector<genre_stats::FicGenreData> fics, sql::Database db)
{
    std::string qsClenaup = std::string("delete from FIC_GENRE_ITERATIONS");
    SqlContext<bool> cleanup(db, std::move(qsClenaup));
    cleanup();
    std::string qs = " insert into fic_genre_iterations("
                         "fic_id, "
                         " true_genre1, "
                         " true_genre1_percent,"
                         " true_genre2, "
                         " true_genre2_percent,"
                         " true_genre3,"
                         " true_genre3_percent,"
                         " kept_genres,"
                         " max_genre_percent"
                         ")"
                         " values("
                         " :fic_id, "
                         " :true_genre1, "
                         " :true_genre1_percent,"
                         " :true_genre2, "
                         " :true_genre2_percent,"
                         " :true_genre3,"
                         " :true_genre3_percent,"
                         " :kept_genres,"
                         " :max_genre_percent"
                         ")";
    SqlContext<bool> ctx(db, std::move(qs));
    for(const auto& fic : fics)
    {
        for(int i = 0; i < 3; i++)
        {
            genre_stats::GenreBit genre;
            if(i < fic.processedGenres.size())
                genre = fic.processedGenres.at(i);
            else
                genre.relevance = 0;

            QString writtenGenre = genre.genres.join(",");
            if(writtenGenre.isEmpty())
                genre.relevance = 0;

            ctx.bindValue("true_genre" + QString::number(i+1).toStdString(), writtenGenre);
            ctx.bindValue("true_genre" + QString::number(i+1).toStdString() + "_percent", genre.relevance > 1 ? 1 : genre.relevance );


        }
        ctx.bindValue("max_genre_percent", fic.maxGenrePercent);
        ctx.bindValue("kept_genres", fic.keptToken);
        ctx.bindValue("fic_id", fic.ficId);
        if(!ctx.ExecAndCheck())
            return std::move(ctx.result);
    }
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<int, QList<genre_stats::GenreBit>>> GetFullGenreList(sql::Database db,bool useOriginalOnly)
{
    std::string qs = "select id, genres, "
                         " true_genre1, "
                         " true_genre1_percent,"
                         " true_genre2, "
                         " true_genre2_percent,"
                         " true_genre3,"
                         " true_genre3_percent"
                         " from fanfics";

    SqlContext<QHash<int, QList<genre_stats::GenreBit>>> ctx (db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        QList<genre_stats::GenreBit> dataForFic;
        auto id = q.value("id").toInt();
        QString genres = QString::fromStdString(q.value("genres").toString());
        if(QString::fromStdString(q.value("true_genre1").toString()).trimmed().isEmpty() || useOriginalOnly)
        {
            // genres not detected

            genres = genres.replace("Hurt/Comfort", "HurtComfort");
            auto list = genres.split("/");
            list.replaceInStrings("HurtComfort","Hurt/Comfort");
            dataForFic.reserve(list.size());
            for(const auto& genreBit: list)
            {
                genre_stats::GenreBit bit;
                bit.genres.push_back(genreBit);
                bit.isInTheOriginal = true;
                bit.relevance = 1;
                dataForFic.push_back(bit);
            }
        }
        else{

            // genres detected
            for(int i = 1; i < 4; i++)
            {
                auto tgKey = "true_genre" + std::to_string(i);
                auto tgKeyValue = "true_genre" + std::to_string(i) + "_percent";
                auto genre = q.value(tgKey).toString();
                if(genre.empty()){
                    break;
                }

                genre_stats::GenreBit bit;
                bit.genres = QString::fromStdString(genre).split(QRegExp("[\\s,]"), Qt::SkipEmptyParts);
                bit.relevance = q.value(tgKeyValue).toFloat();
                bit.isDetected = true;
                for(const auto& genreBit : std::as_const(bit.genres))
                    if(genres.contains(genreBit))
                        bit.isInTheOriginal = true;

                dataForFic.push_back(bit);
            }
        }
        ctx.result.data[id] = dataForFic;
    });
    return std::move(ctx.result);
}


DiagnosticSQLResult<QHash<int, int> > GetMatchesForUID(QString uid, sql::Database db)
{
    std::string qs = "select fic_id, count(fic_id) as cnt from recommendations where cfInAuthors(recommender_id, :uid) = 1 group by fic_id";
    SqlContext<QHash<int, int> > ctx(db);
    ctx.bindValue("uid", uid);
    ctx.FetchSelectFunctor(std::move(qs), [](QHash<int, int>& data, sql::Query& q){
        int fic = q.value("fic_id").toInt();
        int matches = q.value("cnt").toInt();
        //QLOG_INFO() << " fic_id: " << fic << " matches: " << matches;
        data[fic] = matches;
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QStringList> GetAllAuthorFavourites(int id, sql::Database db)
{
    std::string qs = "select id, ffn_id, ao3_id, sb_id, sv_id from fanfics where id in (select fic_id from recommendations where recommender_id = :author_id )";
    SqlContext<QStringList> ctx (db, std::move(qs), {{"author_id", id}});
    ctx.ForEachInSelect([&](sql::Query& q){
        auto ffn_id = q.value("ffn_id").toInt();
        auto ao3_id = q.value("ao3_id").toInt();
        auto sb_id = q.value("sb_id").toInt();
        auto sv_id = q.value("sv_id").toInt();

        if(ffn_id != -1)
            ctx.result.data.push_back(url_utils::GetStoryUrlFromWebId(ffn_id, "ffn"));
        if(ao3_id != -1)
            ctx.result.data.push_back(url_utils::GetStoryUrlFromWebId(ao3_id, "ao3"));
        if(sb_id != -1)
            ctx.result.data.push_back(url_utils::GetStoryUrlFromWebId(sb_id, "sb"));
        if(sv_id != -1)
            ctx.result.data.push_back(url_utils::GetStoryUrlFromWebId(sv_id, "sv"));
    });
    return std::move(ctx.result);

}

DiagnosticSQLResult<QList<int>> GetAllAuthorRecommendationIDs(int id, sql::Database db)
{
    std::string qs = "select distinct fic_id from recommendations where recommender_id = :author_id";
    SqlContext<QList<int>> ctx (db, std::move(qs), {{"author_id", id}});
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data.push_back(q.value("fic_id").toInt());
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, sql::Database db)
{
    std::string qs = " update RecommendationListData set match_count = match_count+1 where "
                         " list_id = :list_id "
                         " and fic_id in (select fic_id from recommendations r where recommender_id = :author_id)";
    return SqlContext<bool> (db, std::move(qs), {{"author_id", authorId},{"list_id", listId}})();
}


DiagnosticSQLResult<bool> DecrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, sql::Database db)
{
    std::string qs = " update RecommendationListData set match_count = match_count-1 where "
                         " list_id = :list_id "
                         " and fic_id in (select fic_id from recommendations r where recommender_id = :author_id)";

    SqlContext<bool> ctx (db, std::move(qs), {{"author_id", authorId},{"list_id", listId}});
    if(!ctx.ExecAndCheck())
        return std::move(ctx.result);

    ctx.ReplaceQuery("delete from RecommendationListData where match_count <= 0");
    ctx.ExecAndCheck();
    return  ctx.result;
}

DiagnosticSQLResult<QSet<QString>> GetAllGenres(sql::Database db)
{
    std::string qs = "select genre from genres";
    SqlContext<QSet<QString>> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("genre", std::move(qs));
    return std::move(ctx.result);
}

static core::FandomPtr FandomfromQueryNew (const sql::Query& q, core::FandomPtr fandom = core::FandomPtr())
{
    if(!fandom)
    {
        fandom = core::Fandom::NewFandom();
        fandom->id = q.value("ID").toInt();
        fandom->SetName(QString::fromStdString(q.value("name").toString()));
        fandom->tracked = q.value("tracked").toInt();
        fandom->lastUpdateDate = q.value("updated").toDate();
        fandom->AddUrl({QString::fromStdString(q.value("url").toString()),
                        QString::fromStdString(q.value("website").toString()),
                        ""});
    }
    else{
        fandom->AddUrl({QString::fromStdString(q.value("url").toString()),
                        QString::fromStdString(q.value("website").toString()),
                        ""});
    }
    return fandom;

}
static DiagnosticSQLResult<bool> GetFandomStats(core::FandomPtr fandom, sql::Database db)
{
    std::string qs = "select * from fandomsources where global_id = :id";
    SqlContext<bool> ctx(db, std::move(qs));
    if(!fandom)
        return std::move(ctx.result);
    ctx.bindValue("id",fandom->id);
    ctx.ExecAndCheck();
    if(ctx.Next())
    {
        fandom->source = QString::fromStdString(ctx.value("website").toString());
        fandom->ficCount = ctx.value("fic_count").toInt();
        fandom->averageFavesTop3 = ctx.value("average_faves_top_3").toDouble();
        fandom->dateOfCreation = ctx.value("date_of_creation").toDate();
        fandom->dateOfFirstFic = ctx.value("date_of_first_fic").toDate();
        fandom->dateOfLastFic = ctx.value("date_of_last_fic").toDate();
    }
    return std::move(ctx.result);
}

DiagnosticSQLResult<QList<core::FandomPtr>> GetAllFandoms(sql::Database db)
{
    int fandomsSize = 0;
    {
        std::string qs = " select count(*) as cn from fandomindex";
        SqlContext<int> ctx(db, std::move(qs));
        ctx.FetchSingleValue<int>("cn", 1000);
        fandomsSize = ctx.result.data;
    }

    core::FandomPtr currentFandom;
    int lastId = -1;
    std::string qs = " select ind.id as id, ind.name as name, ind.tracked as tracked, urls.url as url, urls.website as website,"
                         " urls.custom as section, ind.updated as updated "
                         " from fandomindex ind left join fandomurls urls"
                         " on ind.id = urls.global_id order by id asc";
    SqlContext<QList<core::FandomPtr>> ctx(db, std::move(qs));
    ctx.result.data.reserve(fandomsSize);
    ctx.ForEachInSelect([&](sql::Query& q){
        auto currentId= q.value("id").toInt();
        if(currentId != lastId)
        {
            currentFandom = FandomfromQueryNew(q);
            //GetFandomStats(currentFandom, db);
            ctx.result.data.push_back(currentFandom);
        }
        else
            currentFandom = FandomfromQueryNew(q, currentFandom);
        lastId = currentId;
    });
    return std::move(ctx.result);
}
DiagnosticSQLResult<QList<core::FandomPtr> > GetAllFandomsAfter(int id, sql::Database db)
{
    int fandomsSize = 0;
    {
        std::string qs = " select count(*) as cn from fandomindex where id > :id";
        SqlContext<int> ctx(db, std::move(qs), BP1(id));
        ctx.FetchSingleValue<int>("cn", 1000);
        fandomsSize = ctx.result.data;
    }

    core::FandomPtr currentFandom;
    int lastId = -1;
    std::string qs = " select ind.id as id, ind.name as name, ind.tracked as tracked, urls.url as url, urls.website as website,"
                         " urls.custom as section, ind.updated as updated "
                         " from fandomindex ind left join fandomurls urls"
                         " on ind.id = urls.global_id where ind.id > :id order by id asc ";
    SqlContext<QList<core::FandomPtr>> ctx(db, std::move(qs), BP1(id));
    ctx.result.data.reserve(fandomsSize);
    ctx.ForEachInSelect([&](sql::Query& q){
        auto currentId= q.value("id").toInt();
        if(currentId != lastId)
        {
            currentFandom = FandomfromQueryNew(q);
            //GetFandomStats(currentFandom, db);
            ctx.result.data.push_back(currentFandom);
        }
        else
            currentFandom = FandomfromQueryNew(q, currentFandom);
        lastId = currentId;
    });
    return std::move(ctx.result);
}
DiagnosticSQLResult<core::FandomPtr> GetFandom(QString fandom, bool loadFandomStats, sql::Database db)
{
    core::FandomPtr currentFandom;

    std::string qs = " select ind.id as id, ind.name as name, ind.tracked as tracked, urls.url as url, urls.website as website,"
                         " urls.custom as section, ind.updated as updated from fandomindex ind left join fandomurls urls on ind.id = urls.global_id"
                         " where lower(name) = lower(:fandom) ";
    SqlContext<core::FandomPtr> ctx(db, std::move(qs), BP1(fandom));

    ctx.ForEachInSelect([&](sql::Query& q){
        currentFandom = FandomfromQueryNew(q, currentFandom);
    });
    if(loadFandomStats)
    {
        auto statResult = GetFandomStats(currentFandom, db);
        if(!statResult.success)
        {
            ctx.result.success = false;
            return std::move(ctx.result);
        }
    }

    ctx.result.data = currentFandom;
    return std::move(ctx.result);
}

DiagnosticSQLResult<core::FandomPtr> GetFandom(int id, bool loadFandomStats, sql::Database db)
{
    core::FandomPtr currentFandom;

    std::string qs = " select ind.id as id, ind.name as name, ind.tracked as tracked, urls.url as url, urls.website as website,"
                         " urls.custom as section, ind.updated as updated from fandomindex ind left join fandomurls urls on ind.id = urls.global_id"
                         " where id = :id";
    SqlContext<core::FandomPtr> ctx(db, std::move(qs), BP1(id));

    ctx.ForEachInSelect([&](sql::Query& q){
        currentFandom = FandomfromQueryNew(q, currentFandom);
    });
    if(loadFandomStats)
    {
        auto statResult = GetFandomStats(currentFandom, db);
        if(!statResult.success)
        {
            ctx.result.success = false;
            return std::move(ctx.result);
        }
    }

    ctx.result.data = currentFandom;
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> IgnoreFandom(int fandom_id, bool including_crossovers, sql::Database db)
{
    std::string qs = " insert into ignored_fandoms (fandom_id, including_crossovers) values (:fandom_id, :including_crossovers) ";
    SqlContext<bool> ctx(db, std::move(qs),  BP2(fandom_id,including_crossovers));
    return ctx(true);
}

DiagnosticSQLResult<bool> SetUserProfile(int id,  sql::Database db)
{
    std::string qs = " update user_settings set value = :id where name = 'user_ffn_id' ";
    SqlContext<bool> ctx(db, std::move(qs),  BP1(id));
    return ctx(true);
}

DiagnosticSQLResult<int> GetUserProfile(sql::Database db)
{
    std::string qs = "select value from user_settings where name = 'user_ffn_id'";
    SqlContext<int> ctx(db, std::move(qs));
    ctx.FetchSingleValue<int>("value", -1);
    return std::move(ctx.result);
}
DiagnosticSQLResult<int> GetRecommenderIDByFFNId(int id, sql::Database db)
{
    std::string qs = "select id from recommenders where ffn_id = :id";
    SqlContext<int> ctx(db, std::move(qs), BP1(id));
    ctx.FetchSingleValue<int>("id", -1);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> RemoveFandomFromIgnoredList(int fandom_id, sql::Database db)
{
    std::string qs = " delete from ignored_fandoms where fandom_id  = :fandom_id";
    return SqlContext<bool>(db, std::move(qs), BP1(fandom_id))();
}

DiagnosticSQLResult<bool> ProcessIgnoresIntoFandomLists(sql::Database db)
{
    // first we read the ignorelist and verify it's not empty
    int numberOfIgnorelistRecords = 0;
    DiagnosticSQLResult<bool> result;
    result.success = true;
    {
        std::string qs = " select count(*) as count from ignored_fandoms";
        SqlContext<int> ctx(db, std::move(qs));
        ctx.FetchSingleValue<int>("count", 0);
        if(!ctx.result.success)
        {
            // we assume that this table no longer exists and move on
            return result;
        }
        else
            numberOfIgnorelistRecords = ctx.result.data;
    }

    // if there is nothing in ignores - we have nothing to do
    if(numberOfIgnorelistRecords == 0)
        return result;
    {
        int index = 0;
        std::string qs = " select * from ignored_fandoms";
        SqlContext<bool> ctx(db, std::move(qs));
        ctx.ForEachInSelect([&index, db](sql::Query& q){
            auto qs = "insert into fandom_list_data(list_id, fandom_id, fandom_name, enabled_state, inclusion_mode, crossover_mode, ui_index)"
                      " values(0, :fandom_id, (select name from fandomindex where id = :fandom_id_repeat), 1, 0, :crossover_mode, :ui_index)";
            SqlContext<bool> ctx(db, qs);
            ctx.bindValue("fandom_id", q.value("fandom_id").toInt());
            ctx.bindValue("fandom_id_repeat", q.value("fandom_id").toInt());
            auto includeCrossovers = q.value("fandom_id").toInt();
            ctx.bindValue("crossover_mode", includeCrossovers ? 0 : 1);
            ctx.bindValue("ui_index", index);
            index++;
            ctx();
            if(ctx.result.success)
            {
                SqlContext<bool> ctx(db, "delete from ignored_fandoms where fandom_id = :fandom_id");
                ctx.bindValue("fandom_id", q.value("fandom_id").toInt());
                ctx();
            }
        });
        result = ctx.result;
    }
    return result;
}

DiagnosticSQLResult<QStringList> GetIgnoredFandoms(sql::Database db)
{
    std::string qs = "select name from fandomindex where id in (select fandom_id from ignored_fandoms) order by name asc";
    SqlContext<QStringList> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("name", std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<int, QString>> GetFandomNamesForIDs(QList<int>ids, sql::Database db)
{
    SqlContext<QHash<int, QString> > ctx(db);
    std::string qs = "select id, name from fandomindex where id in ({0})";
    QStringList inParts;
    inParts.reserve(ids.size());
    for(auto id: ids)
        inParts.push_back("'" + QString::number(id) + "'");
    if(inParts.size() == 0)
        return std::move(ctx.result);
    qs = fmt::format(qs, inParts.join(",").toStdString());

    ctx.FetchSelectFunctor(std::move(qs), [](QHash<int, QString>& data, sql::Query& q){
        data[q.value("id").toInt()] = QString::fromStdString(q.value("name").toString());
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<int, bool> > GetIgnoredFandomIDs(sql::Database db)
{
    std::string qs = "select fandom_id, crossover_mode from fandom_list_data where list_id = 0 and inclusion_mode = 1 "
                     " and list_id in (select id from fandom_lists where is_enabled = 1) "
                     " and enabled_state = 1 "
                     " order by fandom_id asc";
    SqlContext<QHash<int, bool> > ctx(db);
    ctx.FetchSelectFunctor(std::move(qs), [](auto& data, sql::Query& q){
        data[q.value("fandom_id").toInt()] = q.value("crossover_mode").toInt() *in(0, 2); // todo check this
    }, true);
    return std::move(ctx.result);
}


DiagnosticSQLResult<bool> IgnoreFandomSlashFilter(int fandom_id, sql::Database db)
{
    std::string qs = " insert into ignored_fandoms_slash_filter (fandom_id) values (:fandom_id) ";
    SqlContext<bool> ctx(db, std::move(qs), BP1(fandom_id));
    return ctx(true);
}

DiagnosticSQLResult<bool> RemoveFandomFromIgnoredListSlashFilter(int fandom_id, sql::Database db)
{
    std::string qs = " delete from ignored_fandoms_slash_filter where fandom_id  = :fandom_id";
    return SqlContext<bool>(db, std::move(qs), BP1(fandom_id))();
}

DiagnosticSQLResult<QStringList> GetIgnoredFandomsSlashFilter(sql::Database db)
{
    std::string qs = "select name from fandomindex where id in (select fandom_id from ignored_fandoms_slash_filter) order by name asc";
    SqlContext<QStringList> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("name", std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> CleanupFandom(int fandom_id, sql::Database db)
{
    SqlContext<bool> ctx(db);
    ctx.bindValue("fandom_id", fandom_id);
    ctx.ExecuteList({"delete from fanfics where id in (select distinct fic_id from ficfandoms where fandom_id = :fandom_id)",
                     "delete from fictags where fic_id in (select distinct fic_id from ficfandoms where fandom_id = :fandom_id)",
                     "delete from recommendationlistdata where fic_id in (select distinct fic_id from ficfandoms where fandom_id = :fandom_id)",
                     "delete from ficfandoms where fandom_id = :fandom_id"});
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> DeleteFandom(int fandom_id, sql::Database db)
{
    SqlContext<bool> ctx(db);
    ctx.bindValue("fandom_id", fandom_id);
    ctx.ExecuteList({"delete from fandomindex where id = :fandom_id",
                     "delete from fandomurls where global_id = :fandom_id"});
    return std::move(ctx.result);
}

DiagnosticSQLResult<QStringList> GetTrackedFandomList(sql::Database db)
{
    std::string qs = " select name from fandomindex where tracked = 1 order by name asc";
    SqlContext<QStringList> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("name", std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<int> GetFandomCountInDatabase(sql::Database db)
{
    std::string qs = "Select count(name) as cn from fandomindex";
    SqlContext<int> ctx(db, std::move(qs));
    ctx.FetchSingleValue<int>("cn", 0);
    return std::move(ctx.result);
}


DiagnosticSQLResult<bool> AddFandomForFic(int fic_id, int fandom_id, sql::Database db)
{
    std::string qs = " insert into ficfandoms (fic_id, fandom_id) values (:fic_id, :fandom_id) ";
    SqlContext<bool> ctx(db, std::move(qs), BP2(fic_id,fandom_id));

    if(fic_id == -1 || fandom_id == -1)
        return std::move(ctx.result);

    return ctx(true);
}

DiagnosticSQLResult<QStringList>  GetFandomNamesForFicId(int fic_id, sql::Database db)
{
    std::string qs = "select name from fandomindex where fandomindex.id in (select fandom_id from ficfandoms ff where ff.fic_id = :fic_id)";
    SqlContext<QStringList> ctx(db, std::move(qs), BP1(fic_id));
    ctx.ForEachInSelect([&](sql::Query& q){
        auto fandom = trim_copy(q.value("name").toString());
        if(fandom.find("????") == std::string::npos)
            ctx.result.data.push_back(QString::fromStdString(fandom));
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> AddUrlToFandom(int fandomID, core::Url url, sql::Database db)
{
    std::string qs = " insert into fandomurls (global_id, url, website, custom) "
                         " values (:global_id, :url, :website, :custom) ";

    SqlContext<bool> ctx(db, std::move(qs),{{"global_id",fandomID},
                                 {"url",url.GetUrl()},
                                 {"website",url.GetSource()},
                                 {"custom",url.GetType()}});
    if(fandomID == -1)
        return std::move(ctx.result);
    return ctx(true);
}

DiagnosticSQLResult<std::vector<core::fandom_lists::List::ListPtr>> FetchFandomLists(sql::Database db)
{
    using ListPtr = core::fandom_lists::List::ListPtr;
    using List = core::fandom_lists::List;
    std::string qs = " select * from fandom_lists";

    SqlContext<std::vector<core::fandom_lists::List::ListPtr>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        ListPtr list(new List);
        list->id = q.value("id").toInt();
        list->name = QString::fromStdString(q.value("name").toString());
        list->isEnabled= q.value("is_enabled").toBool();
        list->isDefault = q.value("is_default").toBool();
        list->uiIndex = q.value("ui_index").toInt();
        ctx.result.data.push_back(list);
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<std::vector<core::fandom_lists::FandomStateInList>> FetchFandomStatesInUserList(int list_id, sql::Database db)
{
    using FandomState = core::fandom_lists::FandomStateInList;
    using FandomInclusionMode = core::fandom_lists::EInclusionMode;
    using CrossoverInclusionMode = core::fandom_lists::ECrossoverInclusionMode;

    std::string qs = " select * from fandom_list_data where list_id=:list_id order by fandom_id asc";

    SqlContext<std::vector<core::fandom_lists::FandomStateInList>> ctx(db, std::move(qs), BP1(list_id));
    ctx.ForEachInSelect([&](sql::Query& q){
        FandomState state;
        state.list_id = q.value("list_id").toInt();
        state.name = QString::fromStdString(q.value("fandom_name").toString());
        state.id = q.value("fandom_id").toInt();
        state.isEnabled = q.value("enabled_state").toBool();
        state.uiIndex = q.value("ui_index").toInt();
        state.crossoverInclusionMode = static_cast<CrossoverInclusionMode>(q.value("crossover_mode").toInt());
        state.inclusionMode= static_cast<FandomInclusionMode>(q.value("inclusion_mode").toInt());
        ctx.result.data.push_back(state);
    });
    return std::move(ctx.result);
}


DiagnosticSQLResult<bool> AddFandomToUserList(uint32_t list_id, uint32_t fandom_id, QString fandom_name, sql::Database db)
{
    std::string qs = "insert into fandom_list_data(list_id, fandom_id, fandom_name) values(:list_id, :fandom_id, :fandom_name)";
    SqlContext<bool> ctx(db, std::move(qs), BP3(list_id, fandom_id, fandom_name));
    return ctx(true);
}

DiagnosticSQLResult<bool> RemoveFandomFromUserList(uint32_t list_id, uint32_t fandom_id, sql::Database db)
{
    std::string qs = "delete from fandom_list_data where list_id = :list_id and fandom_id = :fandom_id";
    SqlContext<bool> ctx(db, std::move(qs), BP2(list_id, fandom_id));
    return ctx(true);
}

DiagnosticSQLResult<bool> RemoveFandomList(uint32_t list_id, sql::Database db)
{
    std::string qs = "delete from fandom_lists where id = :list_id";
    SqlContext<bool> ctx(db, std::move(qs), BP1(list_id));
    return ctx(true);
}


DiagnosticSQLResult<int> AddNewFandomList(QString name, sql::Database db)
{
    int maxFandomId = 0;
    {
        std::string qs = "select max(id) as maxid from fandom_lists";
        SqlContext<int>ctx(db, std::move(qs));
        ctx.FetchSingleValue<int>("maxid", -1);
        maxFandomId = ctx.result.data;
    }
    DiagnosticSQLResult<int> result;
    int id = maxFandomId + 1;
    std::string qs = "insert into fandom_lists(id, name) values(:id, :name)";
    SqlContext<int> ctx(db, std::move(qs), BP2(id, name));
    ctx(true);
    if(!ctx.result.success){
        result.data = -1;
        result.success = false;
    }
    else
        result.data = id;
    return result;
}

DiagnosticSQLResult<bool> EditFandomStateForList(const core::fandom_lists::FandomStateInList & fandomState, sql::Database db)
{
    std::string qs = "update fandom_list_data set "
                     " enabled_state = :enabled_state,"
                     " inclusion_mode = :inclusion_mode,"
                     " crossover_mode = :crossover_mode,"
                     " ui_index = :ui_index "
                     " where list_id = :list_id and fandom_id = :fandom_id";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("enabled_state", fandomState.isEnabled);
    ctx.bindValue("inclusion_mode", static_cast<int>(fandomState.inclusionMode));
    ctx.bindValue("crossover_mode", static_cast<int>(fandomState.crossoverInclusionMode));
    ctx.bindValue("ui_index", fandomState.uiIndex);
    ctx.bindValue("list_id", fandomState.list_id);
    ctx.bindValue("fandom_id", fandomState.id);
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<bool> EditListState(const core::fandom_lists::List& listState, sql::Database db)
{
    std::string qs = "update fandom_lists set "
                     " name = :name,"
                     " is_enabled = :is_enabled,"
                     " is_expanded = :is_expanded,"
                     " ui_index = :ui_index"
                     " where id = :id ";

    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("name", listState.name);
    ctx.bindValue("is_enabled", listState.isEnabled);
    ctx.bindValue("is_expanded", listState.isExpanded);
    ctx.bindValue("ui_index", listState.uiIndex);
    ctx.bindValue("id", listState.id);
    ctx.ExecAndCheck(true);
    return ctx.result;
}


DiagnosticSQLResult<bool> FlipListValues(uint32_t list_id, sql::Database db){
    std::string qs = "update fandom_list_data set "
                       " inclusion_mode = CASE WHEN inclusion_mode = 1 THEN 0 ELSE 1 END"
                     "  where list_id = :list_id  ";
    SqlContext<bool> ctx(db, std::move(qs), BP1(list_id));
    return ctx(true);
}

DiagnosticSQLResult<QList<int>> GetRecommendersForFicIdAndListId(int fic_id, sql::Database db)
{
    std::string qs = "Select distinct recommender_id from recommendations where fic_id = :fic_id";
    SqlContext<QList<int>> ctx(db, "", BP1(fic_id));
    ctx.FetchLargeSelectIntoList<int>("recommender_id", std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<QSet<int> > GetAllTaggedFics(sql::Database db)
{
        std::string qs = "select distinct fic_id from fictags ";
        SqlContext<QSet<int>> ctx(db);
        ctx.FetchLargeSelectIntoList<int>("fic_id", std::move(qs));
        return std::move(ctx.result);
}

DiagnosticSQLResult<QSet<int>> GetFicsTaggedWith(QStringList tags, bool useAND, sql::Database db){

    if(!useAND)
    {
        std::string qs = "select distinct fic_id from fictags ";
        QStringList parts;

        if(tags.size() > 0)
            parts.push_back(QString("tag in ('%1')").arg(tags.join("','")));

        if(parts.size() > 0)
        {
            qs+= " where ";
            qs+= parts.join(" and ").toStdString();
        }
        SqlContext<QSet<int>> ctx(db);
        ctx.FetchLargeSelectIntoList<int>("fic_id", std::move(qs));
        return std::move(ctx.result);
    }
    else {
        std::string qs = "select distinct fic_id from fictags ft where ";
        QString prototype = " exists (select fic_id from fictags where ft.fic_id = fic_id and tag = '%1') ";
        QStringList parts;

        QStringList tokens;
        tokens.reserve(tags.size());
        for(const auto& tag : tags)
        {
            tokens.push_back(prototype.arg(tag));
        }
        qs += tokens.join(" and ").toStdString();
        SqlContext<QSet<int>> ctx(db);
        ctx.FetchLargeSelectIntoList<int>("fic_id", std::move(qs));
        return std::move(ctx.result);
    }
}

DiagnosticSQLResult<QSet<int> > GetAuthorsForTags(QStringList tags, sql::Database db){
    std::string qs = "select distinct author_id from ficauthors ";
    qs += fmt::format(" where fic_id in (select distinct fic_id from fictags where tag in ('{0}'))", tags.join("','").toStdString());
    //qDebug() << QString::fromStdString(qs);
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("author_id", std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<QString, int> > GetTagSizes(QStringList tags, sql::Database db)
{
    std::string qs = "select tag, count(tag) as count_tags from fictags {0} group by tag ";
    if(tags.size() > 0)
        qs = fmt::format(qs, "where tag in ('" + tags.join("','").toStdString() + "')");
    else
        qs = fmt::format(qs,"");

    SqlContext<QHash<QString, int>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data.insert(QString::fromStdString(q.value("tag").toString()),q.value("count_tags").toInt());
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> RemoveTagsFromEveryFic(QStringList tags, sql::Database db)
{
    DiagnosticSQLResult<bool> result;
    if(tags.size() == 0)
    {
        result.success = true;
        return result;
    }

    std::string qs = "delete from fictags where tag in ({0})";
    qs = fmt::format(qs, "'" + tags.join("','").toStdString() + "'");
    return SqlContext<bool> (db, std::move(qs))();
}



DiagnosticSQLResult<QHash<int, core::FanficCompletionStatus> > GetSnoozeInfo(sql::Database db)
{
    std::string qs = "select id, ffn_id, complete, chapters from fanfics where cfInFicSelection(id) > 0";
    SqlContext<QHash<int, core::FanficCompletionStatus>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        //qDebug() << " loading snooze data:";
        core::FanficCompletionStatus info;
        info.ficId = q.value("id").toInt();
        info.finished = q.value("complete").toInt();
        info.atChapter = q.value("chapters").toInt();
        //qDebug() << " snoozed ficid: " << info.ficId ;
        //qDebug() << " snoozed fic ffn id: " << q.value("ffn_id").toInt();
        //qDebug() << " finished: " << info.finished;
        //qDebug() << " atChapter: " << info.atChapter;
        //qDebug() << "///////////////";
        ctx.result.data[info.ficId] = info;
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<int, core::FanficSnoozeStatus>> GetUserSnoozeInfo(bool fetchExpired, bool limitedSelection, sql::Database db){
    std::string qs = "select fic_id, snooze_added, snoozed_until_finished, snoozed_at_chapter,  snoozed_till_chapter, expired from ficsnoozes {0} order by fic_id asc";

    QStringList filters;

    if(limitedSelection)
        filters.push_back(" cfInFicSelection(fic_id) > 0 ");

    if(!fetchExpired)
        filters.push_back(" expired == 0 ");

    if(filters.size() > 0)
        qs=fmt::format(qs, " where " + filters.join(" and ").toStdString());
    else
        qs=fmt::format(qs,"");

    QLOG_TRACE() <<  "snooze query: " << QString::fromStdString(qs);


    SqlContext<QHash<int, core::FanficSnoozeStatus>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        core::FanficSnoozeStatus info;
        info.ficId =                q.value("fic_id").toInt();
        info.added =                q.value("snooze_added").toDateTime();
        info.expired =              q.value("expired").toBool();
        info.untilFinished =        q.value("snoozed_until_finished").toInt();
        info.snoozedAtChapter=      q.value("snoozed_at_chapter").toInt();
        info.snoozedTillChapter =   q.value("snoozed_till_chapter").toInt();

        ctx.result.data[info.ficId] = info;
    });
    return std::move(ctx.result);
}


DiagnosticSQLResult<QHash<int, QString>> GetNotesForFics(bool limitedSelection , sql::Database db){
    std::string qs = "select * from ficnotes {0} order by fic_id asc";

    if(limitedSelection)
        qs = fmt::format(qs, " where cfInFicSelection(fic_id) > 0 ");
    else
        qs = fmt::format(qs,"");

    SqlContext<QHash<int, QString>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data[q.value("fic_id").toInt()] = QString::fromStdString(q.value("note_content").toString());
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<int, int>> GetReadingChaptersForFics(bool limitedSelection, sql::Database db)
{
    std::string qs = "select * from FicReadingTracker {0} order by fic_id asc";

    if(limitedSelection)
        qs = fmt::format(qs, " where cfInFicSelection(fic_id) > 0 ");
    else
        qs = fmt::format(qs, "");

    SqlContext<QHash<int, int>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data[q.value("fic_id").toInt()] = q.value("at_chapter").toInt();
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteExpiredSnoozes(QSet<int> data,sql::Database db){
    std::string qs = "update ficsnoozes set expired = 1 where fic_id = :fic_id";
    SqlContext<bool> ctx(db, std::move(qs));
    for(auto ficId : data)
    {
        ctx.bindValue("fic_id", ficId);
        if(!ctx.ExecAndCheck())
            break;
    }
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> SnoozeFic(const core::FanficSnoozeStatus& data,sql::Database db){
    std::string qs = "INSERT INTO FicSnoozes(fic_id, snoozed_at_chapter, snoozed_till_chapter, snoozed_until_finished, snooze_added)"
                 " values(:fic_id, :snoozed_at_chapter, :snoozed_till_chapter, :snoozed_until_finished,  date('now')) "
                 " on conflict (fic_id) "
                 " do update set "
                 " snoozed_at_chapter = :snoozed_at_chapter_, "
                 " snoozed_till_chapter = :snoozed_till_chapter_, "
                 " snoozed_until_finished = :snoozed_until_finished_,"
                 " snooze_added = date('now'), "
                 " expired = 0 "
                 " where fic_id = :fic_id_";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("fic_id", data.ficId);
    ctx.bindValue("snoozed_at_chapter", data.snoozedAtChapter);
    if(!data.untilFinished)
        ctx.bindValue("snoozed_till_chapter", data.snoozedTillChapter);
    else
        ctx.bindValue("snoozed_till_chapter", -1);
    ctx.bindValue("snoozed_until_finished", data.untilFinished);
    ctx.bindValue("snoozed_at_chapter_", data.snoozedAtChapter);
    if(!data.untilFinished)
        ctx.bindValue("snoozed_till_chapter_", data.snoozedTillChapter);
    else
        ctx.bindValue("snoozed_till_chapter_", -1);
    ctx.bindValue("snoozed_until_finished_", data.untilFinished);
    ctx.bindValue("fic_id_", data.ficId);
    ctx.bindValue("fic_id", data.ficId);
    ctx.ExecAndCheck();
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> RemoveSnooze(int fic_id,sql::Database db){
    std::string qs = "delete from FicSnoozes where fic_id = :fic_id";
    return SqlContext<bool> (db, std::move(qs), BP1(fic_id))();
}

DiagnosticSQLResult<bool> AddNoteToFic(int fic_id, QString note, sql::Database db)
{
    std::string qs = "INSERT INTO ficnotes(fic_id, note_content, updated) values(:fic_id, :note, date('now')) "
                 "on conflict (fic_id) do update set note_content = :note_, updated = date('now') where fic_id = :fic_id_";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("fic_id", fic_id);
    ctx.bindValue("note", note);
    ctx.bindValue("note_", note);
    ctx.bindValue("fic_id_", fic_id);
    ctx.ExecAndCheck();
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> RemoveNoteFromFic(int fic_id, sql::Database db)
{
    std::string qs = "delete from ficnotes where fic_id = :fic_id";
    return SqlContext<bool> (db, std::move(qs), BP1(fic_id))();
}


DiagnosticSQLResult<QVector<int> > GetAllFicsThatDontHaveDBID(sql::Database db)
{
    std::string qs = "select distinct ffn_id from fictags where fic_id < 1";
    SqlContext<QVector<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("ffn_id", std::move(qs));
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> FillDBIDsForFics(QVector<core::Identity> pack, sql::Database db)
{
    std::string qs = "update fictags set fic_id = :id where ffn_id = :ffn_id";
    SqlContext<bool> ctx(db, std::move(qs));
    for(const core::Identity& identity: pack)
    {
        if(identity.id < 1)
            continue;

        ctx.bindValue("id", identity.id);
        ctx.bindValue("ffn_id", identity.web.ffn);
        if(!ctx.ExecAndCheck())
        {
            ctx.result.success = false;
            break;
        }
    }
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> FetchTagsForFics(QVector<core::Fanfic> * fics, sql::Database db)
{
    std::string qs = "select fic_id,  group_concat(tag, ' ')  as tags from fictags where cfInSourceFics(fic_id) > 0 group by fic_id";
    QHash<int, QString> tags;
    auto* data= ThreadData::GetRecommendationData();
    auto& hash = data->sourceFics;

    for(const auto& fic : std::as_const(*fics))
        hash.insert(fic.identity.id);

    SqlContext<bool> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        tags[q.value("fic_id").toInt()] = QString::fromStdString(q.value("tags").toString());
    });
    for(auto& fic : *fics)
        fic.userData.tags = tags[fic.identity.id];
    return std::move(ctx.result);
}


template <typename T1, typename T2>
inline double DivideAsDoubles(T1 arg1, T2 arg2){
    return static_cast<double>(arg1)/static_cast<double>(arg2);
}
DiagnosticSQLResult<bool> FetchRecommendationsBreakdown(QVector<core::Fanfic> * fics, int listId, sql::Database db)
{
    std::string qs = "select fic_id,  "
                         "breakdown_available,"
                         "votes_common, votes_uncommon, votes_rare, votes_unique, "
                         "value_common, value_uncommon, value_rare, value_unique, purged "
                         "from RecommendationListData where "
                         " cfInSourceFics(fic_id) > 0 and list_id = :listId"
                         " group by fic_id";
    QHash<int, QStringList> breakdown;
    QHash<int, QStringList> breakdownCounts;
    auto* data= ThreadData::GetRecommendationData();
    auto& sourceSet = data->sourceFics;

    for(const auto& fic : std::as_const(*fics))
        sourceSet.insert(fic.identity.id);
    QSet<int> purgedFics;
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("listId", listId);
    ctx.ForEachInSelect([&](sql::Query& q){
        auto ficId = q.value("fic_id").toInt();
        int sumtotal = q.value("value_common").toInt() +
                q.value("value_uncommon").toInt() +
                q.value("value_rare").toInt() +
                q.value("value_unique").toInt();
        int common = static_cast<int>(DivideAsDoubles(q.value("value_common").toInt(),sumtotal)*100);
        int uncommon = static_cast<int>(DivideAsDoubles(q.value("value_uncommon").toInt(),sumtotal)*100);
        int rare = static_cast<int>(DivideAsDoubles(q.value("value_rare").toInt(),sumtotal)*100);
        int unique = static_cast<int>(DivideAsDoubles(q.value("value_unique").toInt(),sumtotal)*100);
        bool purged = q.value("purged").toBool();
        if(purged == true)
            purgedFics.insert(ficId);

        breakdown[ficId].push_back(QString::number(common));
        breakdown[ficId].push_back(QString::number(uncommon));
        breakdown[ficId].push_back(QString::number(rare));
        breakdown[ficId].push_back(QString::number(unique));


        breakdownCounts[ficId].push_back(QString::fromStdString(q.value("votes_common").toString()));
        breakdownCounts[ficId].push_back(QString::fromStdString(q.value("votes_uncommon").toString()));
        breakdownCounts[ficId].push_back(QString::fromStdString(q.value("votes_rare").toString()));
        breakdownCounts[ficId].push_back(QString::fromStdString(q.value("votes_unique").toString()));


    });


    for(auto& fic : *fics)
    {
        fic.recommendationsData.voteBreakdown = breakdown[fic.identity.id];
        fic.recommendationsData.voteBreakdownCounts = breakdownCounts[fic.identity.id];
        if(purgedFics.contains(fic.identity.id))
            fic.recommendationsData.purged = true;
        else
            fic.recommendationsData.purged = false;
    }
    return std::move(ctx.result);
}


DiagnosticSQLResult<bool> FetchRecommendationScoreForFics(QHash<int, int>& scores, const core::ReclistFilter &filter, sql::Database db)
{
    // need to create a list of ids to query for
    QStringList ids;
    ids.reserve(scores.size());
    auto it = scores.cbegin();
    auto itEnd = scores.cend();
    while(it != itEnd){
        ids.push_back(QString::number(it.key()));
        it++;
    }

    std::string qs = "select fic_id, {0} from RecommendationListData where list_id = :list_id and fic_id in ({1})";
    std::string pointsField = filter.scoreType == core::StoryFilter::st_points ? "match_count" : "no_trash_score";
    qs = fmt::format(qs, pointsField, ids.join(",").toStdString());

    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("list_id", filter.mainListId);

    ctx.ForEachInSelect([&](sql::Query& q){
        scores[q.value("fic_id").toInt()] = q.value(pointsField).toInt();
    });
    return std::move(ctx.result);


}

DiagnosticSQLResult<bool> LoadPlaceAndRecommendationsData(QVector<core::Fanfic> *fics, const core::ReclistFilter& filter, sql::Database db)
{
    QStringList ficIds;

    QHash<int, int> indices;
    int i = 0;
    ficIds.reserve(fics->size());
    for(const auto& fic: std::as_const(*fics))
    {
        ficIds.push_back(QString::number(fic.identity.id));
        indices[fic.identity.id] = i;
        i++;
    }
    QStringList listIds;
    listIds << QString::number(filter.mainListId);
    if(filter.secondListId != -1)
        listIds << QString::number(filter.secondListId);


    std::string qs = "select fic_id, list_id, {0}, position, pedestal from RecommendationListData where list_id in ({1}) and fic_id in ({2})";
    std::string pointsField = filter.scoreType == core::StoryFilter::st_points ? "match_count" : "no_trash_score";
    qs = fmt::format(qs, pointsField, listIds.join(",").toStdString(), ficIds.join(",").toStdString());

    SqlContext<bool> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        int ficId = q.value("fic_id").toInt();
        auto& fic = (*fics)[indices[ficId]];
        if(q.value("list_id").toInt() == filter.mainListId)
        {
            fic.recommendationsData.recommendationsMainList = q.value(pointsField.c_str()).toInt();
            fic.recommendationsData.placeInMainList = q.value("position").toInt();
            fic.recommendationsData.placeOnSecondPedestal = q.value("pedestal").toInt();
        }
        else{
            fic.recommendationsData.recommendationsSecondList = q.value(pointsField.c_str()).toInt();
            fic.recommendationsData.placeInSecondList = q.value("position").toInt();
            fic.recommendationsData.placeOnSecondPedestal= q.value("pedestal").toInt();
        }
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QSharedPointer<core::RecommendationList>> FetchParamsForRecList(int id, sql::Database db)
{
    std::string qs = " select * from recommendationlists where id = :id ";
    SqlContext<QSharedPointer<core::RecommendationList>> ctx(db, std::move(qs), BP1(id));
    //QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data = QSharedPointer<core::RecommendationList>{new core::RecommendationList};
        ctx.result.data->id = q.value("id").toInt();
        ctx.result.data->name = QString::fromStdString(q.value("name").toString());
        ctx.result.data->maxUnmatchedPerMatch = q.value("pick_ratio").toInt();
        ctx.result.data->alwaysPickAt = q.value("always_pick_at").toInt();
        ctx.result.data->minimumMatch = q.value("minimum").toInt();
        ctx.result.data->useWeighting = q.value("use_weighting").toBool();
        ctx.result.data->useMoodAdjustment= q.value("use_mood_adjustment").toBool();
        ctx.result.data->useDislikes = q.value("use_dislikes").toBool();
        ctx.result.data->useDeadFicIgnore= q.value("use_dead_fic_ignore").toBool();
        ctx.result.data->isAutomatic = q.value("is_automatic").toBool();
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> SetFicsAsListOrigin(QVector<int> ficIds, int list_id, sql::Database db)
{
    std::string qs = "update RecommendationListData set is_origin = 1 where fic_id = :fic_id and list_id = :list_id";
    SqlContext<bool> ctx(db, std::move(qs), BP1(list_id));
    ctx.ExecuteWithValueList("fic_id", std::move(ficIds));
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> DeleteLinkedAuthorsForAuthor(int author_id,  sql::Database db)
{
    std::string qs = "delete from LinkedAuthors where recommender_id = :author_id";
    return SqlContext<bool> (db, std::move(qs), BP1(author_id))();
}

DiagnosticSQLResult<bool>  UploadLinkedAuthorsForAuthor(int author_id, QString website, QList<int> ids, sql::Database db)
{
    std::string qs = fmt::format("insert into  LinkedAuthors(recommender_id, {0}_id) values(:author_id, :id)",website.toStdString());
    SqlContext<bool> ctx(db, std::move(qs), BP1(author_id));
    ctx.ExecuteWithValueList("id", std::move(ids), true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<QVector<int> > GetAllUnprocessedLinkedAuthors(sql::Database db)
{
    std::string qs = "select distinct ffn_id from linkedauthors where ffn_id not in (select ffn_id from recommenders)";
    SqlContext<QVector<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("ffn_id", std::move(qs));
    return std::move(ctx.result);
}



DiagnosticSQLResult<QStringList> GetLinkedPagesForList(int list_id, QString website, sql::Database db)
{
    std::string qs = "Select distinct {0}_id from LinkedAuthors "
                         " where recommender_id in ( select author_id from RecommendationListAuthorStats where list_id = {1}) "
                         " and {0}_id not in (select distinct {0}_id from recommenders)"
                         " union all "
                         " select ffn_id from recommenders where id not in (select distinct recommender_id from recommendations) and favourites != 0 ";
    qs=fmt::format(qs, website.toStdString(), list_id);

    SqlContext<QStringList> ctx(db, std::move(qs), BP1(list_id));
    ctx.ForEachInSelect([&](sql::Query& q){
        auto authorUrl = url_utils::GetAuthorUrlFromWebId(q.value(QString("{0}_id").arg(website).toStdString()).toInt(), "ffn");
        ctx.result.data.push_back(authorUrl);
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> RemoveAuthorRecommendationStatsFromDatabase(int list_id, int author_id, sql::Database db)
{
    std::string qs = "delete from recommendationlistauthorstats "
                         " where list_id = :list_id and author_id = :author_id";
    return SqlContext<bool>(db, std::move(qs), BP2(list_id,author_id))();
}

DiagnosticSQLResult<bool> CreateFandomIndexRecord(int id, QString name, sql::Database db)
{
    std::string qs = "insert into fandomindex(id, name) values(:id, :name)";
    return SqlContext<bool>(db, std::move(qs), BP2(name, id))();
}



DiagnosticSQLResult<QHash<int, QList<int>>> GetWholeFicFandomsTable(sql::Database db)
{
    std::string qs = "select fic_id, fandom_id from ficfandoms";
    SqlContext<QHash<int, QList<int>>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data[q.value("fandom_id").toInt()].push_back(q.value("fic_id").toInt());
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> EraseFicFandomsTable(sql::Database db)
{
    std::string qs = "delete from ficfandoms";
    return SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> SetLastUpdateDateForFandom(int id, QDate updated, sql::Database db)
{
    std::string qs = "update fandomindex set updated = :updated where id = :id";
    return SqlContext<bool>(db, std::move(qs), BP2(updated, id))();
}

DiagnosticSQLResult<bool> RemoveFandomFromRecentList(QString name, sql::Database db)
{
    std::string qs = "delete from recent_fandoms where fandom = :name";
    return SqlContext<bool>(db, std::move(qs), BP1(name))();
}


DiagnosticSQLResult<int> GetLastExecutedTaskID(sql::Database db)
{
    std::string qs = "select max(id) as maxid from pagetasks";
    SqlContext<int>ctx(db, std::move(qs));
    ctx.FetchSingleValue<int>("maxid", -1);
    return std::move(ctx.result);
}

// new query limit
DiagnosticSQLResult<bool> GetTaskSuccessByID(int id, sql::Database db)
{
    std::string qs = "select success from pagetasks where id = :id";
    SqlContext<bool>ctx(db, std::move(qs), BP1(id));
    ctx.FetchSingleValue<bool>("success", false);
    return std::move(ctx.result);
}
DiagnosticSQLResult<bool>  IsForceStopActivated(int id, sql::Database db)
{
    std::string qs = "select force_stop from pagetasks where id = :id";
    SqlContext<bool>ctx(db, std::move(qs), BP1(id));
    ctx.FetchSingleValue<bool>("force_stop", false);
    return std::move(ctx.result);
}


void FillPageTaskBaseFromQuery(BaseTaskPtr task, const sql::Query& q){
    if(!NullPtrGuard(task))
        return;
    task->created = q.value("created_at").toDateTime();
    task->scheduledTo = q.value("scheduled_to").toDateTime();
    task->startedAt = q.value("started_at").toDateTime();
    task->finishedAt= q.value("finished_at").toDateTime();
    task->type = q.value("type").toInt();
    task->size = q.value("size").toInt();
    task->retries= q.value("retries").toInt();
    task->success = q.value("success").toBool();
    task->finished= q.value("finished").toBool();
    task->addedFics = q.value("inserted_fics").toInt();
    task->addedAuthors= q.value("inserted_authors").toInt();
    task->updatedFics= q.value("updated_fics").toInt();
    task->updatedAuthors= q.value("updated_authors").toInt();
    task->parsedPages = q.value("parsed_pages").toInt();
}


void FillPageTaskFromQuery(PageTaskPtr task, const sql::Query& q){
    if(!NullPtrGuard(task))
        return;

    FillPageTaskBaseFromQuery(task, q);
    task->id= q.value("id").toInt();
    task->parts = q.value("parts").toInt();
    task->results = QString::fromStdString(q.value("results").toString());
    task->taskComment= QString::fromStdString(q.value("task_comment").toString());
    task->size = q.value("task_size").toInt();
    task->allowedRetries = q.value("allowed_retry_count").toInt();
    if(q.value("cache_mode").toInt() == 0)
        task->cacheStrategy = fetching::CacheStrategy::NetworkOnly();
    else if(q.value("cache_mode").toInt() == 1)
        task->cacheStrategy = fetching::CacheStrategy::CacheThenFetchIfNA();
    else
        task->cacheStrategy = fetching::CacheStrategy::CacheOnly();
    task->refreshIfNeeded= q.value("refresh_if_needed").toBool();
}

void FillSubTaskFromQuery(SubTaskPtr task, const sql::Query& q){
    if(!NullPtrGuard(task))
        return;

    FillPageTaskBaseFromQuery(task, q);
    task->parentId= q.value("task_id").toInt();
    task->id = q.value("sub_id").toInt();
    SubTaskContentBasePtr content;

    if(task->type == 0)
    {
        content = SubTaskAuthorContent::NewContent();
        auto cast = dynamic_cast<SubTaskAuthorContent*>(content.data());
        cast->authors = QString::fromStdString(q.value("content").toString()).split("\n");
    }
    if(task->type == 1)
    {
        content = SubTaskFandomContent::NewContent();
        auto cast = dynamic_cast<SubTaskFandomContent*>(content.data());
        cast->urlLinks = QString::fromStdString(q.value("content").toString()).split("\n");
        cast->fandom = QString::fromStdString(q.value("custom_data1").toString());
        cast->fandom = QString::fromStdString(q.value("custom_data1").toString());
    }
    task->updateLimit = q.value("parse_up_to").toDateTime();
    task->content = content;
    task->isValid = true;
    task->isNew =false;
}

DiagnosticSQLResult<PageTaskPtr> GetTaskData(int id, sql::Database db)
{
    std::string qs = "select * from pagetasks where id = :id";
    SqlContext<PageTaskPtr>ctx(db, std::move(qs), BP1(id));
    ctx.result.data = PageTask::CreateNewTask();
    if(!ctx.ExecAndCheckForData())
        return std::move(ctx.result);

    FillPageTaskFromQuery(ctx.result.data, ctx.q);
    return std::move(ctx.result);
}

DiagnosticSQLResult<SubTaskList> GetSubTaskData(int id, sql::Database db)
{
    std::string qs = "select * from PageTaskParts where task_id = :id";

    SqlContext<SubTaskList>ctx(db, std::move(qs), {{"id", id}});
    return ctx.ForEachInSelect([&](sql::Query& q){
        auto subtask = PageSubTask::CreateNewSubTask();
        FillSubTaskFromQuery(subtask, q);
        ctx.result.data.push_back(subtask); });
}

void FillPageFailuresFromQuery(PageFailurePtr failure, const sql::Query& q){
    if(!NullPtrGuard(failure))
        return;

    failure->action = QSharedPointer<PageTaskAction>(new PageTaskAction{QString::fromStdString(q.value("action_uuid").toString()),
                                                                        q.value("task_id").toInt(),
                                                                        q.value("sub_id").toInt()});
    failure->attemptTimeStamp = q.value("process_attempt").toDateTime();
    failure->errorCode  = static_cast<PageFailure::EFailureReason>(q.value("error_code").toInt());
    failure->errorlevel  = static_cast<PageFailure::EErrorLevel>(q.value("error_level").toInt());
    failure->lastSeen = q.value("last_seen_at").toDateTime();
    failure->url = QString::fromStdString(q.value("url").toString());
    failure->error = QString::fromStdString(q.value("error").toString());
}

void FillActionFromQuery(PageTaskActionPtr action, const sql::Query& q){
    if(!NullPtrGuard(action))
        return;

    action->SetData(QString::fromStdString(q.value("action_uuid").toString()),
                    q.value("task_id").toInt(),
                    q.value("sub_id").toInt());

    action->success = q.value("success").toBool();
    action->started = q.value("started_at").toDateTime();
    action->finished = q.value("finished_at").toDateTime();
    action->isNewAction = false;
}

DiagnosticSQLResult<SubTaskErrors> GetErrorsForSubTask(int id,  sql::Database db, int subId)
{
    std::string qs = "select * from PageWarnings where task_id = :id";
    bool singleSubTask = subId != -1;
    if(singleSubTask)
        qs+= " and sub_id = :sub_id";

    SqlContext<SubTaskErrors>ctx(db, std::move(qs), BP1(id));

    if(singleSubTask)
        ctx.bindValue("sub_id", subId);

    ctx.ForEachInSelect([&](sql::Query& q){
        auto failure = PageFailure::CreateNewPageFailure();
        FillPageFailuresFromQuery(failure, q);
        ctx.result.data.push_back(failure);
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<PageTaskActions> GetActionsForSubTask(int id, sql::Database db, int subId)
{

    std::string qs = "select * from PageTaskActions where task_id = :id";
    bool singleSubTask = subId != -1;
    if(singleSubTask)
        qs+= " and sub_id = :sub_id";
    SqlContext<PageTaskActions>ctx(db, std::move(qs), BP1(id));

    if(singleSubTask)
        ctx.bindValue("sub_id", subId);

    ctx.ForEachInSelect([&](sql::Query& q){
        auto action = PageTaskAction::CreateNewAction();
        FillActionFromQuery(action, q);
        ctx.result.data.push_back(action);
    });

    return std::move(ctx.result);
}

DiagnosticSQLResult<int> CreateTaskInDB(PageTaskPtr task, sql::Database db)
{
    std::string qs = "insert into PageTasks(type, parts, created_at, scheduled_to,  allowed_retry_count, "
                         "allowed_subtask_retry_count, cache_mode, refresh_if_needed, task_comment, task_size, success, finished,"
                         "parsed_pages, updated_fics,inserted_fics,inserted_authors,updated_authors) "
                         "values(:type, :parts, :created_at, :scheduled_to, :allowed_retry_count,"
                         ":allowed_subtask_retry_count, :cache_mode, :refresh_if_needed, :task_comment,:task_size, 0, 0,"
                         " :parsed_pages, :updated_fics,:inserted_fics,:inserted_authors,:updated_authors) ";

    SqlContext<int>ctx(db, std::move(qs));
    ctx.result.data = -1;
    ctx.bindValue("type", task->type);
    ctx.bindValue("parts", task->parts);
    ctx.bindValue("created_at", task->created);
    ctx.bindValue("scheduled_to", task->scheduledTo);
    ctx.bindValue("allowed_retry_count", task->allowedRetries);
    ctx.bindValue("allowed_subtask_retry_count", task->allowedSubtaskRetries);
    if(task->cacheStrategy.useCache == false)
        ctx.bindValue("cache_mode", 0);
    else if(task->cacheStrategy.fetchIfCacheUnavailable == false)
        ctx.bindValue("cache_mode", 2);
    else
        ctx.bindValue("cache_mode", 1);
    ctx.bindValue("refresh_if_needed", task->refreshIfNeeded);
    ctx.bindValue("task_comment", task->taskComment);
    ctx.bindValue("task_size", task->size);
    ctx.bindValue("parsed_pages",      task->parsedPages);
    ctx.bindValue("updated_fics",      task->updatedFics);
    ctx.bindValue("inserted_fics",     task->addedFics);
    ctx.bindValue("inserted_authors",  task->addedAuthors);
    ctx.bindValue("updated_authors",   task->updatedAuthors);
    if(!ctx.ExecAndCheck())
        return std::move(ctx.result);

    ctx.ReplaceQuery("select max(id) as maxid from PageTasks");
    ctx.FetchSingleValue<int>("maxid", -1);
    return std::move(ctx.result);

}

DiagnosticSQLResult<bool> CreateSubTaskInDB(SubTaskPtr subtask, sql::Database db)
{
    std::string qs = "insert into PageTaskParts(task_id, type, sub_id, created_at, scheduled_to, content,task_size, success, finished, parse_up_to,"
                         "custom_data1, parsed_pages, updated_fics,inserted_fics,inserted_authors,updated_authors) "
                         "values(:task_id, :type, :sub_id, :created_at, :scheduled_to, :content,:task_size, 0,0, :parse_up_to,"
                         ":custom_data1, :parsed_pages, :updated_fics,:inserted_fics,:inserted_authors,:updated_authors) ";
    SqlContext<bool>ctx(db, std::move(qs));

    ctx.bindValue("task_id", subtask->parentId);
    ctx.bindValue("type", subtask->type);
    ctx.bindValue("sub_id", subtask->id);
    ctx.bindValue("created_at", subtask->created);
    ctx.bindValue("scheduled_to", subtask->scheduledTo);
    ctx.bindValue("content", subtask->content->ToDB());
    ctx.bindValue("task_size", subtask->size);
    ctx.bindValue("parse_up_to", subtask->updateLimit);

    QString customData = subtask->content->CustomData1();
    ctx.bindValue("custom_data1",      customData);
    ctx.bindValue("parsed_pages",      subtask->parsedPages);
    ctx.bindValue("updated_fics",      subtask->updatedFics);
    ctx.bindValue("inserted_fics",     subtask->addedFics);
    ctx.bindValue("inserted_authors",  subtask->addedAuthors);
    ctx.bindValue("updated_authors",   subtask->updatedAuthors);
    return ctx();

}

DiagnosticSQLResult<bool> CreateActionInDB(PageTaskActionPtr action, sql::Database db)
{
    std::string qs = "insert into PageTaskActions(action_uuid, task_id, sub_id, started_at, finished_at, success) "
                         "values(:action_uuid, :task_id, :sub_id, :started_at, :finished_at, :success) ";
    SqlContext<bool>ctx(db, std::move(qs));
    ctx.bindValue("action_uuid", action->id.toString());
    ctx.bindValue("task_id", action->taskId);
    ctx.bindValue("sub_id", action->subTaskId);
    ctx.bindValue("started_at", action->started);
    ctx.bindValue("finished_at", action->finished);
    ctx.bindValue("success", action->success);

    return ctx();
}

DiagnosticSQLResult<bool> CreateErrorsInDB(const SubTaskErrors& errors, sql::Database db)
{
    std::string qs = "insert into PageWarnings(action_uuid, task_id, sub_id, url, attempted_at, last_seen_at, error_code, error_level, error) "
                         "values(:action_uuid, :task_id, :sub_id, :url, :attempted_at, :last_seen, :error_code, :error_level, :error) ";

    SqlContext<bool>ctx(db, std::move(qs));
        ctx.ExecuteWithKeyListAndBindFunctor<QList, QSharedPointer, PageFailure>(errors, +[](PageFailurePtr error, sql::Query& q){
        q.bindValue("action_uuid", error->action->id.toString());
        q.bindValue("task_id", error->action->taskId);
        q.bindValue("sub_id", error->action->subTaskId);
        q.bindValue("url", error->url);
        q.bindValue("attempted_at", error->attemptTimeStamp);
        q.bindValue("last_seen", error->lastSeen);
        q.bindValue("error_code", static_cast<int>(error->errorCode));
        q.bindValue("error_level", static_cast<int>(error->errorlevel));
        q.bindValue("error", error->error);
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> UpdateTaskInDB(PageTaskPtr task, sql::Database db)
{

    std::string qs = "update PageTasks set scheduled_to = :scheduled_to, started_at = :started, finished_at = :finished_at,"
                         " results = :results, retries = :retries, success = :success, task_size = :size, finished = :finished,"
                         " parsed_pages = :parsed_pages, updated_fics = :updated_fics, inserted_fics = :inserted_fics,"
                         " inserted_authors = :inserted_authors, updated_authors = :updated_authors"
                         " where id = :id";
    SqlContext<bool>ctx(db, std::move(qs));

    ctx.bindValue("scheduled_to",      task->scheduledTo);
    ctx.bindValue("started_at",        task->startedAt);
    ctx.bindValue("finished_at",       task->finishedAt);
    ctx.bindValue("finished",          task->finished);
    ctx.bindValue("results",           task->results);
    ctx.bindValue("retries",           task->retries);
    ctx.bindValue("success",           task->success);
    ctx.bindValue("id",                task->id);
    ctx.bindValue("size",              task->size);
    ctx.bindValue("parsed_pages",      task->parsedPages);
    ctx.bindValue("updated_fics",      task->updatedFics);
    ctx.bindValue("inserted_fics",     task->addedFics);
    ctx.bindValue("inserted_authors",  task->addedAuthors);
    ctx.bindValue("updated_authors",   task->updatedAuthors);

    return ctx();
}

DiagnosticSQLResult<bool> UpdateSubTaskInDB(SubTaskPtr task, sql::Database db)
{
    std::string qs = "update PageTaskParts set scheduled_to = :scheduled_to, started_at = :started_at, finished_at = :finished_at,"
                         " retries = :retries, success = :success, finished = :finished, "
                         " parsed_pages = :parsed_pages, updated_fics = :updated_fics, inserted_fics = :inserted_fics,"
                         " inserted_authors = :inserted_authors, updated_authors = :updated_authors, custom_data1 = :custom_data1"
                         " where task_id = :task_id and sub_id = :sub_id";

    SqlContext<bool>ctx(db, std::move(qs));
    QString customData = task->content->CustomData1();

    ctx.bindValue("scheduled_to",     task->scheduledTo);
    ctx.bindValue("started_at",       task->startedAt);
    ctx.bindValue("finished_at",      task->finishedAt);
    ctx.bindValue("retries",          task->retries);
    ctx.bindValue("success",          task->success);
    ctx.bindValue("finished",         task->finished);
    ctx.bindValue("parsed_pages",      task->parsedPages);
    ctx.bindValue("updated_fics",      task->updatedFics);
    ctx.bindValue("inserted_fics",     task->addedFics);
    ctx.bindValue("inserted_authors",  task->addedAuthors);
    ctx.bindValue("updated_authors",   task->updatedAuthors);
    ctx.bindValue("custom_data1",      customData);

    ctx.bindValue("task_id",          task->parentId);
    ctx.bindValue("sub_id",           task->id);

    return ctx();


}

DiagnosticSQLResult<bool> SetTaskFinished(int id, sql::Database db)
{
    return SqlContext<bool> (db, "update PageTasks set finished = 1 where id = :task_id",{
                                 {"task_id", id}
                             })();
}

DiagnosticSQLResult<TaskList> GetUnfinishedTasks(sql::Database db)
{
    SqlContext<QList<int>> ctx1(db);
    ctx1.FetchLargeSelectIntoList<int>("id", "select id from pagetasks where finished = 0");

    SqlContext<TaskList> ctx2(db, "select * from pagetasks where id = :task_id");
    if(!ctx1.result.success)
        return ctx2.result;

    for(auto id: std::as_const(ctx1.result.data))
    {
        ctx2.bindValue("task_id",id);
        if(!ctx2.ExecAndCheckForData())
            return ctx2.result;
        auto task = PageTask::CreateNewTask();
        FillPageTaskFromQuery(task, ctx2.q);
        ctx2.result.data.push_back(task);
    }
    return ctx2.result;
}



// !!!! requires careful testing!
DiagnosticSQLResult<bool> ExportTagsToDatabase(sql::Database originDB, sql::Database targetDB)
{
    thread_local auto idHash = GetGlobalIDHash(originDB, " where id in (select distinct fic_id from fictags)").data;
    {

        std::vector<std::string> targetKeyList = {"ffn_id","ao3_id","sb_id","sv_id","tag",};
        std::vector<std::string> sourceKeyList = {"ffn","ao3","sb","sv","tag",};
        std::string insertQS = "insert into UserFicTags(ffn_id, ao3_id, sb_id, sv_id, tag) values(:ffn_id, :ao3_id, :sb_id, :sv_id, :tag)";
        ParallelSqlContext<bool> ctx (originDB, "select fic_id, tag from fictags order by fic_id, tag", std::move(sourceKeyList),
                                      targetDB, std::move(insertQS), std::move(targetKeyList));

        auto keyConverter = [&](const std::string& sourceKey, sql::Query q, sql::Database , auto& )->Variant
        {
            auto record = idHash.GetRecord(q.value("fic_id").toInt());
            return record.GetID(sourceKey.c_str());
        };

        ctx.valueConverters["ffn"] = keyConverter;
        ctx.valueConverters["ao3"] = keyConverter;
        ctx.valueConverters["sb"] = keyConverter;
        ctx.valueConverters["sv"] = keyConverter;
        ctx();
        if(!ctx.Success())
            return std::move(ctx.result);
    }
    {
        std::vector<std::string> keyList = {"tag","id"};
        std::string insertQS = "insert into UserTags(fic_id, tag) values(:id, :tag)";
        ParallelSqlContext<bool> ctx (originDB, "select * from tags", std::move(keyList),
                                      targetDB, std::move(insertQS), std::move(keyList));
        return ctx();
    }
}

// !!!! requires careful testing!
DiagnosticSQLResult<bool> ImportTagsFromDatabase(sql::Database currentDB,sql::Database tagImportSourceDB)
{
    {
        SqlContext<bool> ctxTarget(currentDB, std::vector<std::string>{"delete from fictags","delete from tags"});
        if(!ctxTarget.result.success)
            return ctxTarget.result;
    }

    {
        std::vector<std::string> keyList = {"tag","id"};
        std::string insertQS = "insert into Tags(id, tag) values(:id, :tag)";
        ParallelSqlContext<bool> ctx (tagImportSourceDB, "select * from UserTags", std::move(keyList),
                                      currentDB, std::move(insertQS), std::move(keyList));
        ctx();
        if(!ctx.Success())
            return std::move(ctx.result);
    }
    // need to ensure fic_id in the source database
    SqlContext<bool> ctxTarget(tagImportSourceDB, std::vector<std::string>{"alter table UserFicTags add column fic_id integer default -1"});
    ctxTarget();
    if(!ctxTarget.result.success)
        return ctxTarget.result;

    //    SqlContext<bool> ctxTest(currentDB, QStringList{"insert into FicTags(fic_id, ffn_id, ao3_id, sb_id, sv_id, tag)"
    //                                                    " values(-1, -1, -1, -1, -1, -1)"});
    //    ctxTest();
    //    if(!ctxTest.result.success)
    //        return ctxTest.result;

    std::vector<std::string> keyList = {"fic_id", "ffn_id", "ao3_id", "sb_id", "sv_id", "tag"};
    std::string insertQS = "insert into FicTags(fic_id, ffn_id, ao3_id, sb_id, sv_id, tag) values(:fic_id, :ffn_id, :ao3_id, :sb_id, :sv_id, :tag)";
    //bool isOpen = currentDB.isOpen();
    ParallelSqlContext<bool> ctx (tagImportSourceDB, "select * from UserFicTags", std::move(keyList),
                                  currentDB, std::move(insertQS), std::move(keyList));
    return ctx();
}


DiagnosticSQLResult<bool> ExportSlashToDatabase(sql::Database originDB, sql::Database targetDB)
{

    std::vector<std::string> keyList = {"ffn_id","keywords_result","keywords_yes","keywords_no","filter_pass_1", "filter_pass_2"};
    std::string insertQS = "insert into slash_data_ffn(ffn_id, keywords_result, keywords_yes, keywords_no, filter_pass_1, filter_pass_2) "
                               " values(:ffn_id, :keywords_result, :keywords_yes, :keywords_no, :filter_pass_1, :filter_pass_2) ";

    return ParallelSqlContext<bool> (originDB, "select * from slash_data_ffn", std::move(keyList),
                                     targetDB, std::move(insertQS), std::move(keyList))();
}

DiagnosticSQLResult<bool> ImportSlashFromDatabase(sql::Database slashImportSourceDB, sql::Database appDB)
{

    {
        SqlContext<bool> ctxTarget(appDB, "delete from slash_data_ffn");
        if(ctxTarget.ExecAndCheck())
            return ctxTarget.result;
    }

    std::vector<std::string> keyList = {"ffn_id","keywords_result","keywords_yes","keywords_no","filter_pass_1", "filter_pass_2"};
    std::string insertQS = "insert into slash_data_ffn(ffn_id, keywords_yes, keywords_no, keywords_result, filter_pass_1, filter_pass_2)"
                               "  values(:ffn_id, :keywords_yes, :keywords_no, :keywords_result, :filter_pass_1, :filter_pass_2)";
    return ParallelSqlContext<bool> (slashImportSourceDB, "select * from slash_data_ffn", std::move(keyList),
                                     appDB, std::move(insertQS), std::move(keyList))();
}



FanficIdRecord::FanficIdRecord()
{
    ids["ffn"] = -1;
    ids["sb"] = -1;
    ids["sv"] = -1;
    ids["ao3"] = -1;
}

DiagnosticSQLResult<int> FanficIdRecord::CreateRecord(sql::Database db) const
{
    std::string query = "INSERT INTO FANFICS (ffn_id, sb_id, sv_id, ao3_id, for_fill, lastupdate) "
                    "VALUES ( :ffn_id, :sb_id, :sv_id, :ao3_id, 1, date('now'))";

    SqlContext<int> ctx(db, std::move(query),
    {{"ffn_id", ids["ffn"]},
     {"sb_id", ids["sb"]},
     {"sv_id", ids["sv"]},
     {"ao3_id", ids["ao3"]},});

    if(!ctx.ExecAndCheck())
        return std::move(ctx.result);

    ctx.ReplaceQuery("select max(id) as mid from fanfics");
    ctx.FetchSingleValue<int>("mid", 0);
    return std::move(ctx.result);
}

template <typename T>
using AllocatedVector = std::vector<T, std::allocator<T>>;

DiagnosticSQLResult<bool> AddFandomLink(int oldId, int newId, sql::Database db)
{
    SqlContext<bool> ctx(db, "select * from fandoms where id = :old_id",
    {{"old_id", oldId}});
    if(!ctx.ExecAndCheckForData())
        return std::move(ctx.result);

    std::vector<std::string> urls;
    urls.push_back(ctx.trimmedValue("normal_url"));
    urls.push_back(ctx.trimmedValue("crossover_url"));

    auto custom = ctx.trimmedValue("section");

    ctx.ReplaceQuery("insert into fandomurls (global_id, url, website, custom) values(:new_id, :url, 'ffn', :custom)");
    ctx.bindValue("custom", custom);
    ctx.bindValue("new_id",newId);

    ctx.ExecuteWithKeyListAndBindFunctor<AllocatedVector,std::string>(urls, [](std::string&& url, sql::Query& q){
        q.bindValue("url", url);
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteAuthorFavouriteStatistics(core::AuthorPtr author, sql::Database db)
{
    std::string query = "INSERT INTO AuthorFavouritesStatistics ("
                    "author_id, favourites, favourites_wordcount, average_words_per_chapter, esrb_type, prevalent_mood,"
                    "most_favourited_size,favourites_type,average_favourited_length,favourite_fandoms_diversity, explorer_factor, "
                    "mega_explorer_factor, crossover_factor,unfinished_factor,esrb_uniformity_factor,esrb_kiddy,esrb_mature,"
                    "genre_diversity_factor, mood_uniformity_factor, mood_sad, mood_neutral, mood_happy, "
                    "crack_factor,slash_factor,smut_factor, prevalent_genre, size_tiny, size_medium, size_large, size_huge,"
                    "first_published, last_published"
                    ") "
                    "VALUES ("
                    ":author_id, :favourites, :favourites_wordcount, :average_words_per_chapter, :esrb_type, :prevalent_mood,"
                    ":most_favourited_size,:favourites_type,:average_favourited_length,:favourite_fandoms_diversity, :explorer_factor, "
                    ":mega_explorer_factor, :crossover_factor,:unfinished_factor,:esrb_uniformity_factor,:esrb_kiddy,:esrb_mature,"
                    ":genre_diversity_factor, :mood_uniformity_factor, :mood_sad, :mood_neutral, :mood_happy, "
                    ":crack_factor,:slash_factor,:smut_factor, :prevalent_genre, :size_tiny, :size_medium, :size_large, :size_huge,"
                    ":first_published, :last_published"
                    ")";
    SqlContext<bool> ctx(db, std::move(query));

    auto& stats = author->stats.favouriteStats;
    ctx.bindValue("author_id", author->id);
    ctx.bindValue("favourites", stats.favourites);
    ctx.bindValue("favourites_wordcount", stats.ficWordCount);
    ctx.bindValue("average_words_per_chapter", stats.averageWordsPerChapter);
    ctx.bindValue("esrb_type", static_cast<int>(stats.esrbType));
    ctx.bindValue("prevalent_mood", static_cast<int>(stats.prevalentMood));
    ctx.bindValue("most_favourited_size", static_cast<int>(stats.mostFavouritedSize));
    ctx.bindValue("favourites_type", static_cast<int>(stats.sectionRelativeSize));
    ctx.bindValue("average_favourited_length", stats.averageLength);
    ctx.bindValue("favourite_fandoms_diversity", stats.fandomsDiversity);
    ctx.bindValue("explorer_factor", stats.explorerFactor);
    ctx.bindValue("mega_explorer_factor", stats.megaExplorerFactor);
    ctx.bindValue("crossover_factor", stats.crossoverFactor);
    ctx.bindValue("unfinished_factor", stats.unfinishedFactor);
    ctx.bindValue("esrb_uniformity_factor", stats.esrbUniformityFactor);
    ctx.bindValue("esrb_kiddy", stats.esrbKiddy);
    ctx.bindValue("esrb_mature", stats.esrbMature);
    ctx.bindValue("genre_diversity_factor", stats.genreDiversityFactor);
    ctx.bindValue("mood_uniformity_factor", stats.moodUniformity);
    ctx.bindValue("mood_sad", stats.moodSad);
    ctx.bindValue("mood_neutral", stats.moodNeutral);
    ctx.bindValue("mood_happy", stats.moodHappy);
    ctx.bindValue("crack_factor", stats.crackRatio);
    ctx.bindValue("slash_factor", stats.slashRatio);
    ctx.bindValue("smut_factor", stats.smutRatio);
    ctx.bindValue("prevalent_genre", stats.prevalentGenre);
    ctx.bindValue("size_tiny", stats.sizeFactors[0]);
    ctx.bindValue("size_medium", stats.sizeFactors[1]);
    ctx.bindValue("size_large", stats.sizeFactors[2]);
    ctx.bindValue("size_huge", stats.sizeFactors[3]);
    ctx.bindValue("first_published", stats.firstPublished);
    ctx.bindValue("last_published", stats.lastPublished);
    ctx.ExecAndCheck(true);

    return std::move(ctx.result);
}


DiagnosticSQLResult<bool> WriteAuthorFavouriteGenreStatistics(core::AuthorPtr author, sql::Database db)
{
    std::string query = "INSERT INTO AuthorFavouritesGenreStatistics (author_id, "
                    "General_,Humor,Poetry, Adventure, Mystery, Horror,Parody,Angst, Supernatural, Suspense, "
                    " Romance,SciFi, Fantasy,Spiritual,Tragedy, Drama, Western,Crime,Family,HurtComfort,Friendship, NoGenre) "
                    "VALUES ("
                    ":author_id, :General_,:Humor,:Poetry, :Adventure, :Mystery, :Horror,:Parody,:Angst, :Supernatural, :Suspense, "
                    " :Romance,:SciFi, :Fantasy,:Spiritual,:Tragedy, :Drama, :Western,:Crime,:Family,:HurtComfort,:Friendship, :NoGenre  "
                    ")";
    auto& genreFactors = author->stats.favouriteStats.genreFactors;

    SqlContext<bool> ctx(db, std::move(query));
    ctx.bindValue("author_id",author->id);
    auto keyConverter = interfaces::GenreConverter::Instance();
    QList<QString> keys = interfaces::GenreConverter::Instance().GetCodeGenres();
    ctx.ProcessKeys(keys, [&](const QString& key, auto& q){
        q.bindValue(keyConverter.ToDB(key).toStdString(), genreFactors[key]);
    });
    ctx.ExecAndCheck(true);
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> WriteAuthorFavouriteFandomStatistics(core::AuthorPtr author, sql::Database db)
{
    std::string query = "INSERT INTO AuthorFavouritesFandomRatioStatistics ("
                    "author_id, fandom_id, fandom_ratio, fic_count) "
                    "VALUES ("
                    ":author_id, :fandom_id, :fandom_ratio, :fic_count"
                    ")";

    SqlContext<bool> ctx(db, std::move(query));
    ctx.bindValue("author_id",author->id);

    ctx.ExecuteWithKeyListAndBindFunctor<QList,int>(author->stats.favouriteStats.fandomFactorsConverted.keys(), [&](auto&& key, auto& q){
        q.bindValue("fandom_id", key);
        q.bindValue("fandom_ratio", author->stats.favouriteStats.fandomFactorsConverted[key]);
        q.bindValue("fic_count", author->stats.favouriteStats.fandomsConverted[key]);
    });

    return std::move(ctx.result);
}


DiagnosticSQLResult<bool> WipeAuthorStatistics(core::AuthorPtr author, sql::Database db)
{
    SqlContext<bool> ctx(db);
    ctx.bindValue("author_id",author->id);
    ctx.ExecuteList({"delete from AuthorFavouritesGenreStatistics where author_id = :author_id",
                     "delete from AuthorFavouritesStatistics where author_id = :author_id",
                     "delete from AuthorFavouritesFandomRatioStatistics where author_id = :author_id"
                    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QList<int>> GetAllAuthorRecommendations(int id, sql::Database db)
{
    SqlContext<QList<int>> ctx(db);

    ctx.bindValue("id",id);
    ctx.FetchLargeSelectIntoList<int>("fic_id",
                                      "select fic_id from recommendations where recommender_id = :id",
                                      "select count(recommender_id) from recommendations where recommender_id = :id");
    return std::move(ctx.result);
}

DiagnosticSQLResult<QSet<int> > GetAllKnownSlashFics(sql::Database db) //todo wrong table
{
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("fic_id",
                                      "select fic_id from algopasses where keywords_pass_result = 1",
                                      "select count(fic_id) from algopasses where keywords_pass_result = 1");

    return std::move(ctx.result);
}


DiagnosticSQLResult<QSet<int> > GetAllKnownNotSlashFics(sql::Database db) //todo wrong table
{
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("fic_id",
                                      "select fic_id from algopasses where keywords_no = 1",
                                      "select count(fic_id) from algopasses where keywords_no = 1");

    return std::move(ctx.result);
}

DiagnosticSQLResult<QSet<int> > GetSingularFicsInLargeButSlashyLists(sql::Database db)
{
    std::string qs = "select fic_id from "
                         " ( "
                         " select fic_id, count(fic_id) as cnt from recommendations where recommender_id in (select author_id from AuthorFavouritesStatistics where slash_factor > 0.5 and slash_factor < 0.85  and favourites > 1000) and fic_id not in ( "
                         " select distinct fic_id from recommendations where recommender_id in (select author_id from AuthorFavouritesStatistics where slash_factor <= 0.5 or slash_factor > 0.85) ) group by fic_id "
                         " ) "
                         " where cnt = 1 ";
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("fic_id",std::move(qs));

    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<int, double> > GetDoubleValueHashForFics(QString fieldName, sql::Database db)
{
    SqlContext<QHash<int, double>> ctx(db);
    std::string qs = fmt::format("select id, {0} from fanfics order by id",fieldName.toStdString());
    ctx.FetchSelectIntoHash(std::move(qs), "id", fieldName.toStdString());
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<int, QString> >GetGenreForFics(sql::Database db)
{
    SqlContext<QHash<int, QString>> ctx(db);
    std::string qs = fmt::format("select id, {0} from fanfics order by id","genres");
    ctx.FetchSelectIntoHash(std::move(qs), "id", "genres");
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<int, int>> GetScoresForFics(sql::Database db)
{
    SqlContext<QHash<int, int>> ctx(db);
    std::string qs = "select fic_id, score from ficscores order by fic_id asc";
    ctx.FetchSelectIntoHash(std::move(qs), "fic_id", "score");
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<int, std::array<double, genreArraySize>>> GetGenreData(QString keyName, std::string&& query, sql::Database db)
{
    SqlContext<QHash<int, std::array<double, genreArraySize>>> ctx(db);
    ctx.FetchSelectFunctor(std::move(query), DATAQ{
                               std::size_t counter = 0;
                               auto key = q.value(keyName.toStdString()).toInt();
                               auto& dataElement = data[key];
                               dataElement.at(counter++) =q.value("General_").toDouble();
                               dataElement.at(counter++) =q.value("Humor").toDouble();
                               dataElement.at(counter++) =q.value("Poetry").toDouble();
                               dataElement.at(counter++) =q.value("Adventure").toDouble();
                               dataElement.at(counter++) =q.value("Mystery").toDouble();
                               dataElement.at(counter++) =q.value("Horror").toDouble();
                               dataElement.at(counter++) =q.value("Parody").toDouble();
                               dataElement.at(counter++) =q.value("Angst").toDouble();
                               dataElement.at(counter++) =q.value("Supernatural").toDouble();
                               dataElement.at(counter++) =q.value("Suspense").toDouble();
                               dataElement.at(counter++) =q.value("Romance").toDouble();
                               dataElement.at(counter++) =q.value("NoGenre").toDouble();
                               dataElement.at(counter++) =q.value("SciFi").toDouble();
                               dataElement.at(counter++) =q.value("Fantasy").toDouble();
                               dataElement.at(counter++) =q.value("Spiritual").toDouble();
                               dataElement.at(counter++) =q.value("Tragedy").toDouble();
                               dataElement.at(counter++) =q.value("Western").toDouble();
                               dataElement.at(counter++) =q.value("Crime").toDouble();
                               dataElement.at(counter++) =q.value("Family").toDouble();
                               dataElement.at(counter++) =q.value("HurtComfort").toDouble();
                               dataElement.at(counter++) =q.value("Friendship").toDouble();
                               dataElement.at(counter++) =q.value("Drama").toDouble();
                           });

    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<int, std::array<double, genreArraySize>>> GetListGenreData(sql::Database db)
{
    return GetGenreData("author_id", "select * from AuthorFavouritesGenreStatistics "
                                     " where author_id  in (select distinct recommender_id from recommendations) "
                                     " order by author_id asc", db);
}
DiagnosticSQLResult<QHash<int, std::array<double, genreArraySize> > > GetFullFicGenreData(sql::Database db)
{
    return GetGenreData("fic_id", "select * from FicGenreStatistics order by fic_id", db);
}

DiagnosticSQLResult<QHash<int, double> > GetFicGenreData(QString genre, QString cutoff, sql::Database db)
{
    SqlContext<QHash<int, double>> ctx(db);
    std::string qs = "select fic_id, {0} from FicGenreStatistics where {1} order by fic_id";
    qs = fmt::format(qs, genre.toStdString(),cutoff.toStdString());
    ctx.FetchSelectIntoHash(std::move(qs), "fic_id", genre.toStdString());
    return std::move(ctx.result);
}

DiagnosticSQLResult<QSet<int> > GetAllKnownFicIds(QString where, sql::Database db)
{
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("id",
                                      "select id from fanfics where " + where.toStdString(),
                                      "select count(*) from fanfics where " + where.toStdString());

    return std::move(ctx.result);
}
DiagnosticSQLResult<QSet<int>>  GetFicIDsWithUnsetAuthors(sql::Database db)
{
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("fic_id",
                                      "select distinct fic_id from fictags where fic_id not in (select distinct fic_id from ficauthors)");

    return std::move(ctx.result);
}

DiagnosticSQLResult<QVector<core::FicWeightPtr> > GetAllFicsWithEnoughFavesForWeights(int faves, sql::Database db)
{
    SqlContext<QVector<core::FicWeightPtr>> ctx(db);
    std::string qs = fmt::format("select id,Rated, author_id, complete, updated, fandom1,fandom2,favourites, published, updated,  genres, reviews, filter_pass_1, wordcount"
                         "  from fanfics where favourites > {0} order by id", faves);
    ctx.FetchSelectFunctor(std::move(qs), DATAQ{
                               auto fw = getFicWeightPtrFromQuery(q);
                               data.push_back(fw);
                           });
    return std::move(ctx.result);
}


DiagnosticSQLResult<QHash<int, core::AuthorFavFandomStatsPtr>> GetAuthorListFandomStatistics(QList<int> authors, sql::Database db)
{
    DiagnosticSQLResult<QHash<int, core::AuthorFavFandomStatsPtr>> result;
    result.data.reserve(authors.size());
    SqlContext<core::AuthorFavFandomStatsPtr> ctx(db);
    ctx.Prepare(" select fandom_id, fandom_ratio, fic_count from AuthorFavouritesFandomRatioStatistics where author_id = :author_id");
    for(auto author: authors)
    {
        ctx.bindValue("author_id", author);
        //qDebug() << "loading author: " << author;

        core::AuthorFavFandomStatsPtr fs(new core::AuthorFandomStatsForWeightCalc);

        fs->listId = author;
        ctx.FetchSelectFunctor(" select fandom_id, fandom_ratio, fic_count from AuthorFavouritesFandomRatioStatistics where author_id = :author_id",
                               DATAQN{
                               fs->fandomPresence[q.value("fandom_id").toInt()] = q.value("fandom_ratio").toDouble();
                               fs->fandomCounts[q.value("fandom_id").toInt()] = q.value("fic_count").toInt();
                           }, true);
        fs->fandomCount = fs->fandomPresence.size();
        ctx.result.data = fs;

        if(!ctx.result.success)
        {
            result.sqlError = ctx.result.sqlError;
            //qDebug() << "GetAuthorListFandomStatistics: " << ctx.result.oracleError;
            result.success = false;
            result.data.clear();
            break;
        }
        result.data[author] = ctx.result.data;
    }
    return result;
}

DiagnosticSQLResult<bool> FillRecommendationListWithData(int listId,
                                                         const QHash<int, int>& fics,
                                                         sql::Database db)
{
    std::string qs = "INSERT INTO RecommendationListData ("
                 "fic_id, list_id, match_count) "
                 "VALUES ("
                 ":fic_id, :list_id, :match_count)";

    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("list_id", listId);
    ctx.ExecuteWithArgsHash({"fic_id", "match_count"}, fics);
    return std::move(ctx.result);
}


DiagnosticSQLResult<bool> CreateSlashInfoPerFic(sql::Database db)
{
    SqlContext<bool> ctx(db, "insert into algopasses(fic_id) select id from fanfics");
    return ctx(true);
}

DiagnosticSQLResult<bool> WipeSlashMetainformation(sql::Database db)
{
    std::string qs = "update algopasses set "
                 " keywords_yes = 0, keywords_no = 0, keywords_pass_result = 0, "
                 " pass_1 = 0, pass_2 = 0,pass_3 = 0,pass_4 = 0,pass_5 = 0 ";

    return SqlContext<bool>(db, std::move(qs))();
}


DiagnosticSQLResult<bool> ProcessSlashFicsBasedOnWords(std::function<SlashPresence(QString, QString, QString)> func, sql::Database db)
{
    DiagnosticSQLResult<bool> result;
    result.success = false;

    CreateSlashInfoPerFic(db);
    qDebug() << QStringLiteral("finished creating records for fics");
    WipeSlashMetainformation(db);
    qDebug() << QStringLiteral("finished wiping slash info");

    QSet<int> slashFics;
    QSet<int> slashKeywords;
    QSet<int> notSlashFics;
    QString qs = QStringLiteral("select id, summary, characters, fandom from fanfics order by id asc");
    sql::Query q(db);
    q.prepare(qs.toStdString());
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    do
    {
        auto result = func(QString::fromStdString(q.value("summary").toString()),
                           QString::fromStdString(q.value("characters").toString()),
                           QString::fromStdString(q.value("fandom").toString()));
        if(result.containsSlash)
            slashKeywords.insert(q.value("id").toInt());
        if(result.IsSlash())
            slashFics.insert(q.value("id").toInt());
        if(result.containsNotSlash)
            notSlashFics.insert(q.value("id").toInt());


    }while(q.next());
    qDebug() << QStringLiteral("finished processing slash info");
    q.prepare("update algopasses set keywords_pass_result = 1 where fic_id = :id");
    QList<int> list = slashFics.values();
    int counter = 0;
    for(auto tempId: list)
    {
        counter++;
        q.bindValue("id", tempId);
        if(!result.ExecAndCheck(q))
        {
            qDebug() << QStringLiteral("failed to write slash");
            return result;
        }
    }
    q.prepare("update algopasses set keywords_no = 1 where fic_id = :id");
    list = notSlashFics.values();
    counter = 0;
    for(auto tempId: std::as_const(list))
    {
        counter++;
        q.bindValue("id", tempId);
        if(!result.ExecAndCheck(q))
        {
            qDebug() << "failed to write slash";
            return result;
        }
    }
    q.prepare("update algopasses set keywords_yes = 1 where fic_id = :id");
    list = slashKeywords.values();
    counter = 0;
    for(auto tempId: std::as_const(list))
    {
        counter++;
        q.bindValue("id", tempId);
        if(!result.ExecAndCheck(q))
        {
            qDebug() << "failed to write slash";
            return result;
        }
    }
    qDebug() << QStringLiteral("finished writing slash info into DB");

    result.success = true;
    return result;
}

DiagnosticSQLResult<bool> WipeAuthorStatisticsRecords(sql::Database db)
{
    std::string qs = "delete from AuthorFavouritesStatistics";
    return SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> CreateStatisticsRecordsForAuthors(sql::Database db)
{
    std::string qs = "insert into AuthorFavouritesStatistics(author_id, favourites) select r.id, (select count(*) from recommendations where recommender_id = r.id) from recommenders r";
    return SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> CalculateSlashStatisticsPercentages(QString usedField,  sql::Database db)
{
    std::string qs = "update AuthorFavouritesStatistics set slash_factor  = "
                         " cast( (select count (ff.fic_id) from (select fic_id from recommendations where recommender_id = author_id) rs left join  (select fic_id, {0} from algopasses where {0} = 1) ff on ff.fic_id  = rs.fic_id) as float) "
                         "/cast( (select count (ff.fic_id) from (select fic_id from recommendations where recommender_id = author_id) rs left join  (select fic_id, {0} from algopasses)              ff on ff.fic_id  = rs.fic_id) as float)";
    qs = fmt::format(qs, usedField.toStdString());
    return SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> AssignIterationOfSlash(QString iteration, sql::Database db)
{
    std::string qs = "update algopasses set {0} = 1 where keywords_pass_result = 1 or fic_id in "
                         "(select fic_id from recommendationlistdata where list_id in (select id from recommendationlists where name = 'SlashCleaned'))";
    qs = fmt::format(qs, iteration.toStdString());
    return SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> WriteFicRecommenderRelationsForRecList(int list_id, QHash<uint32_t, QVector<uint32_t> > relations, sql::Database db)
{
    DiagnosticSQLResult<bool> result;
    result.success = false;
    {
        std::string qs = "delete from RecommendersForFicAndList where list_id = :list_id";
        SqlContext<bool>(db, std::move(qs),BP1(list_id))();
    }

    {
        std::string qs = "insert into RecommendersForFicAndList(list_id, fic_id, author_id) values(:list_id, :fic_id, :author_id)";
        SqlContext<bool>ctx (db, std::move(qs));
        ctx.bindValue("list_id", list_id);
        auto fics = relations.keys();
        std::sort(fics.begin(), fics.end());
        for(auto& fic : fics){
            ctx.bindValue("fic_id", fic);
            for(auto& author : relations[fic]){
                ctx.bindValue("author_id", author);
                ctx();
                if(!ctx.result.success){
                    result = ctx.result;
                    return result;
                }
            }
        }
    }
    result.success = true;
    return result;
}

DiagnosticSQLResult<bool> WriteAuthorStatsForRecList(int list_id,
                                                            const QVector<core::AuthorResult>& authors,
                                                     sql::Database db){
    DiagnosticSQLResult<bool> result;
    result.success = false;
    {
        std::string qs = "delete from AuthorParamsForRecList where list_id = :list_id";
        SqlContext<bool>(db, std::move(qs),BP1(list_id))();
    }
    {
        std::string qs = "insert into AuthorParamsForRecList"
                             "(list_id, author_id, full_list_size, total_matches, negative_matches, match_category, "
                             "list_size_without_ignores, ratio_difference_on_neutral_mood, ratio_difference_on_touchy_mood) "
                             "values(:list_id, :author_id, :full_list_size, :total_matches, :negative_matches, :match_category,"
                             ":list_size_without_ignores, :ratio_difference_on_neutral_mood, :ratio_difference_on_touchy_mood)";
        SqlContext<bool> ctx(db, std::move(qs));
        ctx.bindValue("list_id", list_id);
        for(auto& author : authors){
            ctx.bindValue("author_id", author.id);
            ctx.bindValue("full_list_size", author.fullListSize);
            ctx.bindValue("total_matches", author.matches);
            ctx.bindValue("negative_matches", author.negativeMatches);
            ctx.bindValue("match_category", static_cast<int>(author.authorMatchCloseness));
            ctx.bindValue("list_size_without_ignores", author.sizeAfterIgnore);
            ctx.bindValue("ratio_difference_on_neutral_mood", author.listDiff.neutralDifference.value_or(-1));
            ctx.bindValue("ratio_difference_on_touchy_mood", author.listDiff.touchyDifference.value_or(-1));
            ctx();
            if(!ctx.result.success){
                result = ctx.result;
                return result;
            }
        }

    }
    result.success = true;
    return result;
}



DiagnosticSQLResult<bool> PerformGenreAssignment(sql::Database db)
{
    thread_local QHash<QString, int> result;
    result["General_"] = 0;
    result["Humor"] = 1;
    result["Poetry"] = 0;
    result["Adventure"] = 0;
    result["Mystery"] = 0;
    result["Horror"] = 0;
    result["Parody"] = 1;
    result["Angst"] = -1;
    result["Supernatural"] = 0;
    result["Suspense"] = 0;
    result["Romance"] = 0;
    result["NoGenre"] = 0;
    result["SciFi"] = 0;
    result["Fantasy"] = 0;
    result["Spiritual"] = 0;
    result["Tragedy"] = -1;
    result["Drama"] = -1;
    result["Western"] = 0;
    result["Crime"] = 0;
    result["Family"] = 0;
    result["HurtComfort"] = -1;
    result["Friendship"] = 1;

    std::string qs = "update ficgenrestatistics set {0} =  CASE WHEN  (select count(distinct recommender_id) from recommendations r where r.fic_id = ficgenrestatistics .fic_id) >= 5 "
                         " THEN (select avg({0}) from AuthorFavouritesGenreStatistics afgs where afgs.author_id in (select distinct recommender_id from recommendations r where r.fic_id = ficgenrestatistics .fic_id))  "
                         " ELSE 0 END ";
    std::vector<std::string> list;
    for(auto i = result.cbegin(); i != result.cend(); i++)
        list.push_back(i.key().toStdString());
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.ExecuteWithArgsSubstitution(std::move(list));
    return std::move(ctx.result);

}

DiagnosticSQLResult<bool> EnsureUUIDForUserDatabase(QUuid id, sql::Database db)
{
    DiagnosticSQLResult<bool> result;

    std::string qs = "select count(*) as cnt from user_settings where name = 'db_uuid'";
    SqlContext<int> ctx(db, std::move(qs));
    ctx.FetchSingleValue<int>("cnt", 0);
    if(!ctx.Success())
    {
        result.sqlError = ctx.result.sqlError;
        return result;
    }
    result.success = true;
    if(ctx.result.data > 0)
    {

        return result;
    }
    qs = "insert into user_settings(name, value) values('db_uuid', '{0}')";
    qs = fmt::format(qs,id.toString().toStdString());
    ctx.ReplaceQuery(std::move(qs));

    ctx();
    if(!ctx.Success())
    {
        result.sqlError = ctx.result.sqlError;
        result.success = false;
        return result;
    }
    return result;
}

DiagnosticSQLResult<bool> FillFicDataForList(int listId,
                                             const QVector<int> & fics,
                                             const QVector<int> & matchCounts,
                                             const QSet<int> &origins,
                                             sql::Database db)
{
    std::string qs = "insert into RecommendationListData(list_id, fic_id, match_count, is_origin) values(:listId, :ficId, :matchCount, :is_origin)";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("listId", listId);
    for(int i = 0; i < fics.size(); i++)
    {
        ctx.bindValue("ficId", fics.at(i));
        ctx.bindValue("matchCount", matchCounts.at(i));
        bool isOrigin = origins.contains(fics.at(i));
        ctx.bindValue("is_origin", isOrigin);
        if(!ctx.ExecAndCheck())
        {
            ctx.result.success = false;
            break;
        }
    }
    return std::move(ctx.result);
}

//alter table RecommendationListData add column votes_common integer default 0;
//alter table RecommendationListData add column votes_rare integer default 0;
//alter table RecommendationListData add column votes_unique integer default 0;

//alter table RecommendationListData add column value_common integer default 0;
//alter table RecommendationListData add column value_rare integer default 0;
//alter table RecommendationListData add column value_unique integer default 0;

//alter table RecommendationListData add column breakdown_available integer default 0;

static QHash<int, int> RecastFicScoresIntoPedestalSet(QSharedPointer<core::RecommendationListFicData> ficData){
    int position = 1;
    QSet<int> scoresSet;
    QList<int> scoresList;
    QHash<int, int> scorePositions;
    //QVector ficsCopy = ficData->fics;
    for(int i = 0; i < ficData->fics.size(); i++)
        scoresSet.insert(ficData->metascores[i]);

    scoresList = scoresSet.values();
    std::sort(scoresList.begin(), scoresList.end());
    std::reverse(scoresList.begin(), scoresList.end());

    for(int i = 0; i < scoresList.size(); i++)
        scorePositions[scoresList.at(i)]=position++;
    return scorePositions;
}

static QHash<int, int> CreateFicPositions(QSharedPointer<core::RecommendationListFicData> ficData){

    // first we need to fill the hash of scores per fic
    QHash<int, int> scores;
    QHash<int, int> positions;
    QVector ficsCopy = ficData->fics;
    for(int i = 0; i < ficData->fics.size(); i++)
        scores[ficData->fics[i]] = ficData->metascores.at(i);

    std::sort(ficsCopy.begin(), ficsCopy.end(), [&](const int& fic1, const int& fic2){
        return scores[fic1] > scores[fic2] ;
    });
    for(int i = 0; i < ficData->fics.size(); i++){
        positions[ficsCopy[i]] = i+1;
    }
    return positions;
}


DiagnosticSQLResult<bool> FillFicDataForList(QSharedPointer<core::RecommendationList> list,
                                             sql::Database db)
{
    std::string qs = "insert into RecommendationListData(list_id, fic_id, position, pedestal, "
                         "match_count, no_trash_score, is_origin, "
                         "breakdown_available,"
                         "votes_common, votes_uncommon, votes_rare, votes_unique, "
                         "value_common, value_uncommon, value_rare, value_unique, purged) "
                         "values(:listId, :ficId, :position, :pedestal, :matchCount, :no_trash_score, :is_origin,"
                         ":breakdown_available,"
                         ":votes_common, :votes_uncommon, :votes_rare, :votes_unique, "
                         ":value_common, :value_uncommon, :value_rare, :value_unique, :purged)";

    SqlContext<bool> ctx(db, std::move(qs));
    //QLOG_INFO() << "Origins list: " << list->ficData.sourceFics;

    qDebug() << "Creating new vote breakdown records for list with id: " << list->id;
    ctx.bindValue("listId", list->id);
    auto scorePedestalPositions = RecastFicScoresIntoPedestalSet(list->ficData);
    auto ficPositionsInList = CreateFicPositions(list->ficData);

    for(int i = 0; i < list->ficData->fics.size(); i++)
    {

        int ficId = list->ficData->fics.at(i);

        ctx.bindValue("ficId", ficId);
        ctx.bindValue("position", ficPositionsInList[ficId]);
        ctx.bindValue("pedestal", scorePedestalPositions[list->ficData->metascores.at(i)]);

        ctx.bindValue("matchCount", list->ficData->metascores.at(i));
        ctx.bindValue("no_trash_score", list->ficData->noTrashScores.at(i));
        bool isOrigin = list->ficData->sourceFics.contains(list->ficData->fics.at(i));
        //QLOG_INFO() << "Writing fic: " << ficId << " isOrigin: " << isOrigin;
        ctx.bindValue("is_origin", isOrigin);
        ctx.bindValue("breakdown_available", true);

        ctx.bindValue("votes_common", list->ficData->breakdowns[ficId].authorTypes[core::AuthorWeightingResult::EAuthorType::common]);
        ctx.bindValue("votes_uncommon", list->ficData->breakdowns[ficId].authorTypes[core::AuthorWeightingResult::EAuthorType::uncommon]);
        ctx.bindValue("votes_rare", list->ficData->breakdowns[ficId].authorTypes[core::AuthorWeightingResult::EAuthorType::rare]);
        ctx.bindValue("votes_unique", list->ficData->breakdowns[ficId].authorTypes[core::AuthorWeightingResult::EAuthorType::unique]);

        ctx.bindValue("value_common", list->ficData->breakdowns[ficId].authorTypeVotes[core::AuthorWeightingResult::EAuthorType::common]);
        ctx.bindValue("value_uncommon", list->ficData->breakdowns[ficId].authorTypeVotes[core::AuthorWeightingResult::EAuthorType::uncommon]);
        ctx.bindValue("value_rare", list->ficData->breakdowns[ficId].authorTypeVotes[core::AuthorWeightingResult::EAuthorType::rare]);
        ctx.bindValue("value_unique", list->ficData->breakdowns[ficId].authorTypeVotes[core::AuthorWeightingResult::EAuthorType::unique]);
        ctx.bindValue("purged", list->ficData->purges.at(i));
        if(!ctx.ExecAndCheck())
        {
            ctx.result.success = false;
            break;
        }
    }
    return std::move(ctx.result);
}

DiagnosticSQLResult<QString> GetUserToken(sql::Database db)
{
    std::string qs = "Select value from user_settings where name = :db_uuid";

    SqlContext<QString> ctx(db, std::move(qs));
    ctx.bindValue("db_uuid", "db_uuid");
    ctx.FetchSingleValue<QString>("value", "");
    return std::move(ctx.result);
}

DiagnosticSQLResult<genre_stats::FicGenreData> GetRealGenresForFic(int ficId, sql::Database db)
{
    std::string query = " with count_per_fic(fic_id, ffn_id, title, genres, total, HumorComposite, Flirty, Pure_drama, Pure_Romance,"
                    " Hurty, Bondy, NeutralComposite,DramaComposite, NeutralSingle) as ("
                    " with"
                    " fic_ids as (select {0} as fid),"

                    " total as ("
                    " select fic_id, count(distinct recommender_id) as total from recommendations, fic_ids "
                    " on fic_ids.fid = recommendations.fic_id "
                    " and recommender_id in (select author_id from AuthorFavouritesStatistics where favourites > 30)"
                    " group by fic_id"
                    " )"
                    " select fanfics.id as id, fanfics.ffn_id as ffn_id,  fanfics.title as title, fanfics.genres as genres, total.total, "

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Funny > 0.3 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as HumorComposite,"

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Flirty > 0.5 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as Flirty,"

                    " (select count(distinct author_id) from  AuthorFavouritesGenreStatistics where (drama-romance) > 0.05 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as Pure_Drama,"

                    " (select count(distinct author_id) from  AuthorFavouritesGenreStatistics where (romance-drama) > 0.8 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as Pure_Romance, "

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Hurty > 0.15 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as Hurty,"

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Bondy > 0.3 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as Bondy,"

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Neutral > 0.3 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as NeutralComposite,"

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Dramatic > 0.3 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as DramaComposite,"


                    " (select count(distinct author_id) from  AuthorFavouritesGenreStatistics where Adventure > 0.3 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as NeutralSingle"

                    " from fanfics, total on total.fic_id = fanfics.id "
                    " )"
                    " select  fic_id, total, title, genres,"

                    " cast(HumorComposite as float)/cast(total as float) as Div_HumorComposite,"
                    " cast(Flirty as float)/cast(total as float) as Div_Flirty,"
                    " cast(DramaComposite as float)/cast(total as float) as Div_Dramatic,"
                    " cast(Pure_drama as float)/cast(total as float) as Div_PureDrama,"
                    " cast(Pure_Romance as float)/cast(total as float) as Div_PureRomance,"
                    " cast(Bondy as float)/cast(total as float) as Div_Bondy,"
                    " cast(Hurty as float)/cast(total as float) as Div_Hurty,"
                    " cast(NeutralComposite as float)/cast(total as float) as Div_NeutralComposite,"
                    " cast(NeutralSingle as float)/cast(total as float) as Div_NeutralSingle,"

                    " ffn_id "

                    " from count_per_fic";
    query= fmt::format(query, ficId);
    SqlContext<genre_stats::FicGenreData> ctx(db, std::move(query));
    auto genreConverter = interfaces::GenreConverter::Instance();

    ctx.ForEachInSelect([&](sql::Query& q){
        ctx.result.data.originalGenreString = QString::fromStdString(q.value("genres").toString());
        ctx.result.data.originalGenres = genreConverter.GetFFNGenreList(QString::fromStdString(q.value("genres").toString()));

        ctx.result.data.ficId = ficId;
        ctx.result.data.ffnId = q.value("ffn_id").toInt();
        ctx.result.data.totalLists = q.value("total").toInt();

        ctx.result.data.strengthHumor = q.value("Div_HumorComposite").toFloat();
        ctx.result.data.strengthRomance = q.value("Div_Flirty").toFloat();
        ctx.result.data.strengthDrama= q.value("Div_Dramatic").toFloat();
        ctx.result.data.strengthBonds= q.value("Div_Bondy").toFloat();
        ctx.result.data.strengthHurtComfort= q.value("Div_Hurty").toFloat();
        ctx.result.data.strengthNeutralComposite= q.value("Div_NeutralComposite").toFloat();
        ctx.result.data.strengthNeutralAdventure= q.value("Div_NeutralSingle").toFloat();
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QHash<uint32_t, genre_stats::ListMoodData>> GetMoodDataForLists(sql::Database db)
{
    std::string query = " select * from AuthorMoodStatistics "
                    " where author_id in (select distinct recommender_id from recommendations) "
                    " order by author_id asc ";

    SqlContext<QHash<uint32_t, genre_stats::ListMoodData>> ctx(db, std::move(query));
    auto genreConverter = interfaces::GenreConverter::Instance();
    genre_stats::ListMoodData tmp;
    ctx.ForEachInSelect([&](sql::Query& q){
        tmp.listId = q.value("author_id").toInt();

        tmp.strengthBondy = q.value("bondy").toFloat();
        tmp.strengthDramatic= q.value("dramatic").toFloat();
        tmp.strengthFlirty= q.value("flirty").toFloat();
        tmp.strengthFunny= q.value("funny").toFloat();
        tmp.strengthHurty= q.value("hurty").toFloat();
        tmp.strengthNeutral= q.value("neutral").toFloat();

        tmp.strengthNonBondy= q.value("nonbondy").toFloat();
        tmp.strengthNonDramatic= q.value("NonDramatic").toFloat();
        tmp.strengthNonFlirty= q.value("NonFlirty").toFloat();
        tmp.strengthNonFunny= q.value("NonFunny").toFloat();
        tmp.strengthNonHurty= q.value("NonHurty").toFloat();
        tmp.strengthNonNeutral= q.value("NonNeutral").toFloat();
        tmp.strengthNonShocky= q.value("NonShocky").toFloat();
        tmp.strengthNone= q.value("None").toFloat();
        tmp.strengthOther= q.value("Other").toFloat();


        ctx.result.data.insert(tmp.listId, tmp);
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<QVector<genre_stats::FicGenreData>> GetGenreDataForQueuedFics(sql::Database db)
{
    std::string query = " with count_per_fic(fic_id, ffn_id, title, genres, total, HumorComposite, Flirty, Pure_drama, Pure_Romance,"
                    " Hurty, Bondy, NeutralComposite,DramaComposite, NeutralSingle) as ("
                    " with"
                    " fic_ids as (select id as fid from fanfics where queued_for_action = 1),"

                    " total as ("
                    " select fic_id, count(distinct recommender_id) as total from recommendations, fic_ids "
                    " on fic_ids.fid = recommendations.fic_id "
                    " and recommender_id in (select author_id from AuthorFavouritesStatistics where favourites > 30)"
                    " group by fic_id"
                    " )"
                    " select fanfics.id as id, fanfics.ffn_id as ffn_id,  fanfics.title as title, fanfics.genres as genres, total.total, "

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Funny > 0.3 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as HumorComposite,"

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Flirty > 0.5 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as Flirty,"

                    " (select count(distinct author_id) from  AuthorFavouritesGenreStatistics where (drama-romance) > 0.05 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as Pure_Drama,"

                    " (select count(distinct author_id) from  AuthorFavouritesGenreStatistics where (romance-drama) > 0.8 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as Pure_Romance, "

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Hurty > 0.15 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as Hurty,"

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Bondy > 0.3 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as Bondy,"

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Neutral > 0.3 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as NeutralComposite,"

                    " (select count(distinct author_id) from  AuthorMoodStatistics where Dramatic > 0.3 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as DramaComposite,"


                    " (select count(distinct author_id) from  AuthorFavouritesGenreStatistics where Adventure > 0.3 and author_id in "
                    " (select distinct recommender_id from recommendations where fanfics.id = fic_id)) as NeutralSingle"

                    " from fanfics, total on total.fic_id = fanfics.id "
                    " )"
                    " select  fic_id, total, title, genres,"

                    " cast(HumorComposite as float)/cast(total as float) as Div_HumorComposite,"
                    " cast(Flirty as float)/cast(total as float) as Div_Flirty,"
                    " cast(DramaComposite as float)/cast(total as float) as Div_Dramatic,"
                    " cast(Pure_drama as float)/cast(total as float) as Div_PureDrama,"
                    " cast(Pure_Romance as float)/cast(total as float) as Div_PureRomance,"
                    " cast(Bondy as float)/cast(total as float) as Div_Bondy,"
                    " cast(Hurty as float)/cast(total as float) as Div_Hurty,"
                    " cast(NeutralComposite as float)/cast(total as float) as Div_NeutralComposite,"
                    " cast(NeutralSingle as float)/cast(total as float) as Div_NeutralSingle,"

                    " ffn_id "

                    " from count_per_fic";

    SqlContext<QVector<genre_stats::FicGenreData>> ctx(db, std::move(query));
    auto genreConverter = interfaces::GenreConverter::Instance();
    genre_stats::FicGenreData tmp;
    ctx.ForEachInSelect([&](sql::Query& q){
        tmp.originalGenreString = QString::fromStdString(q.value("genres").toString());
        tmp.originalGenres = genreConverter.GetFFNGenreList(QString::fromStdString(q.value("genres").toString()));

        tmp.ficId = q.value("fic_id").toInt();
        tmp.ffnId = q.value("ffn_id").toInt();
        tmp.totalLists = q.value("total").toInt();

        tmp.strengthHumor = q.value("Div_HumorComposite").toFloat();
        tmp.strengthRomance = q.value("Div_Flirty").toFloat();
        tmp.strengthDrama= q.value("Div_Dramatic").toFloat();
        tmp.strengthBonds= q.value("Div_Bondy").toFloat();
        tmp.strengthHurtComfort= q.value("Div_Hurty").toFloat();
        tmp.strengthNeutralComposite= q.value("Div_NeutralComposite").toFloat();
        tmp.strengthNeutralAdventure= q.value("Div_NeutralSingle").toFloat();
        ctx.result.data.push_back(tmp);
    });
    return std::move(ctx.result);
}

DiagnosticSQLResult<bool> QueueFicsForGenreDetection(int minAuthorRecs, int minFoundLists, int minFaves, sql::Database db)
{
    std::string qs = " with "
                         " min_recs(val) as (select :minAuthorRecs), "
                         " min_filtered_lists(val) as (select :minFoundLists), "
                         " filtered_recommenders(filtered_rec) as( "
                         " select author_id from AuthorFavouritesStatistics where favourites >= (select val from min_recs)  "
                         " ), "
                         " to_update(fic_id) as "
                         " ( "
                         "  select fic_id from ( "
                         "  select fic_id, count(fic_id) as count_rec from recommendations where recommender_id in filtered_recommenders group by fic_id "
                         "  ) fin where count_rec >= (select val from min_filtered_lists) "
                         " ) "
                         " update fanfics set queued_for_action = 1 where id in to_update and favourites > {0} ";
    qs=fmt::format(qs, minFaves);

    return SqlContext<bool> (db, std::move(qs),BP2(minAuthorRecs,minFoundLists))();
}

DiagnosticSQLResult<bool> PassScoresToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget)
{
    std::vector<std::string> keyList = {"fic_id", "score", "updated"};
    std::vector<std::string> keyListCopy = {"fic_id", "score", "updated"};
    std::string insertQS = "insert into FicScores(fic_id, score , updated) values(:fic_id, :score , :updated)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from FicScores", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassSnoozesToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget)
{
    std::vector<std::string> keyList = {"fic_id", "snooze_added", "snoozed_at_chapter",
                           "snoozed_till_chapter", "snoozed_until_finished", "expired"};
    std::vector<std::string> keyListCopy = {"fic_id", "snooze_added", "snoozed_at_chapter",
                           "snoozed_till_chapter", "snoozed_until_finished", "expired"};
    std::string insertQS = "insert into FicSnoozes(fic_id, snooze_added, snoozed_at_chapter, snoozed_till_chapter,snoozed_until_finished,expired) "
                               " values(:fic_id, :snooze_added, :snoozed_at_chapter, :snoozed_till_chapter,:snoozed_until_finished,:expired)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from FicSnoozes", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassFicTagsToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget)
{
    std::vector<std::string> keyList = {"fic_id", "ffn_id", "ao3_id", "sb_id", "sv_id", "tag", "added"};
    std::vector<std::string> keyListCopy = {"fic_id", "ffn_id", "ao3_id", "sb_id", "sv_id", "tag", "added"};
    std::string insertQS = "insert into FicTags(fic_id, ffn_id, ao3_id, sb_id,sv_id,tag, added) "
                               " values(:fic_id, :ffn_id, :ao3_id, :sb_id,:sv_id,:tag, :added)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from FicTags", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassFicNotesToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget)
{
    std::vector<std::string>  keyList = {"fic_id", "note_content", "updated"};
    std::vector<std::string>  keyListCopy  = {"fic_id", "note_content", "updated"};
    std::string insertQS = "insert into ficnotes(fic_id, note_content , updated) values(:fic_id, :note_content , :updated)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from ficnotes", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassTagSetToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget)
{
    std::vector<std::string> keyList = {"id", "tag"};
    std::vector<std::string> keyListCopy = {"id", "tag"};
    std::string insertQS = "insert into Tags(id, tag) values(:id, :tag)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from Tags", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassRecentFandomsToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget)
{
    std::vector<std::string> keyList = {"fandom", "seq_num"};
    std::vector<std::string> keyListCopy = {"fandom", "seq_num"};
    std::string insertQS = "insert into recent_fandoms(fandom, seq_num) values(:fandom, :seq_num)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from recent_fandoms", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassIgnoredFandomsToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget)
{
    std::vector<std::string> keyList = {"fandom_id", "including_crossovers"};
    std::vector<std::string> keyListCopy = {"fandom_id", "including_crossovers"};
    std::string insertQS = "insert into ignored_fandoms(fandom_id, including_crossovers) values(:fandom_id, :including_crossovers)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from ignored_fandoms", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}


DiagnosticSQLResult<bool> PassFandomListSetToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget)
{
    // need to delete default data from instantiated backup DB so that constraint doesn't fail
    {
        std::string qs = "delete from fandom_lists";
        SqlContext<bool> ctx(dbTarget, std::move(qs));
        ctx();
    }
    std::vector<std::string> keyList = {"id", "name","is_enabled", "is_default","is_expanded", "ui_index"};
    std::vector<std::string> keyListCopy = {"id", "name","is_enabled", "is_default","is_expanded", "ui_index"};
    std::string insertQS = "insert into fandom_lists"
                           "(id, name,is_enabled, is_default,is_expanded, ui_index) "
                           "values(:id, :name,:is_enabled, :is_default,:is_expanded, :ui_index)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from fandom_lists", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}


DiagnosticSQLResult<bool> PassFandomListDataToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget)
{
    std::vector<std::string> keyList = {"list_id", "fandom_id","fandom_name", "enabled_state","inclusion_mode", "crossover_mode","ui_index"};
    std::vector<std::string> keyListCopy = {"list_id", "fandom_id","fandom_name", "enabled_state","inclusion_mode", "crossover_mode","ui_index"};
    std::string insertQS = "insert into fandom_list_data(list_id,fandom_id,fandom_name,enabled_state,inclusion_mode,crossover_mode,ui_index) "
                           " values(:list_id,:fandom_id,:fandom_name,:enabled_state,:inclusion_mode,:crossover_mode,:ui_index)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from fandom_list_data", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();

}

DiagnosticSQLResult<bool> PassClientDataToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget)
{
    {
        std::string qs = "delete from user_settings";
        SqlContext<bool> (dbTarget, std::move(qs))();
    }
    std::vector<std::string> keyList = {"name", "value"};
    std::vector<std::string> keyListCopy = {"name", "value"};
    std::string insertQS = "insert into user_settings(name, value) values(:name, :value)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from user_settings", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassReadingDataToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget)
{
    std::vector<std::string> keyList = {"fic_id", "at_chapter"};
    std::vector<std::string> keyListCopy = {"fic_id", "at_chapter"};
    std::string insertQS = "insert into FicReadingTracker(fic_id, at_chapter) values(:name, :at_chapter)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from FicReadingTracker", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<DBVerificationResult> VerifyDatabaseIntegrity(sql::Database db)
{
    DiagnosticSQLResult<DBVerificationResult> result;
    std::string qs = "pragma quick_check;";
    SqlContext<QStringList> ctx(db);
    ctx.FetchLargeSelectIntoListWithoutSize<QString>("quick_check", std::move(qs));
    if(ctx.result.data.size() == 0 || ctx.result.data.at(0) != "ok")
    {
        result.success = false;
        result.data.data = ctx.result.data;
    }
    else
        result.success = true;
    return result;
}














}




