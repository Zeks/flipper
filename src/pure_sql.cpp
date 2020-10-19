/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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
#include "EGenres.h"
#include "include/in_tag_accessor.h"
#include "fmt/format.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVector>
#include <QVariant>
#include <QDebug>
#include <algorithm>
#include<QSqlDriver>
#include<QSqlDatabase>
namespace database {
namespace puresql{

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

static DiagnosticSQLResult<FicIdHash> GetGlobalIDHash(QSqlDatabase db, QString where)
{
    std::string qs = "select id, ffn_id, ao3_id, sb_id, sv_id from fanfics ";
    if(!where.isEmpty())
        qs+=where.toStdString();

    SqlContext<FicIdHash> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data.ids[QStringLiteral("ffn")][q.value(QStringLiteral("ffn_id")).toInt()] = q.value(QStringLiteral("id")).toInt();
        ctx.result.data.ids[QStringLiteral("sb")][q.value(QStringLiteral("sb_id")).toInt()] = q.value(QStringLiteral("id")).toInt();
        ctx.result.data.ids[QStringLiteral("sv")][q.value(QStringLiteral("sv_id")).toInt()] = q.value(QStringLiteral("id")).toInt();
        ctx.result.data.ids[QStringLiteral("ao3")][q.value(QStringLiteral("ao3_id")).toInt()] = q.value(QStringLiteral("id")).toInt();
        FanficIdRecord rec;
        rec.ids[QStringLiteral("ffn")] = q.value(QStringLiteral("ffn_id")).toInt();
        rec.ids[QStringLiteral("sb")] = q.value(QStringLiteral("sb_id")).toInt();
        rec.ids[QStringLiteral("sv")] = q.value(QStringLiteral("sv_id")).toInt();
        rec.ids[QStringLiteral("ao3")] = q.value(QStringLiteral("ao3_id")).toInt();
        ctx.result.data.records[q.value(QStringLiteral("id")).toInt()] = rec;
    });
    return ctx.result;

}

bool CheckExecution(QSqlQuery& q)
{
    if(q.lastError().isValid())
    {
        qDebug() << "SQLERROR: "<< q.lastError();
        qDebug() << q.lastQuery();
        return false;
    }
    return true;
}


#define SQLPARAMS
#define CTXA [&](auto ctx)
DiagnosticSQLResult<bool> SetFandomTracked(int id, bool tracked,  QSqlDatabase db)
{
    return SqlContext<bool> (db,
                             " UPDATE fandomindex SET tracked = :tracked where id = :id",
                             SQLPARAMS{{"tracked",tracked ? "1" : "0"},
                                       {"id", id}})();
}

DiagnosticSQLResult<bool> CalculateFandomsFicCounts(QSqlDatabase db)
{
    std::string qs = "update fandomindex set fic_count = (select count(fic_id) from ficfandoms where fandom_id = fandoms.id)";
    return SqlContext<bool> (db, std::move(qs))();
}

DiagnosticSQLResult<bool> UpdateFandomStats(int fandomId, QSqlDatabase db)
{
    std::string qs = "update fandomsources set fic_count = "
                         " (select count(fic_id) from ficfandoms where fandom_id = :fandom_id)"
                         //! todo
                         //" average_faves_top_3 = (select sum(favourites)/3 from fanfics f where f.fandom = fandoms.fandom and f.id "
                         //" in (select id from fanfics where fanfics.fandom = fandoms.fandom order by favourites desc limit 3))"
                         " where fandoms.id = :fandom_id";

    SqlContext<bool> ctx(db, std::move(qs), {{"fandom_id",fandomId}});
    if(fandomId == -1)
        return ctx.result;
    return ctx();
}

DiagnosticSQLResult<bool> WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom, QSqlDatabase db)
{
    auto result = Internal::WriteMaxUpdateDateForFandom(fandom, "having count(*) = 1", db,
                                                        [](QSharedPointer<core::Fandom> f, QDateTime dt){
            f->lastUpdateDate = dt.date();
});
    return result;
}



DiagnosticSQLResult<bool>  Internal::WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom,
                                                                 QString condition,
                                                                 QSqlDatabase db,
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
DiagnosticSQLResult<QStringList> GetFandomListFromDB(QSqlDatabase db)
{
    std::string qs = "select name from fandomindex";

    SqlContext<QStringList> ctx(db);
    ctx.result.data.push_back("");
    ctx.FetchLargeSelectIntoList<QString>("name", std::move(qs));
    return ctx.result;
}

DiagnosticSQLResult<bool> AssignTagToFandom(QString tag, int fandom_id, QSqlDatabase db, bool includeCrossovers)
{
    std::string qs = "INSERT INTO FicTags(fic_id, tag) SELECT fic_id, '{0}' as tag from FicFandoms f WHERE fandom_id = :fandom_id "
                 " and NOT EXISTS(SELECT 1 FROM FicTags WHERE fic_id = f.fic_id and tag = '{0}')";
    if(!includeCrossovers)
        qs+=" and (select count(distinct fandom_id) from ficfandoms where fic_id = f.fic_id) = 1";
    qs=fmt::format(qs,tag.toStdString());

    SqlContext<bool> ctx(db, std::move(qs), {{"fandom_id", fandom_id}});
    ctx.ExecAndCheck(true);
    return ctx.result;
}



DiagnosticSQLResult<bool> AssignTagToFanfic(QString tag, int fic_id, QSqlDatabase db)
{
    std::string qs = "INSERT INTO FicTags(fic_id, tag, added) values(:fic_id, :tag, date('now'))";
    SqlContext<bool> ctx(db, std::move(qs), {{"tag", tag},{"fic_id", fic_id}});
    ctx.ExecAndCheck(true);
    //    ctx.ReplaceQuery("update fanfics set hidden = 1 where id = :fic_id";
    //    ctx.bindValue("fic_id", fic_id);
    //    ctx.ExecAndCheck(true);
    return ctx.result;
}


DiagnosticSQLResult<bool> RemoveTagFromFanfic(QString tag, int fic_id, QSqlDatabase db)
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
    return ctx.result;
}

DiagnosticSQLResult<bool> AssignSlashToFanfic(int fic_id, int source, QSqlDatabase db)
{
    std::string qs = "update fanfics set slash_probability = 1, slash_source = :source where id = :fic_id";
    return SqlContext<bool> (db, std::move(qs), {{"source", source},{"fic_id", fic_id}})();
}

DiagnosticSQLResult<bool> AssignQueuedToFanfic(int fic_id, QSqlDatabase db)
{
    std::string qs = "update fanfics set queued_for_action = 1 where id = :fic_id";
    return SqlContext<bool> (db, std::move(qs), {{"fic_id", fic_id}})();
}

DiagnosticSQLResult<bool> AssignChapterToFanfic(int fic_id, int chapter, QSqlDatabase db)
{
    std::string qs = "INSERT INTO FicReadingTracker(fic_id, at_chapter) values(:fic_id, :at_chapter) "
                 "on conflict (fic_id) do update set at_chapter = :at_chapter_ where fic_id = :fic_id_";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("fic_id", fic_id);
    ctx.bindValue("at_chapter", chapter);
    ctx.bindValue("at_chapter_", chapter);
    ctx.bindValue("fic_id_", fic_id);
    ctx.ExecAndCheck(true);
    return ctx.result;
}
DiagnosticSQLResult<bool> AssignScoreToFanfic(int score, int fic_id, QSqlDatabase db)
{
    if(score > 0)
    {
        std::string qs = "INSERT INTO FicSCores(fic_id, score) values(:fic_id, :score) on conflict (fic_id) do update set score = :score where fic_id = :fic_id";
        SqlContext<bool> ctx(db, std::move(qs), BP4(fic_id, score, score, fic_id));
        ctx.ExecAndCheck(true);
        return ctx.result;
    }
    else {
        std::string qs = " delete from FicScores where fic_id  = :fic_id";
        return SqlContext<bool>(db, std::move(qs), BP1(fic_id))();
    }
}

DiagnosticSQLResult<int> GetLastFandomID(QSqlDatabase db){
    std::string qs = "Select max(id) as maxid from fandomindex";

    SqlContext<int> ctx(db);
    //qDebug() << "Db open: " << db.isOpen() << " " << db.connectionName();
    ctx.FetchSingleValue<int>("maxid", -1, true, std::move(qs));
    return ctx.result;
}

DiagnosticSQLResult<bool> WriteFandomUrls(core::FandomPtr fandom, QSqlDatabase db)
{
    std::string qs = "insert into fandomurls(global_id, url, website, custom) values(:id, :url, :website, :custom)";

    SqlContext<bool> ctx(db, std::move(qs));
    if(fandom->urls.size() == 0)
        fandom->urls.push_back(core::Url(QStringLiteral(""), QStringLiteral("ffn")));
    ctx.ExecuteWithKeyListAndBindFunctor<core::Url>(fandom->urls, [&](core::Url& url, QSqlQuery& q){
        q.bindValue(QStringLiteral(":id"), fandom->id);
        q.bindValue(QStringLiteral(":url"), url.GetUrl());
        q.bindValue(QStringLiteral(":website"), fandom->source);
        q.bindValue(QStringLiteral(":custom"), fandom->section);
    }, true);
    return ctx.result;
}

DiagnosticSQLResult<bool> CreateFandomInDatabase(core::FandomPtr fandom, QSqlDatabase db, bool writeUrls, bool useSuppliedIds)
{
    SqlContext<bool> ctx(db);


    int newFandomId;
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
        return ctx.result;

    std::string qs = "insert into fandomindex(id, name) "
                         " values(:id, :name)";
    ctx.ReplaceQuery(std::move(qs));
    ctx.bindValue("name", fandom->GetName());
    ctx.bindValue("id", newFandomId);
    //qDebug() << db.isOpen();
    if(!ctx.ExecAndCheck())
        return ctx.result;

    qDebug() << "new fandom: " << fandom->GetName();

    fandom->id = newFandomId;
    if(writeUrls)
        ctx.result.success = WriteFandomUrls(fandom, db).success;
    return ctx.result;

}
DiagnosticSQLResult<int> GetFicIdByAuthorAndName(QString author, QString title, QSqlDatabase db)
{
    std::string qs = " select id from fanfics where author = :author and title = :title";

    SqlContext<int> ctx(db, std::move(qs), {{"author", author},{"title", title}});
    ctx.FetchSingleValue<int>("id", -1);
    return ctx.result;
}

DiagnosticSQLResult<int> GetFicIdByWebId(QString website, int webId, QSqlDatabase db)
{
    std::string qs = " select id from fanfics where {0}_id = :site_id";
    qs=fmt::format(qs,website.toStdString());

    SqlContext<int> ctx(db, std::move(qs), {{"site_id",webId}});
    ctx.FetchSingleValue<int>("id", -1);
    return ctx.result;
}

core::FicPtr LoadFicFromQuery(const QSqlQuery& q1, QString website = QStringLiteral("ffn"))
{
    Q_UNUSED(website)
    auto fic = core::Fanfic::NewFanfic();
    fic->userData.atChapter = q1.value(QStringLiteral("AT_CHAPTER")).toInt();
    fic->complete  = q1.value(QStringLiteral("COMPLETE")).toInt();

    fic->identity.id = q1.value(QStringLiteral("ID")).toInt();
    fic->wordCount = q1.value(QStringLiteral("WORDCOUNT")).toString();
    //fic->chapters = q1.value(QStringLiteral("CHAPTERS")).toString();
    fic->reviews = q1.value(QStringLiteral("REVIEWS")).toString();
    fic->favourites = q1.value(QStringLiteral("FAVOURITES")).toString();
    fic->follows = q1.value(QStringLiteral("FOLLOWS")).toString();
    fic->rated = q1.value(QStringLiteral("RATED")).toString();
    fic->fandom = q1.value(QStringLiteral("FANDOM")).toString();
    fic->title = q1.value(QStringLiteral("TITLE")).toString();
    fic->genres = q1.value(QStringLiteral("GENRES")).toString().split("##");
    fic->summary = q1.value(QStringLiteral("SUMMARY")).toString();
    fic->published = q1.value(QStringLiteral("PUBLISHED")).toDateTime();
    fic->updated = q1.value(QStringLiteral("UPDATED")).toDateTime();
    fic->characters = q1.value(QStringLiteral("CHARACTERS")).toString().split(",");
    //fic->authorId = q1.value(QStringLiteral("AUTHOR_ID").toInt();
    fic->author->name = q1.value(QStringLiteral("AUTHOR")).toString();
    fic->identity.web.ffn = q1.value(QStringLiteral("FFN_ID")).toInt();
    fic->identity.web.ao3 = q1.value(QStringLiteral("AO3_ID")).toInt();
    fic->identity.web.sb = q1.value(QStringLiteral("SB_ID")).toInt();
    fic->identity.web.sv = q1.value(QStringLiteral("SV_ID")).toInt();
    return fic;
}

DiagnosticSQLResult<core::FicPtr> GetFicByWebId(QString website, int webId, QSqlDatabase db)
{
    std::string qs = fmt::format(" select * from fanfics where {0}_id = :site_id",website.toStdString());
    SqlContext<core::FicPtr> ctx(db, std::move(qs), {{"site_id",webId}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data =  LoadFicFromQuery(q);
        ctx.result.data->webSite = website;
    });
    return ctx.result;
}

DiagnosticSQLResult<core::FicPtr> GetFicById( int ficId, QSqlDatabase db)
{
    std::string qs = " select * from fanfics where id = :fic_id";
    SqlContext<core::FicPtr> ctx(db, std::move(qs), {{"fic_id",ficId}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data =  LoadFicFromQuery(q);
    });
    return ctx.result;
}


DiagnosticSQLResult<bool> SetUpdateOrInsert(QSharedPointer<core::Fanfic> fic, QSqlDatabase db, bool alwaysUpdateIfNotInsert)
{
    auto getKeyQuery = fmt::format("Select ( select count(*) from FANFICS where  {0}_id = :site_id1) as COUNT_NAMED,"
                                  " ( select count(*) from FANFICS where  {0}_id = :site_id2 "
                                  "and (updated < :updated or updated is null)) as count_updated",fic->webSite.toStdString());

    SqlContext<bool> ctx(db, std::move(getKeyQuery));
    ctx.bindValue("site_id1", fic->identity.web.GetPrimaryId());
    ctx.bindValue("site_id2", fic->identity.web.GetPrimaryId());
    ctx.bindValue("updated", fic->updated);
    if(!fic)
        return ctx.result;

    int countNamed = 0;
    int countUpdated = 0;
    ctx.ForEachInSelect([&](QSqlQuery& q){
        countNamed = q.value(QStringLiteral("COUNT_NAMED")).toInt();
        countUpdated = q.value(QStringLiteral("count_updated")).toInt();
    });
    if(!ctx.Success())
        return ctx.result;

    bool requiresInsert = countNamed == 0;
    bool requiresUpdate = countUpdated > 0;
//    if(fic->fandom.contains("Greatest Showman"))
//        qDebug() << fic->fandom;
    if(alwaysUpdateIfNotInsert || (!requiresInsert && requiresUpdate))
        fic->updateMode = core::UpdateMode::update;
    if(requiresInsert)
        fic->updateMode = core::UpdateMode::insert;

    return ctx.result;
}

DiagnosticSQLResult<bool> InsertIntoDB(QSharedPointer<core::Fanfic> section, QSqlDatabase db)
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
DiagnosticSQLResult<bool>  UpdateInDB(QSharedPointer<core::Fanfic> section, QSqlDatabase db)
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
    return ctx.result;
}

DiagnosticSQLResult<bool> WriteRecommendation(core::AuthorPtr author, int fic_id, QSqlDatabase db)
{
    // atm this pairs favourite story with an author
    std::string qs = " insert into recommendations (recommender_id, fic_id) values(:recommender_id,:fic_id); ";
    SqlContext<bool> ctx(db, std::move(qs), {{"recommender_id", author->id},{"fic_id", fic_id}});
    if(!author || author->id < 0)
        return ctx.result;

    return ctx(true);
}

DiagnosticSQLResult<bool> WriteFicRelations(QList<core::FicWeightResult> ficRelations, QSqlDatabase db)
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
            result.oracleError = ctx.result.oracleError;
            break;
        }
    }
    return result;
}

DiagnosticSQLResult<bool> WriteAuthorsForFics(QHash<uint32_t, uint32_t> data,  QSqlDatabase db)
{
    std::string qs = " insert into FicAuthors (fic_id, author_id)"
                 " values(:fic_id, :author_id); ";
    SqlContext<bool> ctx(db);
    ctx.Prepare(std::move(qs));
    DiagnosticSQLResult<bool> result;
    for(auto i = data.begin(); i != data.end(); i++)
    {
        ctx.bindValue("fic_id",i.key());
        ctx.bindValue("author_id",i.value());
        ctx();
        if(!ctx.result.success)
        {
            result.success = false;
            result.oracleError = ctx.result.oracleError;
            break;
        }
    }
    return result;
}


DiagnosticSQLResult<int>  GetAuthorIdFromUrl(QString url, QSqlDatabase db)
{
    std::string qsl = " select id from recommenders where url like '%{0}%' ";
    qsl = fmt::format(qsl, url.toStdString());

    SqlContext<int> ctx(db, std::move(qsl));
    ctx.FetchSingleValue<int>("id", -1);
    return ctx.result;
}
DiagnosticSQLResult<int>  GetAuthorIdFromWebID(int id, QString website, QSqlDatabase db)
{
    std::string qs = "select id from recommenders where {0}_id = :id";
    qs=fmt::format(qs,website.toStdString());

    SqlContext<int> ctx(db, std::move(qs), {{"id", id}});
    ctx.FetchSingleValue<int>("id", -1, false);
    return ctx.result;
}


DiagnosticSQLResult<QSet<int> > GetAuthorsForFics(QSet<int> fics, QSqlDatabase db)
{
    auto* userThreadData = ThreadData::GetUserData();
    userThreadData->ficsForAuthorSearch = fics;
    std::string qs = "select distinct author_id from fanfics where cfInFicsForAuthors(id) > 0";
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("author_id", std::move(qs), "",[](QSqlQuery& q){
        return q.value("author_id").toInt();
    });
    ctx.result.data.remove(0);
    ctx.result.data.remove(-1);
    return ctx.result;

}
DiagnosticSQLResult<QSet<int>> GetRecommendersForFics(QSet<int> fics, QSqlDatabase db)
{
    std::string qs = "select distinct recommender_id from  recommendations where fic_id in ({0})";
    QStringList list;
    list.reserve(fics.size());
    for(auto fic: fics)
        list.push_back(QString::number(fic));
    qs = fmt::format(qs, "'" + list.join("','").toStdString() + "'");
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("recommender_id", std::move(qs), "",[](QSqlQuery& q){
        return q.value("recommender_id").toInt();
    });
    ctx.result.data.remove(0);
    ctx.result.data.remove(-1);
    return ctx.result;
}

DiagnosticSQLResult<QHash<uint32_t, int>> GetHashAuthorsForFics(QSet<int> fics, QSqlDatabase db)
{
    auto* userThreadData = ThreadData::GetUserData();
    userThreadData->ficsForAuthorSearch = fics;
    std::string qs = "select author_id, id from fanfics where cfInFicsForAuthors(id) > 0";
    SqlContext<QHash<uint32_t, int>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data[q.value("id").toUInt()] = q.value("author_id").toInt();
    });

    return ctx.result;
}

DiagnosticSQLResult<bool> AssignNewNameToAuthorWithId(core::AuthorPtr author, QSqlDatabase db)
{
    std::string qs = " UPDATE recommenders SET name = :name where id = :id";
    SqlContext<bool> ctx(db, std::move(qs), {{"name",author->name}, {"id",author->id}});
    if(author->GetIdStatus() != core::AuthorIdStatus::valid)
        return ctx.result;
    return ctx();
}

void ProcessIdsFromQuery(core::AuthorPtr author, const QSqlQuery& q)
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

core::AuthorPtr AuthorFromQuery(QSqlQuery& q)
{
    core::AuthorPtr result(new core::Author);
    result->AssignId(q.value("id").toInt());
    result->name = q.value("name").toString();
    result->recCount = q.value("rec_count").toInt();
    result->stats.favouritesLastUpdated = q.value("last_favourites_update").toDateTime().date();
    result->stats.favouritesLastChecked = q.value("last_favourites_checked").toDateTime().date();
    ProcessIdsFromQuery(result, q);
    return result;
}


DiagnosticSQLResult<QList<core::AuthorPtr>> GetAllAuthors(QString website,  QSqlDatabase db, int limit)
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
    ctx.FetchLargeSelectIntoList<core::AuthorPtr>("", std::move(qs), "select count(*) from recommenders where website_type = :site",[](QSqlQuery& q){
        return AuthorFromQuery(q);
    });
    return ctx.result;
}


DiagnosticSQLResult<QList<core::AuthorPtr>> GetAllAuthorsWithFavUpdateSince(QString website,
                                                                            QDateTime date,
                                                                            QSqlDatabase db,
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
    ctx.FetchLargeSelectIntoList<core::AuthorPtr>("", std::move(qs),
                                                  "select count(*) from recommenders where website_type = :site",
                                                  [](QSqlQuery& q){
        return AuthorFromQuery(q);
    });


    return ctx.result;
}

DiagnosticSQLResult<QList<core::AuthorPtr>> GetAllAuthorsWithFavUpdateBetween(QString website,
                                                                             QDateTime dateStart,
                                                                             QDateTime dateEnd,
                                                                             QSqlDatabase db,
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
    ctx.FetchLargeSelectIntoList<core::AuthorPtr>("",  std::move(qs),
                                                  "select count(*) from recommenders where website_type = :site",
                                                  [](QSqlQuery& q){
        return AuthorFromQuery(q);
    });


    return ctx.result;
}



DiagnosticSQLResult<QList<core::AuthorPtr>> GetAuthorsForRecommendationList(int listId,  QSqlDatabase db)
{
    std::string qs = "select id,name, url, ffn_id, ao3_id,sb_id, sv_id, "
                         "(select count(fic_id) from recommendations where recommender_id = recommenders.id) as rec_count, "
                         " last_favourites_update, last_favourites_checked "
                         " from recommenders "
                         "where id in ( select author_id from RecommendationListAuthorStats where list_id = :list_id )";

    SqlContext<QList<core::AuthorPtr>> ctx(db, std::move(qs), {{"list_id",listId}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
        auto author = AuthorFromQuery(q);
        ctx.result.data.push_back(author);
    });
    return ctx.result;
}

DiagnosticSQLResult<QString> GetAuthorsForRecommendationListClient(int list_id,  QSqlDatabase db)
{
    std::string qs = "select sources from recommendationlists where id = :list_id";
    SqlContext<QString> ctx(db, std::move(qs), BP1(list_id));
    ctx.FetchSingleValue<QString>("sources", "");
    return ctx.result;
}


DiagnosticSQLResult<core::AuthorPtr> GetAuthorByNameAndWebsite(QString name, QString website, QSqlDatabase db)
{
    core::AuthorPtr result;
    std::string qs = "select id,"
                         "name, url, website_type, ffn_id, ao3_id,sb_id, sv_id,"
                         " last_favourites_update, last_favourites_checked "
                         " from recommenders where {0}_id is not null and name = :name";
    qs=fmt::format(qs,website.toStdString());

    SqlContext<core::AuthorPtr> ctx(db, std::move(qs), {{"site",website},{"name",name}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data = AuthorFromQuery(q);
    });
    return ctx.result;
}
DiagnosticSQLResult<core::AuthorPtr> GetAuthorByIDAndWebsite(int id, QString website, QSqlDatabase db)
{
    std::string qs = "select r.id,r.name, r.url, r.website_type, r.ffn_id, r.ao3_id,r.sb_id, r.sv_id, "
                         " (select count(fic_id) from recommendations where recommender_id = r.id) as rec_count,"
                         " last_favourites_update, last_favourites_checked "
                         "from recommenders r where {0}_id is not null and {0}_id = :id";
    qs=fmt::format(qs,website.toStdString());
    SqlContext<core::AuthorPtr> ctx(db, std::move(qs), {{"site",website},{"id",id}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data = AuthorFromQuery(q);
    });
    return ctx.result;
}

void AuthorStatisticsFromQuery(const QSqlQuery& q,  core::AuthorPtr author)
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
    stats.prevalentGenre = q.value("prevalent_genre").toString();


    stats.sizeFactors[0] = q.value("size_tiny").toDouble();
    stats.sizeFactors[1] =  q.value("size_medium").toDouble();
    stats.sizeFactors[2] =  q.value("size_large").toDouble();
    stats.sizeFactors[3] = q.value("size_huge").toDouble();
    stats.firstPublished = q.value("first_published").toDate();
    stats.lastPublished= q.value("last_published").toDate();
}

DiagnosticSQLResult<bool> LoadAuthorStatistics(core::AuthorPtr author, QSqlDatabase db)
{
    std::string qs = "select * from AuthorFavouritesStatistics  where author_id = :id";

    SqlContext<bool> ctx(db, std::move(qs), {{"id",author->id}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
        AuthorStatisticsFromQuery(q, author);
    });
    return ctx.result;
}

DiagnosticSQLResult<QHash<int, QSet<int>>> LoadFullFavouritesHashset(QSqlDatabase db)
{
    std::string qs = "select * from recommendations order by recommender_id";

    SqlContext<QHash<int, QSet<int>>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data[q.value("recommender_id").toInt()].insert(q.value("fic_id").toInt());
    });
    return ctx.result;
}

DiagnosticSQLResult<core::AuthorPtr> GetAuthorByUrl(QString url, QSqlDatabase db)
{
    std::string qs = "select r.id,name, r.url, r.ffn_id, r.ao3_id, r.sb_id, r.sv_id, "
                         " (select count(fic_id) from recommendations where recommender_id = r.id) as rec_count,"
                         " last_favourites_update, last_favourites_checked "
                         " from recommenders r where url = :url";

    SqlContext<core::AuthorPtr> ctx(db, std::move(qs), {{"url",url}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data = AuthorFromQuery(q);
    });
    return ctx.result;
}

DiagnosticSQLResult<core::AuthorPtr> GetAuthorById(int id, QSqlDatabase db)
{
    std::string qs = "select id,name, url, ffn_id, ao3_id,sb_id, sv_id, "
                         "(select count(fic_id) from recommendations where recommender_id = :id) as rec_count, "
                         " last_favourites_update, last_favourites_checked "
                         "from recommenders where id = :id";

    SqlContext<core::AuthorPtr> ctx(db, std::move(qs), {{"id",id}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data = AuthorFromQuery(q);
    });
    return ctx.result;
}
DiagnosticSQLResult<bool> AssignAuthorNamesForWebIDsInFanficTable(QSqlDatabase db){

    std::string qs = " UPDATE fanfics SET author = (select name from recommenders rs where rs.ffn_id = fanfics.author_id) where exists (select name from recommenders rs where rs.ffn_id = fanfics.author_id)";
    SqlContext<bool> ctx(db, std::move(qs));
    return ctx();
}
DiagnosticSQLResult<QList<QSharedPointer<core::RecommendationList>>> GetAvailableRecommendationLists(QSqlDatabase db)
{
    std::string qs = "select * from RecommendationLists order by name";
    SqlContext<QList<QSharedPointer<core::RecommendationList>>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        QSharedPointer<core::RecommendationList>  list(new core::RecommendationList);
        list->alwaysPickAt = q.value("always_pick_at").toInt();
        list->minimumMatch = q.value("minimum").toInt();
        list->maxUnmatchedPerMatch= q.value("pick_ratio").toInt();
        list->id= q.value("id").toInt();
        list->name= q.value("name").toString();
        list->ficCount= q.value("fic_count").toInt();
        ctx.result.data.push_back(list);
    });
    return ctx.result;

}
// LIMIT
DiagnosticSQLResult<QSharedPointer<core::RecommendationList>> GetRecommendationList(int listId, QSqlDatabase db)
{
    std::string qs = "select * from RecommendationLists where id = :list_id";
    SqlContext<QSharedPointer<core::RecommendationList>> ctx(db, std::move(qs), {{"list_id", listId}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
        QSharedPointer<core::RecommendationList>  list(new core::RecommendationList);
        list->alwaysPickAt = q.value("always_pick_at").toInt();
        list->minimumMatch = q.value("minimum").toInt();
        list->maxUnmatchedPerMatch= q.value("pick_ratio").toInt();
        list->id= q.value("id").toInt();
        list->name= q.value("name").toString();
        list->ficCount= q.value("fic_count").toInt();
        ctx.result.data = list;
    });
    return ctx.result;
}

DiagnosticSQLResult<QSharedPointer<core::RecommendationList>> GetRecommendationList(QString name, QSqlDatabase db)
{
    std::string qs = "select * from RecommendationLists where name = :list_name";

    SqlContext<QSharedPointer<core::RecommendationList>> ctx(db, std::move(qs), {{"list_name", name}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
        QSharedPointer<core::RecommendationList>  list(new core::RecommendationList);
        list->alwaysPickAt = q.value("always_pick_at").toInt();
        list->minimumMatch = q.value("minimum").toInt();
        list->maxUnmatchedPerMatch= q.value("pick_ratio").toInt();
        list->id= q.value("id").toInt();
        list->name= q.value("name").toString();
        list->ficCount= q.value("fic_count").toInt();
        ctx.result.data = list;
    });
    return ctx.result;
}

DiagnosticSQLResult<QList<core::AuhtorStatsPtr>> GetRecommenderStatsForList(int listId, QString sortOn, QString order, QSqlDatabase db)
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
    ctx.ForEachInSelect([&](QSqlQuery& q){
        core::AuhtorStatsPtr stats(new core::AuthorRecommendationStats);
        stats->isValid = true;
        stats->matchesWithReference = q.value("match_count").toInt();
        stats->matchRatio= q.value("match_ratio").toDouble();
        stats->authorId= q.value("author_id").toInt();
        stats->listId= listId;
        stats->totalRecommendations= q.value("fic_count").toInt();
        stats->authorName = q.value("name").toString();
        ctx.result.data.push_back(stats);
    });
    return ctx.result;

}

DiagnosticSQLResult<int> GetMatchCountForRecommenderOnList(int authorId, int list, QSqlDatabase db)
{
    std::string qs = "select fic_count from RecommendationListAuthorStats where list_id = :list_id and author_id = :author_id";
    SqlContext<int> ctx(db, std::move(qs), {{"list_id", list}, {"author_id",authorId}});
    ctx.FetchSingleValue<int>("fic_count", -1);
    return ctx.result;
}

DiagnosticSQLResult<QVector<int>> GetAllFicIDsFromRecommendationList(int listId,  core::StoryFilter::ESourceListLimiter limiter, QSqlDatabase db)
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
    return ctx.result;
}

DiagnosticSQLResult<QVector<int>> GetAllSourceFicIDsFromRecommendationList(int listId,  QSqlDatabase db)
{
    std::string qs = "select fic_id from RecommendationListData where list_id = :list_id and is_origin = 1";
    SqlContext<QVector<int>> ctx(db);
    ctx.bindValue("list_id",listId);
    ctx.FetchLargeSelectIntoList<int>("fic_id", std::move(qs));
    return ctx.result;
}




DiagnosticSQLResult<QHash<int,int>> GetRelevanceScoresInFilteredReclist(const core::ReclistFilter& filter, QSqlDatabase db)
{
    std::string qs = "select fic_id, {0} from RecommendationListData where list_id = :list_id";
    std::string pointsField = filter.scoreType == core::StoryFilter::st_points ? "match_count" : "no_trash_score";
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

    SqlContext<QHash<int,int>> ctx(db, std::move(qs));
    ctx.bindValue("list_id",filter.mainListId);
    if(filter.minMatchCount != 0)
        ctx.bindValue("match_count",filter.minMatchCount);
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data[q.value("fic_id").toInt()] = q.value(QString::fromStdString(pointsField)).toInt();
    });
    return ctx.result;
}

DiagnosticSQLResult<QStringList> GetAllAuthorNamesForRecommendationList(int listId, QSqlDatabase db)
{
    std::string qs = "select name from recommenders where id in (select author_id from RecommendationListAuthorStats where list_id = :list_id)";
    SqlContext<QStringList> ctx(db);
    ctx.bindValue("list_id",listId);
    ctx.FetchLargeSelectIntoList<QString>("name", std::move(qs));
    return ctx.result;
}

DiagnosticSQLResult<int>  GetCountOfTagInAuthorRecommendations(int authorId, QString tag, QSqlDatabase db)
{
    std::string qs = "select count(distinct fic_id) as ficcount from FicTags ft where ft.tag = :tag and exists"
                         " (select 1 from Recommendations where ft.fic_id = fic_id and recommender_id = :recommender_id)";

    SqlContext<int> ctx(db, std::move(qs), {{"tag", tag}, {"recommender_id",authorId}});
    ctx.FetchSingleValue<int>("ficcount", 0);
    return ctx.result;
}

//!todo needs check if the query actually works
DiagnosticSQLResult<int> GetMatchesWithListIdInAuthorRecommendations(int authorId, int listId, QSqlDatabase db)
{
    std::string qs = "select count(fic_id) as ficcount from Recommendations r where recommender_id = :author_id and exists "
                         " (select 1 from RecommendationListData rld where rld.list_id = :list_id and fic_id = rld.fic_id)";
    SqlContext<int> ctx(db, std::move(qs), {{"author_id", authorId}, {"list_id",listId}});
    ctx.FetchSingleValue<int>("ficcount", 0);
    return ctx.result;
}

DiagnosticSQLResult<bool> DeleteRecommendationList(int listId, QSqlDatabase db )
{
    SqlContext<bool> ctx(db);
    if(listId == 0)
        return ctx.result;
    ctx.bindValue("list_id", listId);
    ctx.ExecuteList({"delete from RecommendationLists where id = :list_id",
                     "delete from RecommendationListAuthorStats where list_id = :list_id",
                     "delete from RecommendationListData where list_id = :list_id"});

    return ctx.result;
}

DiagnosticSQLResult<bool> DeleteRecommendationListData(int listId, QSqlDatabase db)
{
    SqlContext<bool> ctx(db);
    if(listId == 0)
        return ctx.result;
    ctx.bindValue("list_id", listId);
    ctx.ExecuteList({"delete from RecommendationListAuthorStats where list_id = :list_id",
                     "delete from RecommendationListData where list_id = :list_id"});

    return ctx.result;
}

DiagnosticSQLResult<bool> CopyAllAuthorRecommendationsToList(int authorId, int listId, QSqlDatabase db )
{
    std::string qs = "insert into RecommendationListData (fic_id, list_id)"
                         " select fic_id, {0} as list_id from recommendations r where r.recommender_id = :author_id and "
                         " not exists( select 1 from RecommendationListData where list_id=:list_id and fic_id = r.fic_id) ";
    qs=fmt::format(qs,listId);
    return SqlContext<bool>(db, std::move(qs), {{"author_id",authorId},{"list_id",listId}})();
}
DiagnosticSQLResult<bool> WriteAuthorRecommendationStatsForList(int listId, core::AuhtorStatsPtr stats, QSqlDatabase db)
{
    std::string qs = "insert into RecommendationListAuthorStats (author_id, fic_count, match_count, match_ratio, list_id) "
                         "values(:author_id, :fic_count, :match_count, :match_ratio, :list_id)";

    SqlContext<bool> ctx(db, std::move(qs));
    if(!stats)
        return ctx.result;
    ctx.bindValue("author_id",stats->authorId);
    ctx.bindValue("fic_count",stats->totalRecommendations);
    ctx.bindValue("match_count",stats->matchesWithReference);
    ctx.bindValue("match_ratio",stats->matchRatio);
    ctx.bindValue("list_id",listId);
    ctx.ExecAndCheck(true);
    return ctx.result;
}
DiagnosticSQLResult<bool> CreateOrUpdateRecommendationList(QSharedPointer<core::RecommendationList> list, QDateTime creationTimestamp, QSqlDatabase db)
{
    std::string qs;
    SqlContext<bool> ctx(db);
    //int freeId = -2;
    {
        qs = "select max(id) as id from RecommendationLists";
        ctx.ReplaceQuery(std::move(qs));
        if(!ctx.ExecAndCheckForData())
        {
            return ctx.result;
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
        return ctx.result;
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
        return ctx.result;
    }


    return ctx.result;
}

DiagnosticSQLResult<bool> WriteAuxParamsForReclist(QSharedPointer<core::RecommendationList> list, QSqlDatabase db)
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



DiagnosticSQLResult<bool> UpdateFicCountForRecommendationList(int listId, QSqlDatabase db)
{
    std::string qs = "update RecommendationLists set fic_count=(select count(fic_id) "
                         " from RecommendationListData where list_id = :list_id) where id = :list_id";
    SqlContext<bool> ctx(db, std::move(qs),{{"list_id",listId}});
    if(listId == -1 || !ctx.ExecAndCheck())
        return ctx.result;
    return ctx.result;
}

DiagnosticSQLResult<bool> DeleteTagFromDatabase(QString tag, QSqlDatabase db)
{
    SqlContext<bool> ctx(db);
    ctx.bindValue("tag", tag);
    ctx.ExecuteList({"delete from FicTags where tag = :tag",
                     "delete from tags where tag = :tag"});
    return ctx.result;
}

DiagnosticSQLResult<bool>  CreateTagInDatabase(QString tag, QSqlDatabase db)
{
    std::string qs = "INSERT INTO TAGS(TAG) VALUES(:tag)";
    return SqlContext<bool>(db, std::move(qs),{{"tag",tag}})();
}

DiagnosticSQLResult<int>  GetRecommendationListIdForName(QString name, QSqlDatabase db)
{
    std::string qs = "select id from RecommendationLists where name = :name";
    SqlContext<int> ctx(db, std::move(qs), {{"name", name}});
    ctx.FetchSingleValue<int>("id", 0);
    return ctx.result;
}

DiagnosticSQLResult<bool>  AddAuthorFavouritesToList(int authorId, int listId, QSqlDatabase db)
{
    std::string qs = " update RecommendationListData set match_count = match_count+1 where "
                         " list_id = :list_id "
                         " and fic_id in (select fic_id from recommendations r where recommender_id = :author_id)";
    return  SqlContext<bool>(db, std::move(qs),{{"author_id",authorId},{"list_id",listId}})();
}

DiagnosticSQLResult<bool>  ShortenFFNurlsForAllFics(QSqlDatabase db)
{
    std::string qs = "update fanfics set url = cfReturnCapture('(/s/\\d+/)', url)";
    return  SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> IsGenreList(QStringList list, QString website, QSqlDatabase db)
{
    std::string qs = "select count(*) as idcount from genres where genre in({0}) and website = :website";
    qs = fmt::format(qs, "'" + list.join("','").toStdString() + "'");

    SqlContext<int> ctx(db, std::move(qs), {{"website", website}});
    ctx.FetchSingleValue<int>("idcount", 0);

    DiagnosticSQLResult<bool> realResult;
    realResult.success = ctx.result.success;
    realResult.oracleError = ctx.result.oracleError;
    realResult.data = ctx.result.data > 0;
    return realResult;

}

DiagnosticSQLResult<QVector<int>> GetWebIdList(QString where, QString website, QSqlDatabase db)
{
    //QVector<int> result;
    std::string fieldName = website.toStdString() + "_id";
    std::string qs = fmt::format("select {0}_id from fanfics {1}",website.toStdString(),where.toStdString());
    SqlContext<QVector<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>(std::move(fieldName), std::move(qs));
    return ctx.result;
}

DiagnosticSQLResult<QVector<int>> GetIdList(QString where, QSqlDatabase db)
{
    std::string qs = fmt::format("select id from fanfics {0} order by id asc",where.toStdString());
    SqlContext<QVector<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("id", std::move(qs));
    return ctx.result;
}

DiagnosticSQLResult<bool> DeactivateStory(int id, QString website, QSqlDatabase db)
{
    std::string qs = "update fanfics set alive = 0 where {0}_id = :id";
    qs=fmt::format(qs,website.toStdString());
    return SqlContext<bool>(db, std::move(qs), {{"id",id}})();
}

DiagnosticSQLResult<bool> CreateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, QSqlDatabase db)
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
    return ctx.result;
}

DiagnosticSQLResult<bool>  UpdateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, QSqlDatabase db)
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
    //q1.bindValue(":info_wordcount", author->ficCount);
    //    q1.bindValue(":last_published_fic_date", author->ficCount);
    //    q1.bindValue(":first_published_fic_date", author->ficCount);
    //    q1.bindValue(":own_wordcount", author->ficCount);
    //    q1.bindValue(":own_favourites", author->ficCount);
    //    q1.bindValue(":own_finished_ratio", author->ficCount);
    //    q1.bindValue(":most_written_size", author->ficCount);

    return ctx.result;
}

DiagnosticSQLResult<bool> UpdateAuthorFavouritesUpdateDate(int authorId, QDateTime date, QSqlDatabase db)
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

    return ctx.result;
}


DiagnosticSQLResult<QStringList> ReadUserTags(QSqlDatabase db)
{
    DiagnosticSQLResult<QStringList> result;
    QSet<QString> tags;
    {
    std::string qs = "Select tag from tags";
    SqlContext<QStringList> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("tag", std::move(qs));
    if(!ctx.result.success)
        return ctx.result;
    tags = QSet<QString>(ctx.result.data.begin(), ctx.result.data.end());
    }
    {
    std::string qs = "select distinct tag from fictags";
    SqlContext<QStringList> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("tag", std::move(qs));
    if(!ctx.result.success)
        return ctx.result;
    tags = QSet<QString>(ctx.result.data.begin(), ctx.result.data.end());
    }
    auto values = tags.values();
    std::sort(values.begin(), values.end());
    result.data = values;
    result.success = true;
    return result;

}

DiagnosticSQLResult<bool>  PushTaglistIntoDatabase(QStringList tagList, QSqlDatabase db)
{
    int counter = 0;
    std::string qs = "INSERT INTO TAGS (TAG, id) VALUES (:tag, :id)";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.ExecuteWithKeyListAndBindFunctor<QString>(tagList, [&](QString tag, QSqlQuery q){
        q.bindValue(":tag", tag);
        q.bindValue(":id", counter);
        counter++;
    }, true);
    return ctx.result;
}

DiagnosticSQLResult<bool>  AssignNewNameForAuthor(core::AuthorPtr author, QString name, QSqlDatabase db)
{
    std::string qs = " UPDATE recommenders SET name = :name where id = :id";
    SqlContext<bool> ctx(db, std::move(qs), {{"name", name},{"id", author->id}});
    if(author->GetIdStatus() != core::AuthorIdStatus::valid)
        return ctx.result;
    return ctx();
}

DiagnosticSQLResult<QList<int>> GetAllAuthorIds(QSqlDatabase db)
{
    std::string qs = "select distinct id from recommenders";

    SqlContext<QList<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("id", std::move(qs));
    return ctx.result;
}

// not working, no idea why
//DiagnosticSQLResult<QSet<int> > GetAllMatchesWithRecsUID(QSharedPointer<core::RecommendationList> params, QString uid, QSqlDatabase db)
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
//    return ctx.result;
//}

DiagnosticSQLResult<QSet<int> > GetAllMatchesWithRecsUID(QSharedPointer<core::RecommendationList> params, QString uid, QSqlDatabase db)
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

    return ctx.result;
}


DiagnosticSQLResult<QSet<int> > ConvertFFNSourceFicsToDB(QString uid, QSqlDatabase db)
{
    SqlContext<QSet<int>> ctx(db);
    ctx.bindValue("uid", uid);
    ctx.FetchLargeSelectIntoList<int>("id",
                                      "select id from fanfics where cfInSourceFics(ffn_id)");

    return ctx.result;
}

static auto getFicWeightPtrFromQuery = [](auto& q){
    core::FicWeightPtr fw(new core::FanficDataForRecommendationCreation);
    fw->adult = q.value("Rated").toString() == "M";
    fw->authorId = q.value("author_id").toInt();
    fw->complete = q.value("complete").toBool();
    //fw->chapterCount = q.value("chapters").toInt();
    fw->dead = !fw->complete  && q.value("updated").toDateTime().daysTo(QDateTime::currentDateTime()) > 365;
    fw->fandoms.push_back(q.value("fandom1").toInt());
    fw->fandoms.push_back(q.value("fandom2").toInt());
    fw->favCount = q.value("favourites").toInt();
    fw->published = q.value("published").toDate();
    fw->updated = q.value("updated").toDate();
    fw->genreString = q.value("genres").toString();
    fw->id = q.value("id").toInt();
    fw->reviewCount = q.value("reviews").toInt();
    fw->slash = q.value("filter_pass_1").toBool();
    fw->wordCount = q.value("wordcount").toInt();
    return fw;
};

DiagnosticSQLResult<QHash<uint32_t, core::FicWeightPtr>> GetFicsForRecCreation(QSqlDatabase db)
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
    return ctx.result;


}
DiagnosticSQLResult<bool> ConvertFFNTaggedFicsToDB(QHash<int, int>& hash, QSqlDatabase db)
{
    SqlContext<int> ctx(db);
    QLOG_INFO() << "keys list:" << hash.keys();

    for(auto i = hash.begin(); i != hash.end(); i++)
    {
        ctx.bindValue("ffn_id", i.key());
        ctx.FetchSingleValue<int>("id", -1,false, "select id from fanfics where ffn_id = :ffn_id");
        QString error = ctx.result.oracleError;
        if(!error.isEmpty() && error != "no data to read")
        {
            QLOG_ERROR() << "///ERROR///" << error;
            ctx.result.success = false;
            break;
        }
        if(ctx.result.data != -1)
            i.value() = ctx.result.data; // todo check this iterator usage is valid
        else
            hash.remove(i.key());
    }

    QLOG_INFO() << "Resulting list:" << hash;

    SqlContext<bool> ctx1(db);
    ctx1.result.success = ctx.result.success;
    ctx1.result.oracleError = ctx.result.oracleError;
    return ctx1.result;
}

DiagnosticSQLResult<bool> ConvertDBFicsToFFN(QHash<int, int>& hash, QSqlDatabase db)
{
    SqlContext<int> ctx(db);
    QHash<int, int>::iterator it = hash.begin();
    QHash<int, int>::iterator itEnd = hash.end();
    while(it != itEnd){
        ctx.bindValue("id", it.key());
        ctx.FetchSingleValue<int>("ffn_id", -1, true, "select ffn_id from fanfics where id = :id");
        QString error = ctx.result.oracleError;
        if(!error.isEmpty() && error != "no data to read")
        {
            ctx.result.success = false;
            break;
        }
        it.value() = ctx.result.data; // todo check this iterator usage is valid
        it++;
    }

    SqlContext<bool> ctx1(db);
    ctx1.result.success = ctx.result.success;
    ctx1.result.oracleError = ctx.result.oracleError;
    return ctx1.result;
}



DiagnosticSQLResult<bool> ResetActionQueue(QSqlDatabase db)
{
    std::string qs = "update fanfics set queued_for_action = 0";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx();
    return ctx.result;
}

DiagnosticSQLResult<bool> WriteDetectedGenres(QVector<genre_stats::FicGenreData> fics, QSqlDatabase db)
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
            return ctx.result;
    }
    return ctx.result;
}


DiagnosticSQLResult<bool> WriteDetectedGenresIteration2(QVector<genre_stats::FicGenreData> fics, QSqlDatabase db)
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
            return ctx.result;
    }
    return ctx.result;
}

DiagnosticSQLResult<QHash<int, QList<genre_stats::GenreBit>>> GetFullGenreList(QSqlDatabase db,bool useOriginalOnly)
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
    ctx.ForEachInSelect([&](QSqlQuery& q){
        QList<genre_stats::GenreBit> dataForFic;
        auto id = q.value("id").toInt();
        QString genres = q.value("genres").toString();
        if(q.value("true_genre1").toString().trimmed().isEmpty() || useOriginalOnly)
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
                QString tgKey = "true_genre" + QString::number(i);
                QString tgKeyValue = "true_genre" + QString::number(i) + "_percent";
                QString genre = q.value(tgKey).toString();
                if(genre.isEmpty()){
                    break;
                }

                genre_stats::GenreBit bit;
                bit.genres = genre.split(QRegExp("[\\s,]"), Qt::SkipEmptyParts);
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
    return ctx.result;
}


DiagnosticSQLResult<QHash<int, int> > GetMatchesForUID(QString uid, QSqlDatabase db)
{
    std::string qs = "select fic_id, count(fic_id) as cnt from recommendations where cfInAuthors(recommender_id, :uid) = 1 group by fic_id";
    SqlContext<QHash<int, int> > ctx(db);
    ctx.bindValue("uid", uid);
    ctx.FetchSelectFunctor(std::move(qs), [](QHash<int, int>& data, QSqlQuery& q){
        int fic = q.value("fic_id").toInt();
        int matches = q.value("cnt").toInt();
        //QLOG_INFO() << " fic_id: " << fic << " matches: " << matches;
        data[fic] = matches;
    });
    return ctx.result;
}

DiagnosticSQLResult<QStringList> GetAllAuthorFavourites(int id, QSqlDatabase db)
{
    std::string qs = "select id, ffn_id, ao3_id, sb_id, sv_id from fanfics where id in (select fic_id from recommendations where recommender_id = :author_id )";
    SqlContext<QStringList> ctx (db, std::move(qs), {{"author_id", id}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
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
    return ctx.result;

}

DiagnosticSQLResult<QList<int>> GetAllAuthorRecommendationIDs(int id, QSqlDatabase db)
{
    std::string qs = "select distinct fic_id from recommendations where recommender_id = :author_id";
    SqlContext<QList<int>> ctx (db, std::move(qs), {{"author_id", id}});
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data.push_back(q.value("fic_id").toInt());
    });
    return ctx.result;
}

DiagnosticSQLResult<bool> IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, QSqlDatabase db)
{
    std::string qs = " update RecommendationListData set match_count = match_count+1 where "
                         " list_id = :list_id "
                         " and fic_id in (select fic_id from recommendations r where recommender_id = :author_id)";
    return SqlContext<bool> (db, std::move(qs), {{"author_id", authorId},{"list_id", listId}})();
}


DiagnosticSQLResult<bool> DecrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, QSqlDatabase db)
{
    std::string qs = " update RecommendationListData set match_count = match_count-1 where "
                         " list_id = :list_id "
                         " and fic_id in (select fic_id from recommendations r where recommender_id = :author_id)";

    SqlContext<bool> ctx (db, std::move(qs), {{"author_id", authorId},{"list_id", listId}});
    if(!ctx.ExecAndCheck())
        return ctx.result;

    ctx.ReplaceQuery("delete from RecommendationListData where match_count <= 0");
    ctx.ExecAndCheck();
    return  ctx.result;
}

DiagnosticSQLResult<QSet<QString>> GetAllGenres(QSqlDatabase db)
{
    std::string qs = "select genre from genres";
    SqlContext<QSet<QString>> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("genre", std::move(qs));
    return ctx.result;
}

static core::FandomPtr FandomfromQueryNew (const QSqlQuery& q, core::FandomPtr fandom = core::FandomPtr())
{
    if(!fandom)
    {
        fandom = core::Fandom::NewFandom();
        fandom->id = q.value("ID").toInt();
        fandom->SetName(q.value("name").toString());
        fandom->tracked = q.value("tracked").toInt();
        fandom->lastUpdateDate = q.value("updated").toDate();
        fandom->AddUrl({q.value("url").toString(),
                        q.value("website").toString(),
                        ""});
    }
    else{
        fandom->AddUrl({q.value("url").toString(),
                        q.value("website").toString(),
                        ""});
    }
    return fandom;

}
static DiagnosticSQLResult<bool> GetFandomStats(core::FandomPtr fandom, QSqlDatabase db)
{
    std::string qs = "select * from fandomsources where global_id = :id";
    SqlContext<bool> ctx(db, std::move(qs));
    if(!fandom)
        return ctx.result;
    ctx.bindValue("id",fandom->id);
    ctx.ExecAndCheck();
    if(ctx.Next())
    {
        fandom->source = ctx.value("website").toString();
        fandom->ficCount = ctx.value("fic_count").toInt();
        fandom->averageFavesTop3 = ctx.value("average_faves_top_3").toDouble();
        fandom->dateOfCreation = ctx.value("date_of_creation").toDate();
        fandom->dateOfFirstFic = ctx.value("date_of_first_fic").toDate();
        fandom->dateOfLastFic = ctx.value("date_of_last_fic").toDate();
    }
    return ctx.result;
}

DiagnosticSQLResult<QList<core::FandomPtr>> GetAllFandoms(QSqlDatabase db)
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
    ctx.ForEachInSelect([&](QSqlQuery& q){
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
    return ctx.result;
}
DiagnosticSQLResult<QList<core::FandomPtr> > GetAllFandomsAfter(int id, QSqlDatabase db)
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
    ctx.ForEachInSelect([&](QSqlQuery& q){
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
    return ctx.result;
}
DiagnosticSQLResult<core::FandomPtr> GetFandom(QString fandom, bool loadFandomStats, QSqlDatabase db)
{
    core::FandomPtr currentFandom;

    std::string qs = " select ind.id as id, ind.name as name, ind.tracked as tracked, urls.url as url, urls.website as website,"
                         " urls.custom as section, ind.updated as updated from fandomindex ind left join fandomurls urls on ind.id = urls.global_id"
                         " where lower(name) = lower(:fandom) ";
    SqlContext<core::FandomPtr> ctx(db, std::move(qs), BP1(fandom));

    ctx.ForEachInSelect([&](QSqlQuery& q){
        currentFandom = FandomfromQueryNew(q, currentFandom);
    });
    if(loadFandomStats)
    {
        auto statResult = GetFandomStats(currentFandom, db);
        if(!statResult.success)
        {
            ctx.result.success = false;
            return ctx.result;
        }
    }

    ctx.result.data = currentFandom;
    return ctx.result;
}

DiagnosticSQLResult<core::FandomPtr> GetFandom(int id, bool loadFandomStats, QSqlDatabase db)
{
    core::FandomPtr currentFandom;

    std::string qs = " select ind.id as id, ind.name as name, ind.tracked as tracked, urls.url as url, urls.website as website,"
                         " urls.custom as section, ind.updated as updated from fandomindex ind left join fandomurls urls on ind.id = urls.global_id"
                         " where id = :id";
    SqlContext<core::FandomPtr> ctx(db, std::move(qs), BP1(id));

    ctx.ForEachInSelect([&](QSqlQuery& q){
        currentFandom = FandomfromQueryNew(q, currentFandom);
    });
    if(loadFandomStats)
    {
        auto statResult = GetFandomStats(currentFandom, db);
        if(!statResult.success)
        {
            ctx.result.success = false;
            return ctx.result;
        }
    }

    ctx.result.data = currentFandom;
    return ctx.result;
}

DiagnosticSQLResult<bool> IgnoreFandom(int fandom_id, bool including_crossovers, QSqlDatabase db)
{
    std::string qs = " insert into ignored_fandoms (fandom_id, including_crossovers) values (:fandom_id, :including_crossovers) ";
    SqlContext<bool> ctx(db, std::move(qs),  BP2(fandom_id,including_crossovers));
    return ctx(true);
}

DiagnosticSQLResult<bool> SetUserProfile(int id,  QSqlDatabase db)
{
    std::string qs = " update user_settings set value = :id where name = 'user_ffn_id' ";
    SqlContext<bool> ctx(db, std::move(qs),  BP1(id));
    return ctx(true);
}

DiagnosticSQLResult<int> GetUserProfile(QSqlDatabase db)
{
    std::string qs = "select value from user_settings where name = 'user_ffn_id'";
    SqlContext<int> ctx(db, std::move(qs));
    ctx.FetchSingleValue<int>("value", -1);
    return ctx.result;
}
DiagnosticSQLResult<int> GetRecommenderIDByFFNId(int id, QSqlDatabase db)
{
    std::string qs = "select id from recommenders where ffn_id = :id";
    SqlContext<int> ctx(db, std::move(qs), BP1(id));
    ctx.FetchSingleValue<int>("id", -1);
    return ctx.result;
}

DiagnosticSQLResult<bool> RemoveFandomFromIgnoredList(int fandom_id, QSqlDatabase db)
{
    std::string qs = " delete from ignored_fandoms where fandom_id  = :fandom_id";
    return SqlContext<bool>(db, std::move(qs), BP1(fandom_id))();
}

DiagnosticSQLResult<QStringList> GetIgnoredFandoms(QSqlDatabase db)
{
    std::string qs = "select name from fandomindex where id in (select fandom_id from ignored_fandoms) order by name asc";
    SqlContext<QStringList> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("name", std::move(qs));
    return ctx.result;
}

DiagnosticSQLResult<QHash<int, QString>> GetFandomNamesForIDs(QList<int>ids, QSqlDatabase db)
{
    SqlContext<QHash<int, QString> > ctx(db);
    std::string qs = "select id, name from fandomindex where id in ({0})";
    QStringList inParts;
    inParts.reserve(ids.size());
    for(auto id: ids)
        inParts.push_back("'" + QString::number(id) + "'");
    if(inParts.size() == 0)
        return ctx.result;
    qs = fmt::format(qs, inParts.join(",").toStdString());

    ctx.FetchSelectFunctor(std::move(qs), [](QHash<int, QString>& data, QSqlQuery& q){
        data[q.value("id").toInt()] = q.value("name").toString();
    });
    return ctx.result;
}

DiagnosticSQLResult<QHash<int, bool> > GetIgnoredFandomIDs(QSqlDatabase db)
{
    std::string qs = "select fandom_id, including_crossovers from ignored_fandoms order by fandom_id asc";
    SqlContext<QHash<int, bool> > ctx(db);
    ctx.FetchSelectFunctor(std::move(qs), [](QHash<int, bool>& data, QSqlQuery& q){
        data[q.value("fandom_id").toInt()] = q.value("including_crossovers").toBool();
    }, true);
    return ctx.result;
}


DiagnosticSQLResult<bool> IgnoreFandomSlashFilter(int fandom_id, QSqlDatabase db)
{
    std::string qs = " insert into ignored_fandoms_slash_filter (fandom_id) values (:fandom_id) ";
    SqlContext<bool> ctx(db, std::move(qs), BP1(fandom_id));
    return ctx(true);
}

DiagnosticSQLResult<bool> RemoveFandomFromIgnoredListSlashFilter(int fandom_id, QSqlDatabase db)
{
    std::string qs = " delete from ignored_fandoms_slash_filter where fandom_id  = :fandom_id";
    return SqlContext<bool>(db, std::move(qs), BP1(fandom_id))();
}

DiagnosticSQLResult<QStringList> GetIgnoredFandomsSlashFilter(QSqlDatabase db)
{
    std::string qs = "select name from fandomindex where id in (select fandom_id from ignored_fandoms_slash_filter) order by name asc";
    SqlContext<QStringList> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("name", std::move(qs));
    return ctx.result;
}

DiagnosticSQLResult<bool> CleanupFandom(int fandom_id, QSqlDatabase db)
{
    SqlContext<bool> ctx(db);
    ctx.bindValue("fandom_id", fandom_id);
    ctx.ExecuteList({"delete from fanfics where id in (select distinct fic_id from ficfandoms where fandom_id = :fandom_id)",
                     "delete from fictags where fic_id in (select distinct fic_id from ficfandoms where fandom_id = :fandom_id)",
                     "delete from recommendationlistdata where fic_id in (select distinct fic_id from ficfandoms where fandom_id = :fandom_id)",
                     "delete from ficfandoms where fandom_id = :fandom_id"});
    return ctx.result;
}

DiagnosticSQLResult<bool> DeleteFandom(int fandom_id, QSqlDatabase db)
{
    SqlContext<bool> ctx(db);
    ctx.bindValue("fandom_id", fandom_id);
    ctx.ExecuteList({"delete from fandomindex where id = :fandom_id",
                     "delete from fandomurls where global_id = :fandom_id"});
    return ctx.result;
}

DiagnosticSQLResult<QStringList> GetTrackedFandomList(QSqlDatabase db)
{
    std::string qs = " select name from fandomindex where tracked = 1 order by name asc";
    SqlContext<QStringList> ctx(db);
    ctx.FetchLargeSelectIntoList<QString>("name", std::move(qs));
    return ctx.result;
}

DiagnosticSQLResult<int> GetFandomCountInDatabase(QSqlDatabase db)
{
    std::string qs = "Select count(name) as cn from fandomindex";
    SqlContext<int> ctx(db, std::move(qs));
    ctx.FetchSingleValue<int>("cn", 0);
    return ctx.result;
}


DiagnosticSQLResult<bool> AddFandomForFic(int fic_id, int fandom_id, QSqlDatabase db)
{
    std::string qs = " insert into ficfandoms (fic_id, fandom_id) values (:fic_id, :fandom_id) ";
    SqlContext<bool> ctx(db, std::move(qs), BP2(fic_id,fandom_id));

    if(fic_id == -1 || fandom_id == -1)
        return ctx.result;

    return ctx(true);
}

DiagnosticSQLResult<QStringList>  GetFandomNamesForFicId(int fic_id, QSqlDatabase db)
{
    std::string qs = "select name from fandomindex where fandomindex.id in (select fandom_id from ficfandoms ff where ff.fic_id = :fic_id)";
    SqlContext<QStringList> ctx(db, std::move(qs), BP1(fic_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        auto fandom = q.value("name").toString().trimmed();
        if(!fandom.contains("????"))
            ctx.result.data.push_back(fandom);
    });
    return ctx.result;
}

DiagnosticSQLResult<bool> AddUrlToFandom(int fandomID, core::Url url, QSqlDatabase db)
{
    std::string qs = " insert into fandomurls (global_id, url, website, custom) "
                         " values (:global_id, :url, :website, :custom) ";

    SqlContext<bool> ctx(db, std::move(qs),{{"global_id",fandomID},
                                 {"url",url.GetUrl()},
                                 {"website",url.GetSource()},
                                 {"custom",url.GetType()}});
    if(fandomID == -1)
        return ctx.result;
    return ctx(true);
}

DiagnosticSQLResult<QList<int>> GetRecommendersForFicIdAndListId(int fic_id, QSqlDatabase db)
{
    std::string qs = "Select distinct recommender_id from recommendations where fic_id = :fic_id";
    SqlContext<QList<int>> ctx(db, "", BP1(fic_id));
    ctx.FetchLargeSelectIntoList<int>("recommender_id", std::move(qs));
    return ctx.result;
}

DiagnosticSQLResult<QSet<int> > GetAllTaggedFics(QSqlDatabase db)
{
        std::string qs = "select distinct fic_id from fictags ";
        SqlContext<QSet<int>> ctx(db);
        ctx.FetchLargeSelectIntoList<int>("fic_id", std::move(qs));
        return ctx.result;
}

DiagnosticSQLResult<QSet<int>> GetFicsTaggedWith(QStringList tags, bool useAND, QSqlDatabase db){

    if(!useAND)
    {
        std::string qs = "select distinct fic_id from fictags ";
        QStringList parts;

        if(tags.size() > 0)
            parts.push_back(QString("tag in ('{0}')").arg(tags.join("','")));

        if(parts.size() > 0)
        {
            qs+= " where ";
            qs+= parts.join(" and ").toStdString();
        }
        SqlContext<QSet<int>> ctx(db);
        ctx.FetchLargeSelectIntoList<int>("fic_id", std::move(qs));
        return ctx.result;
    }
    else {
        std::string qs = "select distinct fic_id from fictags ft where ";
        QString prototype = " exists (select fic_id from fictags where ft.fic_id = fic_id and tag = '{0}') ";
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
        return ctx.result;
    }
}

DiagnosticSQLResult<QSet<int> > GetAuthorsForTags(QStringList tags, QSqlDatabase db){
    std::string qs = "select distinct author_id from ficauthors ";
    qs += fmt::format(" where fic_id in (select distinct fic_id from fictags where tag in ('{0}'))", tags.join("','").toStdString());
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("author_id", std::move(qs));
    return ctx.result;
}

DiagnosticSQLResult<QHash<QString, int> > GetTagSizes(QStringList tags, QSqlDatabase db)
{
    std::string qs = "select tag, count(tag) as count_tags from fictags {0} group by tag ";
    if(tags.size() > 0)
        qs = fmt::format(qs, "where tag in ('" + tags.join("','").toStdString() + "')");
    else
        qs = fmt::format(qs,"");

    SqlContext<QHash<QString, int>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data.insert(q.value("tag").toString(),q.value("count_tags").toInt());
    });
    return ctx.result;
}

DiagnosticSQLResult<bool> RemoveTagsFromEveryFic(QStringList tags, QSqlDatabase db)
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



DiagnosticSQLResult<QHash<int, core::FanficCompletionStatus> > GetSnoozeInfo(QSqlDatabase db)
{
    std::string qs = "select id, ffn_id, complete, chapters from fanfics where cfInFicSelection(id) > 0";
    SqlContext<QHash<int, core::FanficCompletionStatus>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](QSqlQuery& q){
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
    return ctx.result;
}

DiagnosticSQLResult<QHash<int, core::FanficSnoozeStatus>> GetUserSnoozeInfo(bool fetchExpired, bool limitedSelection, QSqlDatabase db){
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
    ctx.ForEachInSelect([&](QSqlQuery& q){
        core::FanficSnoozeStatus info;
        info.ficId =                q.value("fic_id").toInt();
        info.added =                q.value("snooze_added").toDateTime();
        info.expired =              q.value("expired").toBool();
        info.untilFinished =        q.value("snoozed_until_finished").toInt();
        info.snoozedAtChapter=      q.value("snoozed_at_chapter").toInt();
        info.snoozedTillChapter =   q.value("snoozed_till_chapter").toInt();

        ctx.result.data[info.ficId] = info;
    });
    return ctx.result;
}


DiagnosticSQLResult<QHash<int, QString>> GetNotesForFics(bool limitedSelection , QSqlDatabase db){
    std::string qs = "select * from ficnotes {0} order by fic_id asc";

    if(limitedSelection)
        qs = fmt::format(qs, " where cfInFicSelection(fic_id) > 0 ");
    else
        qs = fmt::format(qs,"");

    SqlContext<QHash<int, QString>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data[q.value("fic_id").toInt()] = q.value("note_content").toString();
    });
    return ctx.result;
}

DiagnosticSQLResult<QHash<int, int>> GetReadingChaptersForFics(bool limitedSelection, QSqlDatabase db)
{
    std::string qs = "select * from FicReadingTracker {0} order by fic_id asc";

    if(limitedSelection)
        qs = fmt::format(qs, " where cfInFicSelection(fic_id) > 0 ");
    else
        qs = fmt::format(qs, "");

    SqlContext<QHash<int, int>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data[q.value("fic_id").toInt()] = q.value("at_chapter").toInt();
    });
    return ctx.result;
}

DiagnosticSQLResult<bool> WriteExpiredSnoozes(QSet<int> data,QSqlDatabase db){
    std::string qs = "update ficsnoozes set expired = 1 where fic_id = :fic_id";
    SqlContext<bool> ctx(db, std::move(qs));
    for(auto ficId : data)
    {
        ctx.bindValue("fic_id", ficId);
        if(!ctx.ExecAndCheck())
            break;
    }
    return ctx.result;
}

DiagnosticSQLResult<bool> SnoozeFic(const core::FanficSnoozeStatus& data,QSqlDatabase db){
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
    return ctx.result;
}

DiagnosticSQLResult<bool> RemoveSnooze(int fic_id,QSqlDatabase db){
    std::string qs = "delete from FicSnoozes where fic_id = :fic_id";
    return SqlContext<bool> (db, std::move(qs), BP1(fic_id))();
}

DiagnosticSQLResult<bool> AddNoteToFic(int fic_id, QString note, QSqlDatabase db)
{
    std::string qs = "INSERT INTO ficnotes(fic_id, note_content, updated) values(:fic_id, :note, date('now')) "
                 "on conflict (fic_id) do update set note_content = :note_, updated = date('now') where fic_id = :fic_id_";
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("fic_id", fic_id);
    ctx.bindValue("note", note);
    ctx.bindValue("note_", note);
    ctx.bindValue("fic_id_", fic_id);
    ctx.ExecAndCheck();
    return ctx.result;
}

DiagnosticSQLResult<bool> RemoveNoteFromFic(int fic_id, QSqlDatabase db)
{
    std::string qs = "delete from ficnotes where fic_id = :fic_id";
    return SqlContext<bool> (db, std::move(qs), BP1(fic_id))();
}


DiagnosticSQLResult<QVector<int> > GetAllFicsThatDontHaveDBID(QSqlDatabase db)
{
    std::string qs = "select distinct ffn_id from fictags where fic_id < 1";
    SqlContext<QVector<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("ffn_id", std::move(qs));
    return ctx.result;
}

DiagnosticSQLResult<bool> FillDBIDsForFics(QVector<core::Identity> pack, QSqlDatabase db)
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
    return ctx.result;
}

DiagnosticSQLResult<bool> FetchTagsForFics(QVector<core::Fanfic> * fics, QSqlDatabase db)
{
    std::string qs = "select fic_id,  group_concat(tag, ' ')  as tags from fictags where cfInSourceFics(fic_id) > 0 group by fic_id";
    QHash<int, QString> tags;
    auto* data= ThreadData::GetRecommendationData();
    auto& hash = data->sourceFics;

    for(const auto& fic : std::as_const(*fics))
        hash.insert(fic.identity.id);

    SqlContext<bool> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        tags[q.value("fic_id").toInt()] = q.value("tags").toString();
    });
    for(auto& fic : *fics)
        fic.userData.tags = tags[fic.identity.id];
    return ctx.result;
}


template <typename T1, typename T2>
inline double DivideAsDoubles(T1 arg1, T2 arg2){
    return static_cast<double>(arg1)/static_cast<double>(arg2);
}
DiagnosticSQLResult<bool> FetchRecommendationsBreakdown(QVector<core::Fanfic> * fics, int listId, QSqlDatabase db)
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
    ctx.ForEachInSelect([&](QSqlQuery& q){
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


        breakdownCounts[ficId].push_back(q.value("votes_common").toString());
        breakdownCounts[ficId].push_back(q.value("votes_uncommon").toString());
        breakdownCounts[ficId].push_back(q.value("votes_rare").toString());
        breakdownCounts[ficId].push_back(q.value("votes_unique").toString());


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
    return ctx.result;
}


DiagnosticSQLResult<bool> FetchRecommendationScoreForFics(QHash<int, int>& scores, const core::ReclistFilter &filter, QSqlDatabase db)
{
    // need to create a list of ids to query for
    QStringList ids;
    ids.reserve(scores.size());
    auto it = scores.begin();
    auto itEnd = scores.end();
    while(it != itEnd){
        ids.push_back(QString::number(it.key()));
        it++;
    }

    std::string qs = "select fic_id, {0} from RecommendationListData where list_id = :list_id and fic_id in ({1})";
    std::string pointsField = filter.scoreType == core::StoryFilter::st_points ? "match_count" : "no_trash_score";
    qs = fmt::format(qs, pointsField, ids.join(",").toStdString());

    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("list_id", filter.mainListId);

    ctx.ForEachInSelect([&](QSqlQuery& q){
        scores[q.value("fic_id").toInt()] = q.value(QString::fromStdString(pointsField)).toInt();
    });
    return ctx.result;


}

DiagnosticSQLResult<bool> LoadPlaceAndRecommendationsData(QVector<core::Fanfic> *fics, const core::ReclistFilter& filter, QSqlDatabase db)
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
    ctx.ForEachInSelect([&](QSqlQuery& q){
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
    return ctx.result;
}

DiagnosticSQLResult<QSharedPointer<core::RecommendationList>> FetchParamsForRecList(int id, QSqlDatabase db)
{
    std::string qs = " select * from recommendationlists where id = :id ";
    SqlContext<QSharedPointer<core::RecommendationList>> ctx(db, std::move(qs), BP1(id));
    //QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data = QSharedPointer<core::RecommendationList>{new core::RecommendationList};
        ctx.result.data->id = q.value("id").toInt();
        ctx.result.data->name = q.value("name").toString();
        ctx.result.data->maxUnmatchedPerMatch = q.value("pick_ratio").toInt();
        ctx.result.data->alwaysPickAt = q.value("always_pick_at").toInt();
        ctx.result.data->minimumMatch = q.value("minimum").toInt();
        ctx.result.data->useWeighting = q.value("use_weighting").toBool();
        ctx.result.data->useMoodAdjustment= q.value("use_mood_adjustment").toBool();
        ctx.result.data->useDislikes = q.value("use_dislikes").toBool();
        ctx.result.data->useDeadFicIgnore= q.value("use_dead_fic_ignore").toBool();
        ctx.result.data->isAutomatic = q.value("is_automatic").toBool();
    });
    return ctx.result;
}

DiagnosticSQLResult<bool> SetFicsAsListOrigin(QVector<int> ficIds, int list_id, QSqlDatabase db)
{
    std::string qs = "update RecommendationListData set is_origin = 1 where fic_id = :fic_id and list_id = :list_id";
    SqlContext<bool> ctx(db, std::move(qs), BP1(list_id));
    ctx.ExecuteWithValueList<int>("fic_id", ficIds);
    return ctx.result;
}

DiagnosticSQLResult<bool> DeleteLinkedAuthorsForAuthor(int author_id,  QSqlDatabase db)
{
    std::string qs = "delete from LinkedAuthors where recommender_id = :author_id";
    return SqlContext<bool> (db, std::move(qs), BP1(author_id))();
}

DiagnosticSQLResult<bool>  UploadLinkedAuthorsForAuthor(int author_id, QString website, QList<int> ids, QSqlDatabase db)
{
    std::string qs = fmt::format("insert into  LinkedAuthors(recommender_id, {0}_id) values(:author_id, :id)",website.toStdString());
    SqlContext<bool> ctx(db, std::move(qs), BP1(author_id));
    ctx.ExecuteWithValueList<int>("id", ids, true);
    return ctx.result;
}

DiagnosticSQLResult<QVector<int> > GetAllUnprocessedLinkedAuthors(QSqlDatabase db)
{
    std::string qs = "select distinct ffn_id from linkedauthors where ffn_id not in (select ffn_id from recommenders)";
    SqlContext<QVector<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("ffn_id", std::move(qs));
    return ctx.result;
}



DiagnosticSQLResult<QStringList> GetLinkedPagesForList(int list_id, QString website, QSqlDatabase db)
{
    std::string qs = "Select distinct {0}_id from LinkedAuthors "
                         " where recommender_id in ( select author_id from RecommendationListAuthorStats where list_id = {1}) "
                         " and {0}_id not in (select distinct {0}_id from recommenders)"
                         " union all "
                         " select ffn_id from recommenders where id not in (select distinct recommender_id from recommendations) and favourites != 0 ";
    qs=fmt::format(qs, website.toStdString(), list_id);

    SqlContext<QStringList> ctx(db, std::move(qs), BP1(list_id));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        auto authorUrl = url_utils::GetAuthorUrlFromWebId(q.value(QString("{0}_id").arg(website)).toInt(), "ffn");
        ctx.result.data.push_back(authorUrl);
    });
    return ctx.result;
}

DiagnosticSQLResult<bool> RemoveAuthorRecommendationStatsFromDatabase(int list_id, int author_id, QSqlDatabase db)
{
    std::string qs = "delete from recommendationlistauthorstats "
                         " where list_id = :list_id and author_id = :author_id";
    return SqlContext<bool>(db, std::move(qs), BP2(list_id,author_id))();
}

DiagnosticSQLResult<bool> CreateFandomIndexRecord(int id, QString name, QSqlDatabase db)
{
    std::string qs = "insert into fandomindex(id, name) values(:id, :name)";
    return SqlContext<bool>(db, std::move(qs), BP2(name, id))();
}



DiagnosticSQLResult<QHash<int, QList<int>>> GetWholeFicFandomsTable(QSqlDatabase db)
{
    std::string qs = "select fic_id, fandom_id from ficfandoms";
    SqlContext<QHash<int, QList<int>>> ctx(db, std::move(qs));
    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data[q.value("fandom_id").toInt()].push_back(q.value("fic_id").toInt());
    });
    return ctx.result;
}

DiagnosticSQLResult<bool> EraseFicFandomsTable(QSqlDatabase db)
{
    std::string qs = "delete from ficfandoms";
    return SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> SetLastUpdateDateForFandom(int id, QDate updated, QSqlDatabase db)
{
    std::string qs = "update fandomindex set updated = :updated where id = :id";
    return SqlContext<bool>(db, std::move(qs), BP2(updated, id))();
}

DiagnosticSQLResult<bool> RemoveFandomFromRecentList(QString name, QSqlDatabase db)
{
    std::string qs = "delete from recent_fandoms where fandom = :name";
    return SqlContext<bool>(db, std::move(qs), BP1(name))();
}


DiagnosticSQLResult<int> GetLastExecutedTaskID(QSqlDatabase db)
{
    std::string qs = "select max(id) as maxid from pagetasks";
    SqlContext<int>ctx(db, std::move(qs));
    ctx.FetchSingleValue<int>("maxid", -1);
    return ctx.result;
}

// new query limit
DiagnosticSQLResult<bool> GetTaskSuccessByID(int id, QSqlDatabase db)
{
    std::string qs = "select success from pagetasks where id = :id";
    SqlContext<bool>ctx(db, std::move(qs), BP1(id));
    ctx.FetchSingleValue<bool>("success", false);
    return ctx.result;
}
DiagnosticSQLResult<bool>  IsForceStopActivated(int id, QSqlDatabase db)
{
    std::string qs = "select force_stop from pagetasks where id = :id";
    SqlContext<bool>ctx(db, std::move(qs), BP1(id));
    ctx.FetchSingleValue<bool>("force_stop", false);
    return ctx.result;
}


void FillPageTaskBaseFromQuery(BaseTaskPtr task, const QSqlQuery& q){
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


void FillPageTaskFromQuery(PageTaskPtr task, const QSqlQuery& q){
    if(!NullPtrGuard(task))
        return;

    FillPageTaskBaseFromQuery(task, q);
    task->id= q.value("id").toInt();
    task->parts = q.value("parts").toInt();
    task->results = q.value("results").toString();
    task->taskComment= q.value("task_comment").toString();
    task->size = q.value("task_size").toInt();
    task->allowedRetries = q.value("allowed_retry_count").toInt();
    task->cacheMode = static_cast<ECacheMode>(q.value("cache_mode").toInt());
    task->refreshIfNeeded= q.value("refresh_if_needed").toBool();
}

void FillSubTaskFromQuery(SubTaskPtr task, const QSqlQuery& q){
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
        cast->authors = q.value("content").toString().split("\n");
    }
    if(task->type == 1)
    {
        content = SubTaskFandomContent::NewContent();
        auto cast = dynamic_cast<SubTaskFandomContent*>(content.data());
        cast->urlLinks = q.value("content").toString().split("\n");
        cast->fandom = q.value("custom_data1").toString();
        cast->fandom = q.value("custom_data1").toString();
    }
    task->updateLimit = q.value("parse_up_to").toDateTime();
    task->content = content;
    task->isValid = true;
    task->isNew =false;
}

DiagnosticSQLResult<PageTaskPtr> GetTaskData(int id, QSqlDatabase db)
{
    std::string qs = "select * from pagetasks where id = :id";
    SqlContext<PageTaskPtr>ctx(db, std::move(qs), BP1(id));
    ctx.result.data = PageTask::CreateNewTask();
    if(!ctx.ExecAndCheckForData())
        return ctx.result;

    FillPageTaskFromQuery(ctx.result.data, ctx.q);
    return ctx.result;
}

DiagnosticSQLResult<SubTaskList> GetSubTaskData(int id, QSqlDatabase db)
{
    std::string qs = "select * from PageTaskParts where task_id = :id";

    SqlContext<SubTaskList>ctx(db, std::move(qs), {{"id", id}});
    return ctx.ForEachInSelect([&](QSqlQuery& q){
        auto subtask = PageSubTask::CreateNewSubTask();
        FillSubTaskFromQuery(subtask, q);
        ctx.result.data.push_back(subtask); });
}

void FillPageFailuresFromQuery(PageFailurePtr failure, const QSqlQuery& q){
    if(!NullPtrGuard(failure))
        return;

    failure->action = QSharedPointer<PageTaskAction>(new PageTaskAction{q.value("action_uuid").toString(),
                                                                        q.value("task_id").toInt(),
                                                                        q.value("sub_id").toInt()});
    failure->attemptTimeStamp = q.value("process_attempt").toDateTime();
    failure->errorCode  = static_cast<PageFailure::EFailureReason>(q.value("error_code").toInt());
    failure->errorlevel  = static_cast<PageFailure::EErrorLevel>(q.value("error_level").toInt());
    failure->lastSeen = q.value("last_seen_at").toDateTime();
    failure->url = q.value("url").toString();
    failure->error = q.value("error").toString();
}

void FillActionFromQuery(PageTaskActionPtr action, const QSqlQuery& q){
    if(!NullPtrGuard(action))
        return;

    action->SetData(q.value("action_uuid").toString(),
                    q.value("task_id").toInt(),
                    q.value("sub_id").toInt());

    action->success = q.value("success").toBool();
    action->started = q.value("started_at").toDateTime();
    action->finished = q.value("finished_at").toDateTime();
    action->isNewAction = false;
}

DiagnosticSQLResult<SubTaskErrors> GetErrorsForSubTask(int id,  QSqlDatabase db, int subId)
{
    std::string qs = "select * from PageWarnings where task_id = :id";
    bool singleSubTask = subId != -1;
    if(singleSubTask)
        qs+= " and sub_id = :sub_id";

    SqlContext<SubTaskErrors>ctx(db, std::move(qs), BP1(id));

    if(singleSubTask)
        ctx.bindValue("sub_id", subId);

    ctx.ForEachInSelect([&](QSqlQuery& q){
        auto failure = PageFailure::CreateNewPageFailure();
        FillPageFailuresFromQuery(failure, q);
        ctx.result.data.push_back(failure);
    });
    return ctx.result;
}

DiagnosticSQLResult<PageTaskActions> GetActionsForSubTask(int id, QSqlDatabase db, int subId)
{

    std::string qs = "select * from PageTaskActions where task_id = :id";
    bool singleSubTask = subId != -1;
    if(singleSubTask)
        qs+= " and sub_id = :sub_id";
    SqlContext<PageTaskActions>ctx(db, std::move(qs), BP1(id));

    if(singleSubTask)
        ctx.bindValue("sub_id", subId);

    ctx.ForEachInSelect([&](QSqlQuery& q){
        auto action = PageTaskAction::CreateNewAction();
        FillActionFromQuery(action, q);
        ctx.result.data.push_back(action);
    });

    return ctx.result;
}

DiagnosticSQLResult<int> CreateTaskInDB(PageTaskPtr task, QSqlDatabase db)
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
    ctx.bindValue("cache_mode", static_cast<int>(task->cacheMode));
    ctx.bindValue("refresh_if_needed", task->refreshIfNeeded);
    ctx.bindValue("task_comment", task->taskComment);
    ctx.bindValue("task_size", task->size);
    ctx.bindValue("parsed_pages",      task->parsedPages);
    ctx.bindValue("updated_fics",      task->updatedFics);
    ctx.bindValue("inserted_fics",     task->addedFics);
    ctx.bindValue("inserted_authors",  task->addedAuthors);
    ctx.bindValue("updated_authors",   task->updatedAuthors);
    if(!ctx.ExecAndCheck())
        return ctx.result;

    ctx.ReplaceQuery("select max(id) as maxid from PageTasks");
    ctx.FetchSingleValue<int>("maxid", -1);
    return ctx.result;

}

DiagnosticSQLResult<bool> CreateSubTaskInDB(SubTaskPtr subtask, QSqlDatabase db)
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

DiagnosticSQLResult<bool> CreateActionInDB(PageTaskActionPtr action, QSqlDatabase db)
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

DiagnosticSQLResult<bool> CreateErrorsInDB(const SubTaskErrors& errors, QSqlDatabase db)
{
    std::string qs = "insert into PageWarnings(action_uuid, task_id, sub_id, url, attempted_at, last_seen_at, error_code, error_level, error) "
                         "values(:action_uuid, :task_id, :sub_id, :url, :attempted_at, :last_seen, :error_code, :error_level, :error) ";

    SqlContext<bool>ctx(db, std::move(qs));
    ctx.ExecuteWithKeyListAndBindFunctor<PageFailurePtr>(errors, [](PageFailurePtr error, QSqlQuery& q){
        q.bindValue(":action_uuid", error->action->id.toString());
        q.bindValue(":task_id", error->action->taskId);
        q.bindValue(":sub_id", error->action->subTaskId);
        q.bindValue(":url", error->url);
        q.bindValue(":attempted_at", error->attemptTimeStamp);
        q.bindValue(":last_seen", error->lastSeen);
        q.bindValue(":error_code", static_cast<int>(error->errorCode));
        q.bindValue(":error_level", static_cast<int>(error->errorlevel));
        q.bindValue(":error", error->error);
    });
    return ctx.result;
}

DiagnosticSQLResult<bool> UpdateTaskInDB(PageTaskPtr task, QSqlDatabase db)
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

DiagnosticSQLResult<bool> UpdateSubTaskInDB(SubTaskPtr task, QSqlDatabase db)
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

DiagnosticSQLResult<bool> SetTaskFinished(int id, QSqlDatabase db)
{
    return SqlContext<bool> (db, "update PageTasks set finished = 1 where id = :task_id",{
                                 {":task_id", id}
                             })();
}

DiagnosticSQLResult<TaskList> GetUnfinishedTasks(QSqlDatabase db)
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
DiagnosticSQLResult<bool> ExportTagsToDatabase(QSqlDatabase originDB, QSqlDatabase targetDB)
{
    thread_local auto idHash = GetGlobalIDHash(originDB, " where id in (select distinct fic_id from fictags)").data;
    {

        QList<std::string> targetKeyList = {"ffn_id","ao3_id","sb_id","sv_id","tag",};
        QList<std::string> sourceKeyList = {"ffn","ao3","sb","sv","tag",};
        std::string insertQS = "insert into UserFicTags(ffn_id, ao3_id, sb_id, sv_id, tag) values(:ffn_id, :ao3_id, :sb_id, :sv_id, :tag)";
        ParallelSqlContext<bool> ctx (originDB, "select fic_id, tag from fictags order by fic_id, tag", std::move(sourceKeyList),
                                      targetDB, std::move(insertQS), std::move(targetKeyList));

        auto keyConverter = [&](const std::string& sourceKey, QSqlQuery q, QSqlDatabase , auto& )->QVariant
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
            return ctx.result;
    }
    {
        QList<std::string> keyList = {"tag","id"};
        std::string insertQS = "insert into UserTags(fic_id, tag) values(:id, :tag)";
        ParallelSqlContext<bool> ctx (originDB, "select * from tags", std::move(keyList),
                                      targetDB, std::move(insertQS), std::move(keyList));
        return ctx();
    }
}

// !!!! requires careful testing!
DiagnosticSQLResult<bool> ImportTagsFromDatabase(QSqlDatabase currentDB,QSqlDatabase tagImportSourceDB)
{
    {
        SqlContext<bool> ctxTarget(currentDB, std::list<std::string>{"delete from fictags","delete from tags"});
        if(!ctxTarget.result.success)
            return ctxTarget.result;
    }

    {
        QList<std::string> keyList = {"tag","id"};
        std::string insertQS = "insert into Tags(id, tag) values(:id, :tag)";
        ParallelSqlContext<bool> ctx (tagImportSourceDB, "select * from UserTags", std::move(keyList),
                                      currentDB, std::move(insertQS), std::move(keyList));
        ctx();
        if(!ctx.Success())
            return ctx.result;
    }
    // need to ensure fic_id in the source database
    SqlContext<bool> ctxTarget(tagImportSourceDB, std::list<std::string>{"alter table UserFicTags add column fic_id integer default -1"});
    ctxTarget();
    if(!ctxTarget.result.success)
        return ctxTarget.result;

    //    SqlContext<bool> ctxTest(currentDB, QStringList{"insert into FicTags(fic_id, ffn_id, ao3_id, sb_id, sv_id, tag)"
    //                                                    " values(-1, -1, -1, -1, -1, -1)"});
    //    ctxTest();
    //    if(!ctxTest.result.success)
    //        return ctxTest.result;

    QList<std::string> keyList = {"fic_id", "ffn_id", "ao3_id", "sb_id", "sv_id", "tag"};
    std::string insertQS = "insert into FicTags(fic_id, ffn_id, ao3_id, sb_id, sv_id, tag) values(:fic_id, :ffn_id, :ao3_id, :sb_id, :sv_id, :tag)";
    //bool isOpen = currentDB.isOpen();
    ParallelSqlContext<bool> ctx (tagImportSourceDB, "select * from UserFicTags", std::move(keyList),
                                  currentDB, std::move(insertQS), std::move(keyList));
    return ctx();
}


DiagnosticSQLResult<bool> ExportSlashToDatabase(QSqlDatabase originDB, QSqlDatabase targetDB)
{

    QList<std::string> keyList = {"ffn_id","keywords_result","keywords_yes","keywords_no","filter_pass_1", "filter_pass_2"};
    std::string insertQS = "insert into slash_data_ffn(ffn_id, keywords_result, keywords_yes, keywords_no, filter_pass_1, filter_pass_2) "
                               " values(:ffn_id, :keywords_result, :keywords_yes, :keywords_no, :filter_pass_1, :filter_pass_2) ";

    return ParallelSqlContext<bool> (originDB, "select * from slash_data_ffn", std::move(keyList),
                                     targetDB, std::move(insertQS), std::move(keyList))();
}

DiagnosticSQLResult<bool> ImportSlashFromDatabase(QSqlDatabase slashImportSourceDB, QSqlDatabase appDB)
{

    {
        SqlContext<bool> ctxTarget(appDB, "delete from slash_data_ffn");
        if(ctxTarget.ExecAndCheck())
            return ctxTarget.result;
    }

    QList<std::string> keyList = {"ffn_id","keywords_result","keywords_yes","keywords_no","filter_pass_1", "filter_pass_2"};
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

DiagnosticSQLResult<int> FanficIdRecord::CreateRecord(QSqlDatabase db) const
{
    std::string query = "INSERT INTO FANFICS (ffn_id, sb_id, sv_id, ao3_id, for_fill, lastupdate) "
                    "VALUES ( :ffn_id, :sb_id, :sv_id, :ao3_id, 1, date('now'))";

    SqlContext<int> ctx(db, std::move(query),
    {{"ffn_id", ids["ffn"]},
     {"sb_id", ids["sb"]},
     {"sv_id", ids["sv"]},
     {"ao3_id", ids["ao3"]},});

    if(!ctx.ExecAndCheck())
        return ctx.result;

    ctx.ReplaceQuery("select max(id) as mid from fanfics");
    ctx.FetchSingleValue<int>("mid", 0);
    return ctx.result;
}

DiagnosticSQLResult<bool> AddFandomLink(int oldId, int newId, QSqlDatabase db)
{
    SqlContext<bool> ctx(db, "select * from fandoms where id = :old_id",
    {{"old_id", oldId}});
    if(!ctx.ExecAndCheckForData())
        return ctx.result;

    QStringList urls;
    urls << ctx.trimmedValue("normal_url");
    urls << ctx.trimmedValue("crossover_url");

    urls.removeAll("");
    urls.removeAll("none");

    QString custom = ctx.trimmedValue("section");

    ctx.ReplaceQuery("insert into fandomurls (global_id, url, website, custom) values(:new_id, :url, 'ffn', :custom)");
    ctx.bindValue("custom", custom);
    ctx.bindValue("new_id",newId);
    ctx.ExecuteWithKeyListAndBindFunctor<QString>(urls, [](QString url, QSqlQuery& q){
        q.bindValue(":url", url);
    });
    return ctx.result;
}

DiagnosticSQLResult<bool> WriteAuthorFavouriteStatistics(core::AuthorPtr author, QSqlDatabase db)
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

    return ctx.result;
}


DiagnosticSQLResult<bool> WriteAuthorFavouriteGenreStatistics(core::AuthorPtr author, QSqlDatabase db)
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
    auto converter = interfaces::GenreConverter::Instance();
    ctx.ProcessKeys<QString>(interfaces::GenreConverter::Instance().GetCodeGenres(), [&](auto key, auto& q){
        q.bindValue(":" + converter.ToDB(key), genreFactors[key]);
    });
    ctx.ExecAndCheck(true);
    return ctx.result;
}

DiagnosticSQLResult<bool> WriteAuthorFavouriteFandomStatistics(core::AuthorPtr author, QSqlDatabase db)
{
    std::string query = "INSERT INTO AuthorFavouritesFandomRatioStatistics ("
                    "author_id, fandom_id, fandom_ratio, fic_count) "
                    "VALUES ("
                    ":author_id, :fandom_id, :fandom_ratio, :fic_count"
                    ")";

    SqlContext<bool> ctx(db, std::move(query));
    ctx.bindValue("author_id",author->id);

    ctx.ExecuteWithKeyListAndBindFunctor<int>(author->stats.favouriteStats.fandomFactorsConverted.keys(), [&](auto key, auto& q){
        q.bindValue(":fandom_id", key);
        q.bindValue(":fandom_ratio", author->stats.favouriteStats.fandomFactorsConverted[key]);
        q.bindValue(":fic_count", author->stats.favouriteStats.fandomsConverted[key]);
    });

    return ctx.result;
}


DiagnosticSQLResult<bool> WipeAuthorStatistics(core::AuthorPtr author, QSqlDatabase db)
{
    SqlContext<bool> ctx(db);
    ctx.bindValue("author_id",author->id);
    ctx.ExecuteList({"delete from AuthorFavouritesGenreStatistics where author_id = :author_id",
                     "delete from AuthorFavouritesStatistics where author_id = :author_id",
                     "delete from AuthorFavouritesFandomRatioStatistics where author_id = :author_id"
                    });
    return ctx.result;
}

DiagnosticSQLResult<QList<int>> GetAllAuthorRecommendations(int id, QSqlDatabase db)
{
    SqlContext<QList<int>> ctx(db);

    ctx.bindValue("id",id);
    ctx.FetchLargeSelectIntoList<int>("fic_id",
                                      "select fic_id from recommendations where recommender_id = :id",
                                      "select count(recommender_id) from recommendations where recommender_id = :id");
    return ctx.result;
}

DiagnosticSQLResult<QSet<int> > GetAllKnownSlashFics(QSqlDatabase db) //todo wrong table
{
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("fic_id",
                                      "select fic_id from algopasses where keywords_pass_result = 1",
                                      "select count(fic_id) from algopasses where keywords_pass_result = 1");

    return ctx.result;
}


DiagnosticSQLResult<QSet<int> > GetAllKnownNotSlashFics(QSqlDatabase db) //todo wrong table
{
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("fic_id",
                                      "select fic_id from algopasses where keywords_no = 1",
                                      "select count(fic_id) from algopasses where keywords_no = 1");

    return ctx.result;
}

DiagnosticSQLResult<QSet<int> > GetSingularFicsInLargeButSlashyLists(QSqlDatabase db)
{
    std::string qs = "select fic_id from "
                         " ( "
                         " select fic_id, count(fic_id) as cnt from recommendations where recommender_id in (select author_id from AuthorFavouritesStatistics where slash_factor > 0.5 and slash_factor < 0.85  and favourites > 1000) and fic_id not in ( "
                         " select distinct fic_id from recommendations where recommender_id in (select author_id from AuthorFavouritesStatistics where slash_factor <= 0.5 or slash_factor > 0.85) ) group by fic_id "
                         " ) "
                         " where cnt = 1 ";
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("fic_id",std::move(qs));

    return ctx.result;
}

DiagnosticSQLResult<QHash<int, double> > GetDoubleValueHashForFics(QString fieldName, QSqlDatabase db)
{
    SqlContext<QHash<int, double>> ctx(db);
    std::string qs = fmt::format("select id, {0} from fanfics order by id",fieldName.toStdString());
    ctx.FetchSelectIntoHash(std::move(qs), "id", fieldName.toStdString());
    return ctx.result;
}

DiagnosticSQLResult<QHash<int, QString> >GetGenreForFics(QSqlDatabase db)
{
    SqlContext<QHash<int, QString>> ctx(db);
    std::string qs = fmt::format("select id, {0} from fanfics order by id","genres");
    ctx.FetchSelectIntoHash(std::move(qs), "id", "genres");
    return ctx.result;
}

DiagnosticSQLResult<QHash<int, int>> GetScoresForFics(QSqlDatabase db)
{
    SqlContext<QHash<int, int>> ctx(db);
    std::string qs = "select fic_id, score from ficscores order by fic_id asc";
    ctx.FetchSelectIntoHash(std::move(qs), "fic_id", "score");
    return ctx.result;
}

DiagnosticSQLResult<QHash<int, std::array<double, genreArraySize>>> GetGenreData(QString keyName, std::string&& query, QSqlDatabase db)
{

    SqlContext<QHash<int, std::array<double, genreArraySize>>> ctx(db);
    ctx.FetchSelectFunctor(std::move(query), DATAQ{
                               std::size_t counter = 0;
//                               qDebug() << "Loading list for author: " << q.value(keyName).toInt();
//                               qDebug() << "Adventure value is: " << q.value("Adventure").toDouble();
                               auto key = q.value(keyName).toInt();
                               auto& dataElement = data[key];
                               dataElement.at(counter++) =q.value(QStringLiteral("General_")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Humor")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Poetry")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Adventure")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Mystery")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Horror")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Parody")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Angst")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Supernatural")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Suspense")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Romance")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("NoGenre")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("SciFi")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Fantasy")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Spiritual")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Tragedy")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Western")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Crime")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Family")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("HurtComfort")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Friendship")).toDouble();
                               dataElement.at(counter++) =q.value(QStringLiteral("Drama")).toDouble();
                           });

    return ctx.result;
}

DiagnosticSQLResult<QHash<int, std::array<double, genreArraySize>>> GetListGenreData(QSqlDatabase db)
{
    return GetGenreData("author_id", "select * from AuthorFavouritesGenreStatistics "
                                     " where author_id  in (select distinct recommender_id from recommendations) "
                                     " order by author_id asc", db);
}
DiagnosticSQLResult<QHash<int, std::array<double, genreArraySize> > > GetFullFicGenreData(QSqlDatabase db)
{
    return GetGenreData("fic_id", "select * from FicGenreStatistics order by fic_id", db);
}

DiagnosticSQLResult<QHash<int, double> > GetFicGenreData(QString genre, QString cutoff, QSqlDatabase db)
{
    SqlContext<QHash<int, double>> ctx(db);
    std::string qs = "select fic_id, {0} from FicGenreStatistics where {1} order by fic_id";
    qs = fmt::format(qs, genre.toStdString(),cutoff.toStdString());
    ctx.FetchSelectIntoHash(std::move(qs), "fic_id", genre.toStdString());
    return ctx.result;
}

DiagnosticSQLResult<QSet<int> > GetAllKnownFicIds(QString where, QSqlDatabase db)
{
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("id",
                                      "select id from fanfics where " + where.toStdString(),
                                      "select count(*) from fanfics where " + where.toStdString());

    return ctx.result;
}
DiagnosticSQLResult<QSet<int>>  GetFicIDsWithUnsetAuthors(QSqlDatabase db)
{
    SqlContext<QSet<int>> ctx(db);
    ctx.FetchLargeSelectIntoList<int>("fic_id",
                                      "select distinct fic_id from fictags where fic_id not in (select distinct fic_id from ficauthors)");

    return ctx.result;
}

DiagnosticSQLResult<QVector<core::FicWeightPtr> > GetAllFicsWithEnoughFavesForWeights(int faves, QSqlDatabase db)
{
    SqlContext<QVector<core::FicWeightPtr>> ctx(db);
    std::string qs = fmt::format("select id,Rated, author_id, complete, updated, fandom1,fandom2,favourites, published, updated,  genres, reviews, filter_pass_1, wordcount"
                         "  from fanfics where favourites > {0} order by id", faves);
    ctx.FetchSelectFunctor(std::move(qs), DATAQ{
                               auto fw = getFicWeightPtrFromQuery(q);
                               data.push_back(fw);
                           });
    return ctx.result;
}


DiagnosticSQLResult<QHash<int, core::AuthorFavFandomStatsPtr>> GetAuthorListFandomStatistics(QList<int> authors, QSqlDatabase db)
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
            result.oracleError = ctx.result.oracleError;
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
                                                         QSqlDatabase db)
{
    std::string qs = "INSERT INTO RecommendationListData ("
                 "fic_id, list_id, match_count) "
                 "VALUES ("
                 ":fic_id, :list_id, :match_count)";

    SqlContext<bool> ctx(db, std::move(qs));
    ctx.bindValue("list_id", listId);
    ctx.ExecuteWithArgsHash({"fic_id", "match_count"}, fics);
    return ctx.result;
}


DiagnosticSQLResult<bool> CreateSlashInfoPerFic(QSqlDatabase db)
{
    SqlContext<bool> ctx(db, "insert into algopasses(fic_id) select id from fanfics");
    return ctx(true);
}

DiagnosticSQLResult<bool> WipeSlashMetainformation(QSqlDatabase db)
{
    std::string qs = "update algopasses set "
                 " keywords_yes = 0, keywords_no = 0, keywords_pass_result = 0, "
                 " pass_1 = 0, pass_2 = 0,pass_3 = 0,pass_4 = 0,pass_5 = 0 ";

    return SqlContext<bool>(db, std::move(qs))();
}


DiagnosticSQLResult<bool> ProcessSlashFicsBasedOnWords(std::function<SlashPresence(QString, QString, QString)> func, QSqlDatabase db)
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
    QSqlQuery q(db);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    do
    {
        auto result = func(q.value(QStringLiteral("summary")).toString(),
                           q.value(QStringLiteral("characters")).toString(),
                           q.value(QStringLiteral("fandom")).toString());
        if(result.containsSlash)
            slashKeywords.insert(q.value(QStringLiteral("id")).toInt());
        if(result.IsSlash())
            slashFics.insert(q.value(QStringLiteral("id")).toInt());
        if(result.containsNotSlash)
            notSlashFics.insert(q.value(QStringLiteral("id")).toInt());


    }while(q.next());
    qDebug() << QStringLiteral("finished processing slash info");
    q.prepare(QStringLiteral("update algopasses set keywords_pass_result = 1 where fic_id = :id"));
    QList<int> list = slashFics.values();
    int counter = 0;
    for(auto tempId: list)
    {
        counter++;
        q.bindValue(QStringLiteral(":id"), tempId);
        if(!result.ExecAndCheck(q))
        {
            qDebug() << QStringLiteral("failed to write slash");
            return result;
        }
    }
    q.prepare(QStringLiteral("update algopasses set keywords_no = 1 where fic_id = :id"));
    list = notSlashFics.values();
    counter = 0;
    for(auto tempId: std::as_const(list))
    {
        counter++;
        q.bindValue(QStringLiteral(":id"), tempId);
        if(!result.ExecAndCheck(q))
        {
            qDebug() << "failed to write slash";
            return result;
        }
    }
    q.prepare(QStringLiteral("update algopasses set keywords_yes = 1 where fic_id = :id"));
    list = slashKeywords.values();
    counter = 0;
    for(auto tempId: std::as_const(list))
    {
        counter++;
        q.bindValue(QStringLiteral(":id"), tempId);
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

DiagnosticSQLResult<bool> WipeAuthorStatisticsRecords(QSqlDatabase db)
{
    std::string qs = "delete from AuthorFavouritesStatistics";
    return SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> CreateStatisticsRecordsForAuthors(QSqlDatabase db)
{
    std::string qs = "insert into AuthorFavouritesStatistics(author_id, favourites) select r.id, (select count(*) from recommendations where recommender_id = r.id) from recommenders r";
    return SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> CalculateSlashStatisticsPercentages(QString usedField,  QSqlDatabase db)
{
    std::string qs = "update AuthorFavouritesStatistics set slash_factor  = "
                         " cast( (select count (ff.fic_id) from (select fic_id from recommendations where recommender_id = author_id) rs left join  (select fic_id, {0} from algopasses where {0} = 1) ff on ff.fic_id  = rs.fic_id) as float) "
                         "/cast( (select count (ff.fic_id) from (select fic_id from recommendations where recommender_id = author_id) rs left join  (select fic_id, {0} from algopasses)              ff on ff.fic_id  = rs.fic_id) as float)";
    qs = fmt::format(qs, usedField.toStdString());
    return SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> AssignIterationOfSlash(QString iteration, QSqlDatabase db)
{
    std::string qs = "update algopasses set {0} = 1 where keywords_pass_result = 1 or fic_id in "
                         "(select fic_id from recommendationlistdata where list_id in (select id from recommendationlists where name = 'SlashCleaned'))";
    qs = fmt::format(qs, iteration.toStdString());
    return SqlContext<bool>(db, std::move(qs))();
}

DiagnosticSQLResult<bool> WriteFicRecommenderRelationsForRecList(int list_id, QHash<uint32_t, QVector<uint32_t> > relations, QSqlDatabase db)
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
                                                     QSqlDatabase db){
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



DiagnosticSQLResult<bool> PerformGenreAssignment(QSqlDatabase db)
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
    std::list<std::string> list;
    for(auto i = result.begin(); i != result.end(); i++)
        list.push_back(i.key().toStdString());
    SqlContext<bool> ctx(db, std::move(qs));
    ctx.ExecuteWithArgsSubstitution(std::move(list));
    return ctx.result;

}

DiagnosticSQLResult<bool> EnsureUUIDForUserDatabase(QUuid id, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;

    std::string qs = "select count(*) as cnt from user_settings where name = 'db_uuid'";
    SqlContext<int> ctx(db, std::move(qs));
    ctx.FetchSingleValue<int>("cnt", 0);
    if(!ctx.Success())
    {
        result.oracleError = ctx.result.oracleError;
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
        result.oracleError = ctx.result.oracleError;
        result.success = false;
        return result;
    }
    return result;
}

DiagnosticSQLResult<bool> FillFicDataForList(int listId,
                                             const QVector<int> & fics,
                                             const QVector<int> & matchCounts,
                                             const QSet<int> &origins,
                                             QSqlDatabase db)
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
    return ctx.result;
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
        scoresSet.insert(ficData->matchCounts[i]);

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
        scores[ficData->fics[i]] = ficData->matchCounts.at(i);

    std::sort(ficsCopy.begin(), ficsCopy.end(), [&](const int& fic1, const int& fic2){
        return scores[fic1] > scores[fic2] ;
    });
    for(int i = 0; i < ficData->fics.size(); i++){
        positions[ficsCopy[i]] = i+1;
    }
    return positions;
}


DiagnosticSQLResult<bool> FillFicDataForList(QSharedPointer<core::RecommendationList> list,
                                             QSqlDatabase db)
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
        ctx.bindValue("pedestal", scorePedestalPositions[list->ficData->matchCounts.at(i)]);

        ctx.bindValue("matchCount", list->ficData->matchCounts.at(i));
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
    return ctx.result;
}

DiagnosticSQLResult<QString> GetUserToken(QSqlDatabase db)
{
    std::string qs = "Select value from user_settings where name = 'db_uuid'";

    SqlContext<QString> ctx(db, std::move(qs));
    ctx.FetchSingleValue<QString>("value", "");
    return ctx.result;
}

DiagnosticSQLResult<genre_stats::FicGenreData> GetRealGenresForFic(int ficId, QSqlDatabase db)
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

    ctx.ForEachInSelect([&](QSqlQuery& q){
        ctx.result.data.originalGenreString = q.value(QStringLiteral("genres")).toString();
        ctx.result.data.originalGenres = genreConverter.GetFFNGenreList(q.value(QStringLiteral("genres")).toString());

        ctx.result.data.ficId = ficId;
        ctx.result.data.ffnId = q.value(QStringLiteral("ffn_id")).toInt();
        ctx.result.data.totalLists = q.value(QStringLiteral("total")).toInt();

        ctx.result.data.strengthHumor = q.value(QStringLiteral("Div_HumorComposite")).toFloat();
        ctx.result.data.strengthRomance = q.value(QStringLiteral("Div_Flirty")).toFloat();
        ctx.result.data.strengthDrama= q.value(QStringLiteral("Div_Dramatic")).toFloat();
        ctx.result.data.strengthBonds= q.value(QStringLiteral("Div_Bondy")).toFloat();
        ctx.result.data.strengthHurtComfort= q.value(QStringLiteral("Div_Hurty")).toFloat();
        ctx.result.data.strengthNeutralComposite= q.value(QStringLiteral("Div_NeutralComposite")).toFloat();
        ctx.result.data.strengthNeutralAdventure= q.value(QStringLiteral("Div_NeutralSingle")).toFloat();
    });
    return ctx.result;
}

DiagnosticSQLResult<QHash<uint32_t, genre_stats::ListMoodData>> GetMoodDataForLists(QSqlDatabase db)
{
    std::string query = " select * from AuthorMoodStatistics "
                    " where author_id in (select distinct recommender_id from recommendations) "
                    " order by author_id asc ";

    SqlContext<QHash<uint32_t, genre_stats::ListMoodData>> ctx(db, std::move(query));
    auto genreConverter = interfaces::GenreConverter::Instance();
    genre_stats::ListMoodData tmp;
    ctx.ForEachInSelect([&](QSqlQuery& q){
        tmp.listId = q.value(QStringLiteral("author_id")).toInt();

        tmp.strengthBondy = q.value(QStringLiteral("bondy")).toFloat();
        tmp.strengthDramatic= q.value(QStringLiteral("dramatic")).toFloat();
        tmp.strengthFlirty= q.value(QStringLiteral("flirty")).toFloat();
        tmp.strengthFunny= q.value(QStringLiteral("funny")).toFloat();
        tmp.strengthHurty= q.value(QStringLiteral("hurty")).toFloat();
        tmp.strengthNeutral= q.value(QStringLiteral("neutral")).toFloat();

        tmp.strengthNonBondy= q.value(QStringLiteral("nonbondy")).toFloat();
        tmp.strengthNonDramatic= q.value(QStringLiteral("NonDramatic")).toFloat();
        tmp.strengthNonFlirty= q.value(QStringLiteral("NonFlirty")).toFloat();
        tmp.strengthNonFunny= q.value(QStringLiteral("NonFunny")).toFloat();
        tmp.strengthNonHurty= q.value(QStringLiteral("NonHurty")).toFloat();
        tmp.strengthNonNeutral= q.value(QStringLiteral("NonNeutral")).toFloat();
        tmp.strengthNonShocky= q.value(QStringLiteral("NonShocky")).toFloat();
        tmp.strengthNone= q.value(QStringLiteral("None")).toFloat();
        tmp.strengthOther= q.value(QStringLiteral("Other")).toFloat();


        ctx.result.data.insert(tmp.listId, tmp);
    });
    return ctx.result;
}

DiagnosticSQLResult<QVector<genre_stats::FicGenreData>> GetGenreDataForQueuedFics(QSqlDatabase db)
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
    ctx.ForEachInSelect([&](QSqlQuery& q){
        tmp.originalGenreString = q.value(QStringLiteral("genres")).toString();
        tmp.originalGenres = genreConverter.GetFFNGenreList(q.value(QStringLiteral("genres")).toString());

        tmp.ficId = q.value(QStringLiteral("fic_id")).toInt();
        tmp.ffnId = q.value(QStringLiteral("ffn_id")).toInt();
        tmp.totalLists = q.value(QStringLiteral("total")).toInt();

        tmp.strengthHumor = q.value(QStringLiteral("Div_HumorComposite")).toFloat();
        tmp.strengthRomance = q.value(QStringLiteral("Div_Flirty")).toFloat();
        tmp.strengthDrama= q.value(QStringLiteral("Div_Dramatic")).toFloat();
        tmp.strengthBonds= q.value(QStringLiteral("Div_Bondy")).toFloat();
        tmp.strengthHurtComfort= q.value(QStringLiteral("Div_Hurty")).toFloat();
        tmp.strengthNeutralComposite= q.value(QStringLiteral("Div_NeutralComposite")).toFloat();
        tmp.strengthNeutralAdventure= q.value(QStringLiteral("Div_NeutralSingle")).toFloat();
        ctx.result.data.push_back(tmp);
    });
    return ctx.result;
}

DiagnosticSQLResult<bool> QueueFicsForGenreDetection(int minAuthorRecs, int minFoundLists, int minFaves, QSqlDatabase db)
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

DiagnosticSQLResult<bool> PassScoresToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget)
{
    QList<std::string> keyList = {"fic_id", "score", "updated"};
    QList<std::string> keyListCopy = {"fic_id", "score", "updated"};
    std::string insertQS = "insert into FicScores(fic_id, score , updated) values(:fic_id, :score , :updated)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from FicScores", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassSnoozesToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget)
{
    QList<std::string> keyList = {"fic_id", "snooze_added", "snoozed_at_chapter",
                           "snoozed_till_chapter", "snoozed_until_finished", "expired"};
    QList<std::string> keyListCopy = {"fic_id", "snooze_added", "snoozed_at_chapter",
                           "snoozed_till_chapter", "snoozed_until_finished", "expired"};
    std::string insertQS = "insert into FicSnoozes(fic_id, snooze_added, snoozed_at_chapter, snoozed_till_chapter,snoozed_until_finished,expired) "
                               " values(:fic_id, :snooze_added, :snoozed_at_chapter, :snoozed_till_chapter,:snoozed_until_finished,:expired)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from FicSnoozes", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassFicTagsToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget)
{
    QList<std::string> keyList = {"fic_id", "ffn_id", "ao3_id", "sb_id", "sv_id", "tag", "added"};
    QList<std::string> keyListCopy = {"fic_id", "ffn_id", "ao3_id", "sb_id", "sv_id", "tag", "added"};
    std::string insertQS = "insert into FicTags(fic_id, ffn_id, ao3_id, sb_id,sv_id,tag, added) "
                               " values(:fic_id, :ffn_id, :ao3_id, :sb_id,:sv_id,:tag, :added)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from FicTags", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassFicNotesToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget)
{
    QList<std::string>  keyList = {"fic_id", "note_content", "updated"};
    QList<std::string>  keyListCopy  = {"fic_id", "note_content", "updated"};
    std::string insertQS = "insert into ficnotes(fic_id, note_content , updated) values(:fic_id, :note_content , :updated)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from ficnotes", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassTagSetToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget)
{
    QList<std::string> keyList = {"id", "tag"};
    QList<std::string> keyListCopy = {"id", "tag"};
    std::string insertQS = "insert into Tags(id, tag) values(:id, :tag)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from Tags", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassRecentFandomsToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget)
{
    QList<std::string> keyList = {"fandom", "seq_num"};
    QList<std::string> keyListCopy = {"fandom", "seq_num"};
    std::string insertQS = "insert into recent_fandoms(fandom, seq_num) values(:fandom, :seq_num)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from recent_fandoms", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassIgnoredFandomsToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget)
{
    QList<std::string> keyList = {"fandom_id", "including_crossovers"};
    QList<std::string> keyListCopy = {"fandom_id", "including_crossovers"};
    std::string insertQS = "insert into ignored_fandoms(fandom_id, including_crossovers) values(:fandom_id, :including_crossovers)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from ignored_fandoms", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassClientDataToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget)
{
    {
        std::string qs = "delete from user_settings";
        SqlContext<bool> (dbTarget, std::move(qs))();
    }
    QList<std::string> keyList = {"name", "value"};
    QList<std::string> keyListCopy = {"name", "value"};
    std::string insertQS = "insert into user_settings(name, value) values(:name, :value)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from user_settings", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<bool> PassReadingDataToAnotherDatabase(QSqlDatabase dbSource, QSqlDatabase dbTarget)
{
    QList<std::string> keyList = {"fic_id", "at_chapter"};
    QList<std::string> keyListCopy = {"fic_id", "at_chapter"};
    std::string insertQS = "insert into FicReadingTracker(fic_id, at_chapter) values(:name, :at_chapter)";
    ParallelSqlContext<bool> ctx (dbSource, "select * from FicReadingTracker", std::move(keyList),
                                  dbTarget, std::move(insertQS), std::move(keyListCopy));
    return ctx();
}

DiagnosticSQLResult<DBVerificationResult> VerifyDatabaseIntegrity(QSqlDatabase db)
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

}

