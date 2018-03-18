/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
#include "EGenres.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVector>
#include <QVariant>
#include <QDebug>
namespace database {
namespace puresql{

static FicIdHash GetGlobalIDHash(QSqlDatabase db, QString where)
{
    FicIdHash  result;
    QString qs = QString("select id, ffn_id, ao3_id, sb_id, sv_id from fanfics ");
    if(!where.isEmpty())
        qs+=where;
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return result;
    while(q.next())
    {
        result.ids["ffn"][q.value("ffn_id").toInt()] = q.value("id").toInt();
        result.ids["sb"][q.value("sb_id").toInt()] = q.value("id").toInt();
        result.ids["sv"][q.value("sv_id").toInt()] = q.value("id").toInt();
        result.ids["ao3"][q.value("ao3_id").toInt()] = q.value("id").toInt();
        FanficIdRecord rec;
        rec.ids["ffn"] = q.value("ffn_id").toInt();
        rec.ids["sb"] = q.value("sb_id").toInt();
        rec.ids["sv"] = q.value("sv_id").toInt();
        rec.ids["ao3"] = q.value("ao3_id").toInt();
        result.records[q.value("id").toInt()] = rec;
    }
    return  result;
}
static const FanficIdRecord& GrabFicIDFromQuery(QSqlQuery& q, QSqlDatabase db)
{
    static FicIdHash webIdToGlobal = GetGlobalIDHash(db, "");
    static FanficIdRecord result;
    auto ffn = q.value("ffn_id").toInt();
    auto ao3 = q.value("ao3_id").toInt();
    auto sb = q.value("sb_id").toInt();
    auto sv = q.value("sv_id").toInt();
    result.ids["ffn"] = ffn;
    result.ids["ao3"] = ao3;
    result.ids["sb"] = sb;
    result.ids["sv"] = sv;
    if(webIdToGlobal.ids.size() == 0)
        return result;

    if(webIdToGlobal.ids["ffn"].contains(ffn))
        result.ids["db"] = webIdToGlobal.ids["ffn"][ffn];
    else if(webIdToGlobal.ids["sb"].contains(sb))
        result.ids["db"] = webIdToGlobal.ids["sb"][sb];
    else if(webIdToGlobal.ids["sv"].contains(sv))
        result.ids["db"] = webIdToGlobal.ids["sv"][sv];
    else if(webIdToGlobal.ids["ao3"].contains(ao3))
        result.ids["db"] = webIdToGlobal.ids["ao3"][ao3];

    return result;
}

bool ExecAndCheck(QSqlQuery& q)
{
    q.exec();
    if(q.lastError().isValid())
    {
        if(q.lastError().text().contains("record"))
            qDebug() << "Error while performing a query: ";
        qDebug() << "Error while performing a query: ";
        qDebug().noquote() << q.lastQuery();
        qDebug() << "Error was: " <<  q.lastError();
        qDebug() << q.lastError().nativeErrorCode();
        if(q.lastError().text().contains("Parameter"))
            return false;
        return false;
    }
    return true;
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
bool ExecuteQuery(QSqlQuery& q, QString query)
{
    QString qs = query;
    q.prepare(qs);
    q.exec();

    if(q.lastError().isValid())
    {
        qDebug() << "SQLERROR: "<< q.lastError();
        qDebug() << q.lastQuery();
        return false;
    }
    return true;
}

bool ExecuteQueryChain(QSqlQuery& q, QStringList queries)
{
    for(auto query: queries)
    {
        if(!ExecuteQuery(q, query))
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
                             SQLPARAMS{{":tracked",QString(tracked ? "1" : "0")},
                                       {":id", id}})();
}

void CalculateFandomsFicCounts(QSqlDatabase db)
{
    QString qs = QString("update fandomindex set fic_count = (select count(fic_id) from ficfandoms where fandom_id = fandoms.id)");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return;
    return;
}

bool UpdateFandomStats(int fandomId, QSqlDatabase db)
{
    if(fandomId == -1)
        return false;
    QString qs = QString("update fandomsources set fic_count = "
                         " (select count(fic_id) from ficfandoms where fandom_id = :fandom_id)"
                         //! todo
                         //" average_faves_top_3 = (select sum(favourites)/3 from fanfics f where f.fandom = fandoms.fandom and f.id "
                         //" in (select id from fanfics where fanfics.fandom = fandoms.fandom order by favourites desc limit 3))"
                         " where fandoms.id = :fandom_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fandom_id",fandomId);
    if(!ExecAndCheck(q))
        return false;
    return true;
}

bool WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom, QSqlDatabase db)
{
    bool result = Internal::WriteMaxUpdateDateForFandom(fandom, "having count(*) = 1", db,
                                                        [](QSharedPointer<core::Fandom> f, QDateTime dt){
            f->lastUpdateDate = dt.date();
});
    return result;
}



bool Internal::WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom,
                                           QString condition,
                                           QSqlDatabase db,
                                           std::function<void(QSharedPointer<core::Fandom>,QDateTime)> writer                                           )
{
    QString qs = QString("select max(updated) as updated from fanfics where id in (select distinct fic_id from FicFandoms where fandom_id = :fandom_id %1");
    qs=qs.arg(condition);

    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return false;

    q.next();
    QString resultStr = q.value("updated").toString();
    QDateTime result;
    result = QDateTime::fromString(resultStr, "yyyy-MM-ddThh:mm:ss.000");
    writer(fandom, result);
    return true;
}
//! todo  requires refactor. fandoms need to have a tag table attached to them instead of forced sections
QStringList GetFandomListFromDB(QSqlDatabase db)
{
    QString qs = QString("select name from fandomindex");
    QSqlQuery q(qs, db);
    QStringList result;
    result.append("");
    while(q.next())
        result.append(q.value(0).toString());
    CheckExecution(q);

    return result;
}

void AssignTagToFandom(QString tag, int fandom_id, QSqlDatabase db, bool includeCrossovers)
{
    QString qs = "INSERT INTO FicTags(fic_id, tag) SELECT fic_id, '%1' as tag from FicFandoms f WHERE fandom_id = :fandom_id "
                 " and NOT EXISTS(SELECT 1 FROM FicTags WHERE fic_id = f.fic_id and tag = '%1')";
    if(!includeCrossovers)
        qs+=" and (select count(distinct fandom_id) from ficfandoms where fic_id = f.fic_id) = 1";
    qs=qs.arg(tag);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fandom_id", fandom_id);
    q.exec();
    if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
    }

}



void AssignTagToFanfic(QString tag, int fic_id, QSqlDatabase db)
{
    QString qs = "INSERT INTO FicTags(fic_id, tag) values(:fic_id, :tag)";
    //qs=qs.arg(tag);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fic_id", fic_id);
    q.bindValue(":tag", tag);
    q.exec();
    if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
    }
}


bool RemoveTagFromFanfic(QString tag, int fic_id, QSqlDatabase db)
{
    QString qs = "delete from FicTags where fic_id = :fic_id and tag = :tag";
    //qs=qs.arg(tag);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fic_id", fic_id);
    q.bindValue(":tag", tag);
    if(!ExecAndCheck(q))
        return false;
    return true;
}

bool AssignSlashToFanfic(int fic_id, int source, QSqlDatabase db)
{
    QString qs = QString("update fanfics set slash_probability = 1, slash_source = :source where id = :fic_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fic_id", fic_id);
    q.bindValue(":source", source);

    if(!ExecAndCheck(q))
        return false;
    return true;

}

bool AssignChapterToFanfic(int chapter, int fic_id, QSqlDatabase db)
{
    QString qs = QString("update fanfics set at_chapter = :chapter where id = :fic_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":chapter", chapter);
    q.bindValue(":fic_id", fic_id);

    if(!ExecAndCheck(q))
        return false;
    return true;

}

int GetLastFandomID(QSqlDatabase db){
    QString qs = QString("Select max(id) from fandomindex");
    QSqlQuery q( db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return -1;
    bool gotId = q.next();
    int result = -1;
    if(!gotId)
        result = 0;
    else
        result = q.value(0).toInt();

    return result;
}

bool WriteFandomUrls(core::FandomPtr fandom, QSqlDatabase db)
{
    QString qs = QString("insert into fandomurls(global_id, url, website, custom) values(:id, :url, :website, :custom)");
    QSqlQuery q(db);
    q.prepare(qs);
    for(auto url : fandom->urls)
    {
        q.bindValue(":id", fandom->id);
        q.bindValue(":url", url.GetUrl());
        q.bindValue(":website", fandom->source);
        q.bindValue(":custom", fandom->section);
        q.exec();
        if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
        {
            qDebug() << q.lastError();
            qDebug() << q.lastQuery();
            return false;
        }
    }
    return true;
}

bool CreateFandomInDatabase(core::FandomPtr fandom, QSqlDatabase db)
{
    Transaction transaction(db);
    auto newFandomId = GetLastFandomID(db) + 1;
    if(newFandomId == -1)
        return false;

    QString qs = QString("insert into fandomindex(id, name) "
                         " values(:id, :name)");
    QSqlQuery q(db);
    q.prepare(qs);

    q.bindValue(":name", fandom->GetName());
    q.bindValue(":id", newFandomId);

    if(!ExecAndCheck(q))
        return false;

    qDebug() << "new fandom: " << fandom->GetName();

    fandom->id = newFandomId;
    WriteFandomUrls(fandom, db);
    transaction.finalize();
    return true;
}
int GetFicIdByAuthorAndName(QString author, QString title, QSqlDatabase db)
{
    QSqlQuery q1(db);
    QString qsl = " select id from fanfics where author = :author and title = :title";
    q1.prepare(qsl);
    q1.bindValue(":author", author);
    q1.bindValue(":title", title);
    q1.exec();
    int result = -1;
    while(q1.next())
        result = q1.value(0).toInt();
    CheckExecution(q1);
    return result;
}

int GetFicIdByWebId(QString website, int webId, QSqlDatabase db)
{
    QSqlQuery q1(db);
    QString qsl = " select id from fanfics where %1_id = :site_id";
    qsl = qsl.arg(website);
    q1.prepare(qsl);
    q1.bindValue(":site_id",webId);
    q1.exec();
    int result = -1;
    while(q1.next())
        result = q1.value(0).toInt();
    CheckExecution(q1);

    return result;
}

core::FicPtr LoadFicFromQuery(QSqlQuery& q1, QString website = "ffn")
{
    auto fic = core::Fic::NewFanfic();
    fic->atChapter = q1.value("AT_CHAPTER").toInt();
    fic->complete  = q1.value("COMPLETE").toInt();
    fic->webId     = q1.value(website + "_ID").toInt();
    fic->id        = q1.value("ID").toInt();
    fic->wordCount = q1.value("WORDCOUNT").toString();
    fic->chapters = q1.value("CHAPTERS").toString();
    fic->reviews = q1.value("REVIEWS").toString();
    fic->favourites = q1.value("FAVOURITES").toString();
    fic->follows = q1.value("FOLLOWS").toString();
    fic->rated = q1.value("RATED").toString();
    fic->fandom = q1.value("FANDOM").toString();
    fic->title = q1.value("TITLE").toString();
    fic->genres = q1.value("GENRES").toString().split("##");
    fic->summary = q1.value("SUMMARY").toString();
    fic->published = q1.value("PUBLISHED").toDateTime();
    fic->updated = q1.value("UPDATED").toDateTime();
    fic->characters = q1.value("CHARACTERS").toString().split(",");
    fic->authorId = q1.value("AUTHOR_ID").toInt();
    fic->author->name = q1.value("AUTHOR").toString();
    fic->ffn_id = q1.value("FFN_ID").toInt();
    fic->ao3_id = q1.value("AO3_ID").toInt();
    fic->sb_id = q1.value("SB_ID").toInt();
    fic->sv_id = q1.value("SV_ID").toInt();
    fic->isSlash = q1.value("slash_probability").toDouble() > 0.9;
    return fic;
}

core::FicPtr GetFicByWebId(QString website, int webId, QSqlDatabase db)
{
    core::FicPtr fic;
    QSqlQuery q1(db);
    QString qsl = " select * from fanfics where %1_id = :site_id";
    qsl = qsl.arg(website);
    q1.prepare(qsl);
    q1.bindValue(":site_id",webId);

    if(!ExecAndCheck(q1))
        return fic;

    if(!q1.next())
        return fic;

    fic = LoadFicFromQuery(q1, website);
    fic->webSite = website;
    return fic;
}

core::FicPtr GetFicById( int ficId, QSqlDatabase db)
{
    core::FicPtr fic;
    QSqlQuery q1(db);
    QString qsl = " select * from fanfics where id = :fic_id";
    q1.prepare(qsl);
    q1.bindValue(":fic_id",ficId);

    if(!ExecAndCheck(q1))
        return fic;

    if(!q1.next())
        return fic;

    fic = LoadFicFromQuery(q1);



    return fic;
}


bool SetUpdateOrInsert(QSharedPointer<core::Fic> fic, QSqlDatabase db, bool alwaysUpdateIfNotInsert)
{
    if(!fic)
        return false;

    QString getKeyQuery = "Select ( select count(id) from FANFICS where  %1_id = :site_id) as COUNT_NAMED,"
                          " ( select count(id) from FANFICS where  %1_id = :site_id and (updated <> :updated or updated is null)) as count_updated"
            ;

    QString filledQuery = getKeyQuery.arg(fic->webSite);
    //    if(fic->webId == 12741163 ||
    //            fic->webId == 12322164 ||
    //            fic->webId == 10839545)
    //        qDebug() << filledQuery;
    QSqlQuery q(db);
    q.prepare(filledQuery);
    q.bindValue(":updated", fic->updated);
    q.bindValue(":site_id", fic->webId);
    //qDebug() << getKeyQuery;
    if(!ExecAndCheck(q))
        return false;

    if(!q.next())
        return false;

    int countNamed = q.value("COUNT_NAMED").toInt();
    int countUpdated = q.value("count_updated").toInt();
    bool requiresInsert = countNamed == 0;
    bool requiresUpdate = countUpdated > 0;

    if(alwaysUpdateIfNotInsert || (!requiresInsert && requiresUpdate))
        fic->updateMode = core::UpdateMode::update;
    if(requiresInsert)
        fic->updateMode = core::UpdateMode::insert;
    return true;
}

bool InsertIntoDB(QSharedPointer<core::Fic> section, QSqlDatabase db)
{
    QString query = "INSERT INTO FANFICS (%1_id, FANDOM, AUTHOR, TITLE,WORDCOUNT, CHAPTERS, FAVOURITES, REVIEWS, "
                    " CHARACTERS, COMPLETE, RATED, SUMMARY, GENRES, PUBLISHED, UPDATED, AUTHOR_ID,"
                    " wcr, reviewstofavourites, age, daysrunning, at_chapter ) "
                    "VALUES ( :site_id,  :fandom, :author, :title, :wordcount, :CHAPTERS, :FAVOURITES, :REVIEWS, "
                    " :CHARACTERS, :COMPLETE, :RATED, :summary, :genres, :published, :updated, :author_id,"
                    " :wcr, :reviewstofavourites, :age, :daysrunning, 0 )";
    query=query.arg(section->webSite);
    QSqlQuery q(db);
    q.prepare(query);
    q.bindValue(":site_id",section->webId); //?
    q.bindValue(":fandom",section->fandom);
    q.bindValue(":author",section->author->name); //?
    q.bindValue(":author_id",section->author->id);
    //q.bindValue(":author_web_id",section->author->webId);
    q.bindValue(":title",section->title);
    q.bindValue(":wordcount",section->wordCount.toInt());
    q.bindValue(":CHAPTERS",section->chapters.trimmed().toInt());
    q.bindValue(":FAVOURITES",section->favourites.toInt());
    q.bindValue(":REVIEWS",section->reviews.toInt());
    q.bindValue(":CHARACTERS",section->charactersFull);
    q.bindValue(":RATED",section->rated);
    q.bindValue(":summary",section->summary);
    q.bindValue(":COMPLETE",section->complete);
    q.bindValue(":genres",section->genreString);
    q.bindValue(":published",section->published);
    q.bindValue(":updated",section->updated);
    q.bindValue(":wcr",section->calcStats.wcr);
    q.bindValue(":reviewstofavourites",section->calcStats.reviewsTofavourites);
    q.bindValue(":age",section->calcStats.age);
    q.bindValue(":daysrunning",section->calcStats.daysRunning);
    q.exec();
    //qDebug() << "Inserting:" << section->title;
    if(q.lastError().isValid())
    {
        qDebug() << "failed to insert: " << section->author->name << " " << section->title;
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return false;
    }
    return true;

}
bool UpdateInDB(QSharedPointer<core::Fic> section, QSqlDatabase db)
{
    QString query = "UPDATE FANFICS set fandom = :fandom, wordcount= :wordcount, CHAPTERS = :CHAPTERS,  "
                    "COMPLETE = :COMPLETE, FAVOURITES = :FAVOURITES, REVIEWS= :REVIEWS, CHARACTERS = :CHARACTERS, RATED = :RATED, "
                    "summary = :summary, genres= :genres, published = :published, updated = :updated, author_id = :author_id,"
                    "wcr= :wcr,  author= :author, title= :title, reviewstofavourites = :reviewstofavourites, age = :age, daysrunning = :daysrunning "
                    " where %1_id = :site_id";
    query=query.arg(section->webSite);
    QSqlQuery q(db);
    q.prepare(query);
    q.bindValue(":fandom",section->fandom);
    q.bindValue(":author",section->author->name);
    q.bindValue(":author_id",section->author->id);
    //q.bindValue(":author_web_id",section->author->webId);
    q.bindValue(":title",section->title);

    q.bindValue(":wordcount",section->wordCount.toInt());
    q.bindValue(":CHAPTERS",section->chapters.trimmed().toInt());
    q.bindValue(":FAVOURITES",section->favourites.toInt());
    q.bindValue(":REVIEWS",section->reviews.toInt());
    q.bindValue(":CHARACTERS",section->charactersFull);
    q.bindValue(":RATED",section->rated);

    q.bindValue(":summary",section->summary);
    q.bindValue(":COMPLETE",section->complete);
    q.bindValue(":genres",section->genreString);
    q.bindValue(":published",section->published);
    q.bindValue(":updated",section->updated);
    q.bindValue(":site_id",section->webId);
    q.bindValue(":wcr",section->calcStats.wcr);
    q.bindValue(":reviewstofavourites",section->calcStats.reviewsTofavourites);
    q.bindValue(":age",section->calcStats.age);
    q.bindValue(":daysrunning",section->calcStats.daysRunning);
    q.exec();
    //qDebug() << "Updating:" << section->title;
    if(q.lastError().isValid())
    {
        qDebug() << "failed to update: " << section->author->name << " " << section->title;
        qDebug() << q.lastError();
        return false;
    }
    return true;
}

bool WriteRecommendation(core::AuthorPtr author, int fic_id, QSqlDatabase db)
{
    if(!author || author->id < 0)
        return false;

    QSqlQuery q1(db);
    // atm this pairs favourite story with an author
    QString qsl = " insert into recommendations (recommender_id, fic_id) values(:recommender_id,:fic_id); ";
    q1.prepare(qsl);
    q1.bindValue(":recommender_id", author->id);
    q1.bindValue(":fic_id", fic_id);
    q1.exec();

    if(q1.lastError().isValid() && !q1.lastError().text().contains("UNIQUE constraint failed"))
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    return true;
}
int GetAuthorIdFromUrl(QString url, QSqlDatabase db)
{
    QSqlQuery q1(db);
    QString qsl = " select id from recommenders where url like '%%1%' ";
    qsl = qsl.arg(url);
    q1.prepare(qsl);
    //q1.bindValue(":url", url);
    q1.exec();
    int result = -1;
    while(q1.next())
        result = q1.value(0).toInt();

    if(!CheckExecution(q1))
        return -2;

    return result;
}
int GetAuthorIdFromWebID(int id, QString website, QSqlDatabase db)
{
    QSqlQuery q1(db);
    QString qsl = "select id from recommenders where %1_id = :id";
    qsl = qsl.arg(website);
    q1.prepare(qsl);
    q1.bindValue(":id", id);
    q1.exec();
    int result = -1;
    while(q1.next())
        result = q1.value(0).toInt();

    if(!CheckExecution(q1))
        return -2;

    return result;
}
bool AssignNewNameToAuthorWithId(core::AuthorPtr author, QSqlDatabase db)
{
    if(author->GetIdStatus() != core::AuthorIdStatus::valid)
        return true;
    QSqlQuery q(db);
    QString qsl = " UPDATE recommenders SET name = :name where id = :id";
    q.prepare(qsl);
    q.bindValue(":name",author->name);
    q.bindValue(":id",author->id);
    if(!ExecAndCheck(q))
        return false;
    return true;
}

void ProcessIdsFromQuery(core::AuthorPtr author, QSqlQuery& q)
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
    ProcessIdsFromQuery(result, q);
    return result;
}


QList<core::AuthorPtr> GetAllAuthors(QString website,  QSqlDatabase db)
{
    QList<core::AuthorPtr> result;
    QString qs = QString("select count(id) from recommenders where website_type = :site");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":site",website);
    if(!ExecAndCheck(q))
        return result;

    q.next();
    int size = q.value(0).toInt();

    qs = QString("select distinct id,name, url, ffn_id, ao3_id,sb_id, sv_id,  "
                 "(select count(fic_id) from recommendations where recommender_id = recommenders.id) as rec_count "
                 " from recommenders where website_type = :site order by id");
    q.prepare(qs);
    q.bindValue(":site",website);
    if(!ExecAndCheck(q))
        return result;

    result.reserve(size);
    int counter = 0;
    while(q.next())
    {
        counter++;
        //        if(counter > 100)
        //            break;
        auto author = AuthorFromQuery(q);
        result.push_back(author);
    }
    return result;
}



QList<core::AuthorPtr> GetAuthorsForRecommendationList(int listId,  QSqlDatabase db)
{
    QList<core::AuthorPtr> result;

    QSqlQuery q(db);
    QString qs = QString("select id,name, url, ffn_id, ao3_id,sb_id, sv_id, "
                         "(select count(fic_id) from recommendations where recommender_id = recommenders.id) as rec_count "
                         " from recommenders "
                         "where id in ( select author_id from RecommendationListAuthorStats where list_id = :list_id )");
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return result;

    while(q.next())
    {
        auto author = AuthorFromQuery(q);
        result.push_back(author);
    }
    return result;
}


core::AuthorPtr GetAuthorByNameAndWebsite(QString name, QString website, QSqlDatabase db)
{
    core::AuthorPtr result;
    QString qs = QString("select id,name, url, website_type, ffn_id, ao3_id,sb_id, sv_id from recommenders where %1_id is not null and name = :name");
    qs=qs.arg(website);

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":site",website);
    q.bindValue(":name",name);
    if(!ExecAndCheck(q))
        return result;

    if(!q.next())
        return result;

    result = AuthorFromQuery(q);
    return result;
}
core::AuthorPtr GetAuthorByIDAndWebsite(int id, QString website, QSqlDatabase db)
{
    core::AuthorPtr result;
    QString qs = QString("select r.id,r.name, r.url, r.website_type, r.ffn_id, r.ao3_id,r.sb_id, r.sv_id, "
                         " (select count(fic_id) from recommendations where recommender_id = r.id) as rec_count "
                         "from recommenders r where %1_id is not null and %1_id = :id");
    qs=qs.arg(website);

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":site",website);
    q.bindValue(":id",id);
    if(!ExecAndCheck(q))
        return result;

    if(!q.next())
        return result;

    result = AuthorFromQuery(q);
    return result;
}

void AuthorStatisticsFromQuery(QSqlQuery& q,  core::AuthorPtr author)
{
    core::FicSectionStats& stats = author->stats.favouriteStats;
    stats.favourites = q.value("favourites").toInt();
    stats.ficWordCount = q.value("favourites_wordcount").toInt();
    stats.averageLength = q.value("average_words_per_chapter").toInt();
    stats.esrbType = static_cast<core::FicSectionStats::ESRBType>(q.value("esrb_type").toInt());
    stats.prevalentMood = static_cast<core::FicSectionStats::MoodType>(q.value("prevalent_mood").toInt());
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
    DiagnosticSQLResult<bool> result;
    result.success = false;

    QString qs = QString("select * from AuthorFavouritesStatistics  where author_id = :id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id",author->id);
    if(!result.ExecAndCheck(q))
        return result;

    if(!result.CheckDataAvailability(q))
        return result;

    AuthorStatisticsFromQuery(q, author);
    return result;
}

core::AuthorPtr GetAuthorByUrl(QString url, QSqlDatabase db)
{
    core::AuthorPtr result;
    QString qs = QString("select r.id,name, r.url, r.ffn_id, r.ao3_id, r.sb_id, r.sv_id, "
                         " (select count(fic_id) from recommendations where recommender_id = r.id) as rec_count "
                         " from recommenders r where url = :url");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":url",url);

    if(!ExecAndCheck(q))
        return result;

    if(!q.next())
        return result;

    result = AuthorFromQuery(q);
    return result;
}

core::AuthorPtr GetAuthorById(int id, QSqlDatabase db)
{
    core::AuthorPtr result;
    QString qs = QString("select id,name, url, ffn_id, ao3_id,sb_id, sv_id, "
                         "(select count(fic_id) from recommendations where recommender_id = :id) as rec_count "
                         "from recommenders where id = :id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id",id);

    if(!ExecAndCheck(q))
        return result;

    if(!q.next())
        return result;

    result = AuthorFromQuery(q);
    return result;
}

QList<QSharedPointer<core::RecommendationList> > GetAvailableRecommendationLists(QSqlDatabase db)
{
    QString qs = QString("select * from RecommendationLists order by name");
    QSqlQuery q(db);
    q.prepare(qs);
    QList<QSharedPointer<core::RecommendationList> > result;
    if(!ExecAndCheck(q))
        return result;

    while(q.next())
    {
        QSharedPointer<core::RecommendationList>  list(new core::RecommendationList);
        list->alwaysPickAt = q.value("always_pick_at").toInt();
        list->minimumMatch = q.value("minimum").toInt();
        list->pickRatio= q.value("pick_ratio").toDouble();
        list->id= q.value("id").toInt();
        list->name= q.value("name").toString();
        list->ficCount= q.value("fic_count").toInt();
        result.push_back(list);
    }
    return result;

}
QSharedPointer<core::RecommendationList> GetRecommendationList(int listId, QSqlDatabase db)
{
    QString qs = QString("select * from RecommendationLists where id = :list_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id", listId);
    QSharedPointer<core::RecommendationList>  result;
    if(!ExecAndCheck(q))
        return result;

    if(q.next())
    {
        QSharedPointer<core::RecommendationList>  list(new core::RecommendationList);
        result = list;
        list->alwaysPickAt = q.value("always_pick_at").toInt();
        list->minimumMatch = q.value("minimum").toInt();
        list->pickRatio= q.value("pick_ratio").toDouble();
        list->id= q.value("id").toInt();
        list->name= q.value("name").toString();
        list->ficCount= q.value("fic_count").toInt();
    }
    return result;
}
QSharedPointer<core::RecommendationList> GetRecommendationList(QString name, QSqlDatabase db)
{
    QString qs = QString("select * from RecommendationLists where name = :list_name");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_name", name);
    QSharedPointer<core::RecommendationList>  result;
    if(!ExecAndCheck(q))
        return result;

    if(q.next())
    {
        QSharedPointer<core::RecommendationList>  list(new core::RecommendationList);
        result = list;
        list->alwaysPickAt = q.value("always_pick_at").toInt();
        list->minimumMatch = q.value("minimum").toInt();
        list->pickRatio= q.value("pick_ratio").toDouble();
        list->id= q.value("id").toInt();
        list->name= q.value("name").toString();
        list->ficCount= q.value("fic_count").toInt();
    }
    return result;
}
QList<core::AuhtorStatsPtr> GetRecommenderStatsForList(int listId, QString sortOn, QString order, QSqlDatabase db)
{
    QList<core::AuhtorStatsPtr> result;
    //auto listId= GetRecommendationListIdForName(listName);
    QString qs = QString("select rts.match_count as match_count,"
                         "rts.match_ratio as match_ratio,"
                         "rts.author_id as author_id,"
                         "rts.fic_count as fic_count,"
                         "r.name as name"
                         "  from RecommendationListAuthorStats rts, recommenders r where rts.author_id = r.id and list_id = :list_id order by %1 %2");
    qs=qs.arg(sortOn).arg(order);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return result;

    while(q.next())
    {
        core::AuhtorStatsPtr stats(new core::AuthorRecommendationStats);
        stats->isValid = true;
        stats->matchesWithReference = q.value("match_count").toInt();
        stats->matchRatio= q.value("match_ratio").toDouble();
        stats->authorId= q.value("author_id").toInt();
        stats->listId= listId;
        stats->totalRecommendations= q.value("fic_count").toInt();
        stats->authorName = q.value("name").toString();
        result.push_back(stats);
    }
    return result;
}

int GetMatchCountForRecommenderOnList(int authorId, int list, QSqlDatabase db)
{
    QSqlQuery q1(db);
    QString qsl = "select fic_count from RecommendationListAuthorStats where list_id = :list_id and author_id = :author_id";
    q1.prepare(qsl);
    q1.bindValue(":list_id", list);
    q1.bindValue(":author_id", authorId);
    q1.exec();
    if(ExecAndCheck(q1))
        return -1;
    q1.next();
    int matches  = q1.value(0).toInt();
    qDebug() << "Using query: " << q1.lastQuery();
    qDebug() << "Matches found: " << matches;
    return matches;
}

QVector<int> GetAllFicIDsFromRecommendationList(int listId, QSqlDatabase db)
{
    QVector<int> result;
    QString qs = QString("select count(fic_id) from RecommendationListData where list_id = :list_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return result;

    q.next();
    int size = q.value(0).toInt();

    qs = QString("select fic_id from RecommendationListData where list_id = :list_id");
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return result;

    result.reserve(size);
    while(q.next())
        result.push_back(q.value("fic_id").toInt());

    return result;
}

QStringList GetAllAuthorNamesForRecommendationList(int listId, QSqlDatabase db)
{
    QStringList result;

    QString qs = QString("select name from recommenders where id in (select author_id from RecommendationListAuthorStats where list_id = :list_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return result;

    while(q.next())
        result.push_back(q.value("name").toString());

    return result;
}

int GetCountOfTagInAuthorRecommendations(int authorId, QString tag, QSqlDatabase db)
{
    int result = 0;
    QString qs = QString("select count(distinct fic_id) from FicTags ft where ft.tag = :tag and exists"
                         " (select 1 from Recommendations where ft.fic_id = fic_id and recommender_id = :recommender_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":tag",tag);
    q.bindValue(":recommender_id",authorId);
    if(!ExecAndCheck(q))
        return result;
    auto hasData = q.next();
    result = q.value(0).toInt();
    return result;
}

//!todo needs check if the query actually works
int GetMatchesWithListIdInAuthorRecommendations(int authorId, int listId, QSqlDatabase db)
{
    int result = 0;
    QString qs = QString("select count(fic_id) from Recommendations r where recommender_id = :author_id and exists "
                         " (select 1 from RecommendationListData rld where rld.list_id = :list_id and fic_id = rld.fic_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":author_id",authorId);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return result;
    result = q.value(0).toInt();
    return result;
}

bool DeleteRecommendationList(int listId, QSqlDatabase db )
{
    if(listId == 0)
        return false;
    QString qs = QString("delete from RecommendationLists where id = :list_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return false;

    qs = QString("delete from RecommendationListAuthorStats where list_id = :list_id");
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return false;

    qs = QString("delete from RecommendationListData where list_id = :list_id");
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return false;

    return true;
}

bool DeleteRecommendationListData(int listId, QSqlDatabase db)
{
    if(listId == 0)
        return false;
    QString qs;
    QSqlQuery q(db);
    qs = QString("delete from RecommendationListAuthorStats where list_id = :list_id");
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return false;

    qs = QString("delete from RecommendationListData where list_id = :list_id");
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return false;

    return true;
}

bool CopyAllAuthorRecommendationsToList(int authorId, int listId, QSqlDatabase db )
{
    QString qs = QString("insert into RecommendationListData (fic_id, list_id)"
                         " select fic_id, %1 as list_id from recommendations r where r.recommender_id = :author_id and "
                         " not exists( select 1 from RecommendationListData where list_id=:list_id and fic_id = r.fic_id) ");
    qs=qs.arg(listId);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":author_id",authorId);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return false;
    return true;
}
bool WriteAuthorRecommendationStatsForList(int listId, core::AuhtorStatsPtr stats, QSqlDatabase db)
{
    if(!stats)
        return false;
    DiagnosticSQLResult<bool> result;

    QString qs = QString("insert into RecommendationListAuthorStats (author_id, fic_count, match_count, match_ratio, list_id) "
                         "values(:author_id, :fic_count, :match_count, :match_ratio, :list_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":author_id",stats->authorId);
    q.bindValue(":fic_count",stats->totalRecommendations);
    q.bindValue(":match_count",stats->matchesWithReference);
    q.bindValue(":match_ratio",stats->matchRatio);
    //auto listId= GetRecommendationListIdForName(stats->listName);
    q.bindValue(":list_id",listId);
    if(!result.ExecAndCheck(q, true))
        return false;
    return true;
}
bool CreateOrUpdateRecommendationList(QSharedPointer<core::RecommendationList> list, QDateTime creationTimestamp, QSqlDatabase db)
{
    QString qs = QString("insert into RecommendationLists(name) select '%1' "
                         " where not exists(select 1 from RecommendationLists where name = '%1')");
    qs= qs.arg(list->name);
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return false;
    //CURRENT_TIMESTAMP not portable
    qs = QString("update RecommendationLists set minimum = :minimum, pick_ratio = :pick_ratio, "
                 " always_pick_at = :always_pick_at,  created = :created where name = :name");
    q.prepare(qs);
    q.bindValue(":minimum",list->minimumMatch);
    q.bindValue(":pick_ratio",list->pickRatio);
    q.bindValue(":created",creationTimestamp);
    q.bindValue(":always_pick_at",list->alwaysPickAt);
    q.bindValue(":name",list->name);
    if(!ExecAndCheck(q))
        return false;


    qs = QString("select id from RecommendationLists where name = :name");
    q.prepare(qs);
    q.bindValue(":name",list->name);
    if(!ExecAndCheck(q))
        return false;

    q.next();
    list->id = q.value(0).toInt();
    if(list->id > 0)
        return true;
    return false;
}
bool UpdateFicCountForRecommendationList(int listId, QSqlDatabase db)
{
    if(listId == -1)
        return false;

    QString qs = QString("update RecommendationLists set fic_count=(select count(fic_id) "
                         " from RecommendationListData where list_id = :list_id) where id = :list_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return false;
    return true;
}
bool DeleteTagFromDatabase(QString tag, QSqlDatabase db)
{
    QString qs = QString("delete from FicTags where tag = :tag");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":tag",tag);
    if(!ExecAndCheck(q))
        return false;

    qs = QString("delete from tags where tag = :tag");
    q.prepare(qs);
    q.bindValue(":tag",tag);
    if(!ExecAndCheck(q))
        return false;
    return true;
}

bool CreateTagInDatabase(QString tag, QSqlDatabase db)
{
    QString qs = QString("INSERT INTO TAGS(TAG) VALUES(:tag)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":tag",tag);
    if(!ExecAndCheck(q))
        return false;
    return true;
}

int GetRecommendationListIdForName(QString name, QSqlDatabase db)
{
    int result = 0;
    QString qs = QString("select id from RecommendationLists where name = :name");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":name",name);
    if(!ExecAndCheck(q))
        return result;
    q.next();
    result = q.value(0).toInt();
    return result;
}
bool AddAuthorFavouritesToList(int authorId, int listId, QSqlDatabase db)
{
    QString qs = QString(" update RecommendationListData set match_count = match_count+1 where "
                         " list_id = :list_id "
                         " and fic_id in (select fic_id from recommendations r where recommender_id = :author_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":author_id",authorId);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return false;
    return true;
}
void ShortenFFNurlsForAllFics(QSqlDatabase db)
{
    QString qs = QString("update fanfics set url = cfReturnCapture('(/s/\\d+/)', url)");
    QSqlQuery q(db);
    q.prepare(qs);
    ExecAndCheck(q);
}
bool IsGenreList(QStringList list, QString website, QSqlDatabase db)
{
    // first we need to process hurt/comfort
    QString qs = QString("select count(id) from genres where genre in(%1) and website = :website");
    qs = qs.arg("'" + list.join("','") + "'");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":website", website);
    if(!ExecAndCheck(q))
        return false;
    q.next();
    return q.value(0).toInt() > 0;

}

QVector<int> GetWebIdList(QString where, QString website, QSqlDatabase db)
{
    QVector<int> result;

    QString qs = QString("select count(id), %1_id from fanfics %2");
    qs = qs.arg(website).arg(where);
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return result;

    while(q.next())
    {
        if(result.empty())
            result.reserve(q.value(0).toInt());
        auto id = q.value(1).toInt();
        result.push_back(id);
    }
    return result;
}
DiagnosticSQLResult<QVector<int>> GetIdList(QString where, QSqlDatabase db)
{
    DiagnosticSQLResult<QVector<int>> result;

    QString qs = QString("select count(id) from fanfics");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;

    if(!result.CheckDataAvailability(q))
        return result;

    int size = q.value(0).toInt();

    result.data.reserve(size);


    qs = QString("select id from fanfics %1 order by id asc");
    qs = qs.arg(where);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return result;

    while(q.next())
    {
        auto id = q.value(0).toInt();
        result.data.push_back(id);
    }
    return result;
}

bool DeactivateStory(int id, QString website, QSqlDatabase db)
{
    QString qs = QString("update fanfics set alive = 0 where %1_id = :id");
    qs=qs.arg(website);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id", id);
    if(!ExecAndCheck(q))
        return false;
    return true;
}

bool CreateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, QSqlDatabase db)
{
    QSqlQuery q1(db);
    QString qsl = " insert into recommenders(name, url, favourites, fics, page_updated, ffn_id, ao3_id,sb_id, sv_id, "
                  "page_creation_date, info_updated) "
                  "values(:name, :url, :favourites, :fics,  :time, :ffn_id, :ao3_id,:sb_id, :sv_id, "
                  ":page_creation_date, :info_updated) ";
    q1.prepare(qsl);
    q1.bindValue(":name", author->name);
    q1.bindValue(":time", timestamp);
    q1.bindValue(":url", author->url("ffn"));
    q1.bindValue(":favourites", author->favCount);
    q1.bindValue(":fics", author->ficCount);
    q1.bindValue(":ffn_id", author->GetWebID("ffn"));
    q1.bindValue(":ao3_id", author->GetWebID("ao3"));
    q1.bindValue(":sb_id", author->GetWebID("sb"));
    q1.bindValue(":sv_id", author->GetWebID("sv"));

    q1.bindValue(":page_creation_date", author->stats.pageCreated);
    q1.bindValue(":info_updated", author->stats.bioLastUpdated);

    if(!ExecAndCheck(q1))
        return false;
    return true;
}
bool UpdateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, QSqlDatabase db)
{
    QSqlQuery q1(db);
    QString qsl = " update recommenders set name = :name, url = :url, favourites = :favourites, fics = :fics, page_updated = :time, "
                  "page_creation_date= :page_creation_date, info_updated= :info_updated, "
                  " ffn_id = :ffn_id, ao3_id = :ao3_id, sb_id  =:sb_id, sv_id = :sv_id where id = :id ";
    q1.prepare(qsl);
    q1.bindValue(":id", author->id);
    q1.bindValue(":name", author->name);
    q1.bindValue(":time", timestamp);
    q1.bindValue(":url", author->url("ffn"));
    q1.bindValue(":favourites", author->favCount);
    q1.bindValue(":fics", author->ficCount);
    q1.bindValue(":ffn_id", author->GetWebID("ffn"));
    q1.bindValue(":ao3_id", author->GetWebID("ao3"));
    q1.bindValue(":sb_id", author->GetWebID("sb"));
    q1.bindValue(":sv_id", author->GetWebID("sv"));

    q1.bindValue(":page_creation_date", author->stats.pageCreated);
    q1.bindValue(":info_updated", author->stats.bioLastUpdated);

    //not reading those yet
    //q1.bindValue(":info_wordcount", author->ficCount);
    //    q1.bindValue(":last_published_fic_date", author->ficCount);
    //    q1.bindValue(":first_published_fic_date", author->ficCount);
    //    q1.bindValue(":own_wordcount", author->ficCount);
    //    q1.bindValue(":own_favourites", author->ficCount);
    //    q1.bindValue(":own_finished_ratio", author->ficCount);
    //    q1.bindValue(":most_written_size", author->ficCount);

    if(!ExecAndCheck(q1))
        return false;
    return true;
}

QStringList ReadUserTags(QSqlDatabase db)
{
    QStringList tagList;
    QString qs = QString("Select tag from tags ");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return tagList;
    while(q.next())
    {
        tagList.append(q.value(0).toString());
    }
    return tagList;
}

bool PushTaglistIntoDatabase(QStringList tagList, QSqlDatabase db)
{
    bool success = true;
    int counter = 0;
    for(QString tag : tagList)
    {
        QString qs = QString("INSERT INTO TAGS (TAG, id) VALUES (:tag, :id)");
        QSqlQuery q(db);
        q.prepare(qs);
        q.bindValue(":tag", tag);
        q.bindValue(":id", counter);
        q.exec();
        if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
        {
            success = false;
            qDebug() << q.lastError().text();
        }
        counter++;
    }
    return success;
}

bool AssignNewNameForAuthor(core::AuthorPtr author, QString name, QSqlDatabase db)
{
    if(author->GetIdStatus() != core::AuthorIdStatus::valid)
        return true;
    QSqlQuery q(db);
    QString qsl = " UPDATE recommenders SET name = :name where id = :id";
    q.prepare(qsl);
    q.bindValue(":name",name);
    q.bindValue(":id",author->id);
    if(!ExecAndCheck(q))
        return false;
    return true;
}

QList<int> GetAllAuthorIds(QSqlDatabase db)
{
    QList<int> result;

    QString qs = QString("select distinct id from recommenders");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return result;

    while(q.next())
        result.push_back(q.value(0).toInt());

    return result;
}
DiagnosticSQLResult<QStringList> GetAllAuthorFavourites(int id, QSqlDatabase db)
{
    DiagnosticSQLResult<QStringList> result;
    result.success = false;

    QString qs = QString("select id, ffn_id, ao3_id, sb_id, sv_id from fanfics where id in (select fic_id from recommendations where recommender_id = :author_id )");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":author_id",id);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;

    do
    {
        auto ffn_id = q.value("ffn_id").toInt();
        auto ao3_id = q.value("ao3_id").toInt();
        auto sb_id = q.value("sb_id").toInt();
        auto sv_id = q.value("sv_id").toInt();

        if(ffn_id != -1)
            result.data.push_back(url_utils::GetStoryUrlFromWebId(ffn_id, "ffn"));
        if(ao3_id != -1)
            result.data.push_back(url_utils::GetStoryUrlFromWebId(ao3_id, "ao3"));
        if(sb_id != -1)
            result.data.push_back(url_utils::GetStoryUrlFromWebId(sb_id, "sb"));
        if(sv_id != -1)
            result.data.push_back(url_utils::GetStoryUrlFromWebId(sv_id, "sv"));
    }while(q.next());
    result.success = true;

    return result;
}

bool IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, QSqlDatabase db)
{
    QString qs = QString(" update RecommendationListData set match_count = match_count+1 where "
                         " list_id = :list_id "
                         " and fic_id in (select fic_id from recommendations r where recommender_id = :author_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":author_id",authorId);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return false;
    return true;
}


bool DecrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, QSqlDatabase db)
{
    QString qs = QString(" update RecommendationListData set match_count = match_count-1 where "
                         " list_id = :list_id "
                         " and fic_id in (select fic_id from recommendations r where recommender_id = :author_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":author_id",authorId);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return false;


    qs = QString(" delete from RecommendationListData where match_count <= 0");
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return false;
    return true;
}

QSet<QString> GetAllGenres(QSqlDatabase db)
{
    QSet<QString> result;
    QString qs = QString("select genre from genres");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return result;
    while(q.next())
        result.insert(q.value(0).toString());
    return result;
}

static core::FandomPtr FandomfromQueryNew (QSqlQuery& q, core::FandomPtr fandom = core::FandomPtr())
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
static bool GetFandomStats(core::FandomPtr fandom, QSqlDatabase db)
{
    if(!fandom)
        return false;

    QString qs = QString("select * from fandomsources where global_id = :id");
    QSqlQuery q(db);
    q.bindValue(":id",fandom->id);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return false;
    bool hasData = q.next();
    if(!hasData)
        return false;
    fandom->source = q.value("website").toString();
    fandom->ficCount = q.value("fic_count").toInt();
    fandom->averageFavesTop3 = q.value("average_faves_top_3").toDouble();
    fandom->dateOfCreation = q.value("date_of_creation").toDate();
    fandom->dateOfFirstFic = q.value("date_of_first_fic").toDate();
    fandom->dateOfLastFic = q.value("date_of_last_fic").toDate();
    //fandom->lastUpdateDate = q.value("last_update").toDate();
    return true;
}

QList<core::FandomPtr> GetAllFandoms(QSqlDatabase db)
{
    QList<core::FandomPtr> result;

    QString qs = QString(" select count(id) from fandomindex");

    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return result;
    q.next();

    int fandomsSize = q.value(0).toInt();
    result.reserve(fandomsSize);

    qs = QString(" select ind.id as id, ind.name as name, ind.tracked as tracked, urls.url as url, urls.website as website,"
                 " urls.custom as section, ind.updated as updated "
                 " from fandomindex ind left join fandomurls urls"
                 " on ind.id = urls.global_id order by id asc");
    q.prepare(qs);

    if(!ExecAndCheck(q))
        return result;
    core::FandomPtr currentFandom;
    int lastId = -1;
    while(q.next())
    {
        auto currentId= q.value("id").toInt();
        if(currentId != lastId)
        {
            currentFandom = FandomfromQueryNew(q);
            //GetFandomStats(currentFandom, db);
            result.push_back(currentFandom);
        }
        else
            currentFandom = FandomfromQueryNew(q, currentFandom);
        lastId = currentId;
    }

    return result;
}

core::FandomPtr GetFandom(QString name, QSqlDatabase db)
{
    core::FandomPtr result;

    QString qs = QString(" select ind.id as id, ind.name as name, ind.tracked as tracked, urls.url as url, urls.website as website,"
                         " urls.custom as section, ind.updated as updated from fandomindex ind left join fandomurls urls on ind.id = urls.global_id"
                         " where name = :fandom ");

    QSqlQuery q(db);
    q.prepare(qs);
    QString tempName = name;
    //q.bindValue(":fandom", tempName.replace("'", "''"));
    q.bindValue(":fandom", tempName);
    if(!ExecAndCheck(q))
        return result;

    QString lastName;
    core::FandomPtr currentFandom;
    while(q.next())
        currentFandom = FandomfromQueryNew(q, currentFandom);
    GetFandomStats(currentFandom, db);
    result = currentFandom;
    return result;
}

DiagnosticSQLResult<bool> IgnoreFandom(int id, bool includeCrossovers, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;

    QString qs = QString(" insert into ignored_fandoms (fandom_id, including_crossovers) values (:fandom_id, :including_crossovers) ");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fandom_id", id);
    q.bindValue(":including_crossovers", includeCrossovers);
    result.ExecAndCheck(q, true);
    return result;
}

DiagnosticSQLResult<bool> RemoveFandomFromIgnoredList(int id, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;

    QString qs = QString(" delete from ignored_fandoms where fandom_id  = :fandom_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fandom_id", id);
    result.ExecAndCheck(q, true);
    return result;
}

DiagnosticSQLResult<QStringList> GetIgnoredFandoms(QSqlDatabase db)
{
    DiagnosticSQLResult<QStringList> result;

    QString qs = QString("select name from fandomindex where id in (select fandom_id from ignored_fandoms) order by name asc");

    QSqlQuery q(db);
    q.prepare(qs);
    q.exec();
    if(!result.CheckDataAvailability(q))
        return result;
    do{
        result.data.push_back(q.value("NAME").toString());
    }while(q.next());
    return result;
}


DiagnosticSQLResult<bool> IgnoreFandomSlashFilter(int id, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;

    QString qs = QString(" insert into ignored_fandoms_slash_filter (fandom_id) values (:fandom_id) ");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fandom_id", id);
    result.ExecAndCheck(q, true);
    return result;
}

DiagnosticSQLResult<bool> RemoveFandomFromIgnoredListSlashFilter(int id, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;

    QString qs = QString(" delete from ignored_fandoms_slash_filter where fandom_id  = :fandom_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fandom_id", id);
    result.ExecAndCheck(q, true);
    return result;
}

DiagnosticSQLResult<QStringList> GetIgnoredFandomsSlashFilter(QSqlDatabase db)
{
    DiagnosticSQLResult<QStringList> result;

    QString qs = QString("select name from fandomindex where id in (select fandom_id from ignored_fandoms_slash_filter) order by name asc");

    QSqlQuery q(db);
    q.prepare(qs);
    q.exec();
    if(!result.CheckDataAvailability(q))
        return result;
    do{
        result.data.push_back(q.value("NAME").toString());
    }while(q.next());
    return result;
}

bool CleanupFandom(int fandom_id, QSqlDatabase db)
{
    QString qs = QString("delete from fanfics where id in (select distinct fic_id from ficfandoms where fandom_id = :fandom_id)");

    Transaction transaction(db);

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fandom_id", fandom_id);
    if(!ExecAndCheck(q))
        return false;

    qs = QString("delete from fictags where fic_id in (select distinct fic_id from ficfandoms where fandom_id = :fandom_id)");
    q.prepare(qs);
    q.bindValue(":fandom_id", fandom_id);
    if(!ExecAndCheck(q))
        return false;


    qs = QString("delete from recommendationlistdata where fic_id in (select distinct fic_id from ficfandoms where fandom_id = :fandom_id)");
    q.prepare(qs);
    q.bindValue(":fandom_id", fandom_id);
    if(!ExecAndCheck(q))
        return false;


    qs = QString("delete from ficfandoms where fandom_id = :fandom_id");
    q.prepare(qs);
    q.bindValue(":fandom_id", fandom_id);
    if(!ExecAndCheck(q))
        return false;

    transaction.finalize();
    return true;

}

DiagnosticSQLResult<bool> DeleteFandom(int fandom_id, QSqlDatabase db)
{

    DiagnosticSQLResult<bool> result;
    result.success = false;

    QString qs = QString("delete from fandomindex where id = :fandom_id");

    Transaction transaction(db);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fandom_id", fandom_id);
    if(!result.ExecAndCheck(q))
        return result;

    qs = QString("delete from fandomurls where global_id = :fandom_id");
    q.prepare(qs);
    q.bindValue(":fandom_id", fandom_id);
    if(!result.ExecAndCheck(q))
        return result;

    transaction.finalize();
    result.success = true;
    return result;
}



QStringList GetTrackedFandomList(QSqlDatabase db)
{
    QStringList result;

    QString qs = QString(" select * from fandomindex where tracked = 1 order by name asc");

    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return result;
    while(q.next())
        result.push_back(q.value(0).toString());
    return result;
}

int GetFandomCountInDatabase(QSqlDatabase db)
{
    QString qs = QString("Select count(name) from fandomindex");
    QSqlQuery q(qs, db);
    if(!ExecAndCheck(q))
        return 0;
    if(!q.next())
        return  0;
    return q.value(0).toInt();
}


bool AddFandomForFic(int ficId, int fandomId, QSqlDatabase db)
{
    if(ficId == -1 || fandomId == -1)
        return false;
    QString qs = QString(" insert into ficfandoms (fic_id, fandom_id) values (:fic_id, :fandom_id) ");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fic_id",ficId);
    q.bindValue(":fandom_id",fandomId);
    q.exec();
    if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return false;
    }
    return true;
}
QStringList GetFandomNamesForFicId(int ficId, QSqlDatabase db)
{
    QStringList result;
    QString qs = QString("select name from fandomindex where fandomindex.id in (select fandom_id from ficfandoms ff where ff.fic_id = :fic_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fic_id",ficId);
    if(!ExecAndCheck(q))
        return result;
    while(q.next())
    {
        auto fandom = q.value("name").toString().trimmed();
        if(!fandom.contains("????"))
            result.push_back(fandom);
    }
    return result;
}

DiagnosticSQLResult<bool> AddUrlToFandom(int fandomID, core::Url url, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.success = false;
    if(fandomID == -1)
        return result;

    QString qs = QString(" insert into fandomurls (global_id, url, website, custom) "
                         " values (:global_id, :url, :website, :custom) ");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":global_id",fandomID);
    q.bindValue(":url",url.GetUrl());
    q.bindValue(":website",url.GetSource());
    q.bindValue(":custom",url.GetType());
    q.exec();

    if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        result.oracleError = q.lastError().text();
        return result;
    }
    bool newFandom = !q.lastError().isValid();
    if(url.GetType().isEmpty())
        qDebug() << "empty type";
    if(newFandom)
        qDebug() << "new fandom url: " << url.GetUrl();

    result.success = true;
    return result;

}


QList<int> GetRecommendersForFicIdAndListId(int ficId, QSqlDatabase db)
{
    QList<int> result;
    QString qs = QString("Select distinct recommender_id from recommendations where fic_id = :fic_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fic_id",ficId);
    if(!ExecAndCheck(q))
        return result;
    while(q.next())
        result.push_back(q.value("recommender_id").toInt());
    return result;
}

bool SetFicsAsListOrigin(QList<int> ficIds, int listId, QSqlDatabase db)
{
    QString qs = QString("update RecommendationListData set is_origin = 1 where fic_id = :fic_id and list_id = :list_id");
    QSqlQuery q(db);
    q.prepare(qs);
    for(auto ficId :ficIds)
    {
        q.bindValue(":fic_id",ficId);
        q.bindValue(":list_id",listId);
        if(!ExecAndCheck(q))
            return false;
    }
    return true;
}

bool DeleteLinkedAuthorsForAuthor(int authorId,  QSqlDatabase db)
{
    QString qs = QString("delete from LinkedAuthors where recommender_id = :author_id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":author_id", authorId);
    if(!ExecAndCheck(q))
        return false;
    return true;
}

//bool UploadLinkedAuthorsForAuthor(int authorId, QStringList list, QSqlDatabase db)
//{
//    QSqlQuery q(db);
//    QString qs = QString("insert into  LinkedAuthors(recommender_id, url) values(:author_id, :url)");
//    q.prepare(qs);
//    for(auto url :list)
//    {
//        q.bindValue(":author_id",authorId);
//        q.bindValue(":url",url);
//        q.exec();
//        if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
//        {
//            qDebug() << q.lastError();
//            qDebug() << q.lastQuery();
//            return false;
//        }
//    }
//    return true;
//}
bool UploadLinkedAuthorsForAuthor(int authorId, QString website, QList<int> ids, QSqlDatabase db)
{
    QSqlQuery q(db);
    QString qs = QString("insert into  LinkedAuthors(recommender_id, %1_id) values(:author_id, :id)").arg(website);
    q.prepare(qs);
    for(auto id :ids)
    {
        q.bindValue(":author_id",authorId);
        q.bindValue(":id",id);
        q.exec();
        if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
        {
            qDebug() << q.lastError();
            qDebug() << q.lastQuery();
            return false;
        }
    }
    return true;
}
QStringList GetLinkedPagesForList(int listId, QString website, QSqlDatabase db)
{
    QStringList result;
    QString qs = QString("Select distinct %1_id from LinkedAuthors "
                         " where recommender_id in ( select author_id from RecommendationListAuthorStats where list_id = 17) "
                         " and %1_id not in (select distinct %1_id from recommenders)"
                         " union all "
                         " select ffn_id from recommenders where id not in (select distinct recommender_id from recommendations) and favourites != 0 ");
    qs=qs.arg(website);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return result;
    qDebug() << q.lastQuery();
    while(q.next())
    {
        auto authorUrl = url_utils::GetAuthorUrlFromWebId(q.value(QString("%1_id").arg(website)).toInt(), "ffn");
        result.push_back(authorUrl);
    }
    return result;
}

bool RemoveAuthorRecommendationStatsFromDatabase(int listId, int authorId, QSqlDatabase db)
{
    QString qs = QString("delete from recommendationlistauthorstats "
                         " where list_id = :list_id and author_id = :author_id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id", listId);
    q.bindValue(":author_id", authorId);
    if(!ExecAndCheck(q))
        return false;
    return true;
}

bool CreateFandomIndexRecord(int id, QString name, QSqlDatabase db)
{
    QString qs = QString("insert into fandomindex(id, name) values(:id, :name)");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id", id);
    q.bindValue(":name", name);
    if(!ExecAndCheck(q))
        return false;
    return true;
}



QHash<int, QList<int>> GetWholeFicFandomsTable(QSqlDatabase db)
{
    QHash<int, QList<int>> result;
    QString qs = QString("select fic_id, fandom_id from ficfandoms");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return result;
    while(q.next())
        result[q.value("fandom_id").toInt()].push_back(q.value("fic_id").toInt());
    return result;
}

bool EraseFicFandomsTable(QSqlDatabase db)
{
    QString qs = QString("delete from ficfandoms");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return false;
    return true;
}
bool SetLastUpdateDateForFandom(int id, QDate date, QSqlDatabase db)
{
    QString qs = QString("update fandomindex set updated = :updated where id = :id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":updated", date);
    q.bindValue(":id", id);
    if(!ExecAndCheck(q))
        return false;
    return true;
}

DiagnosticSQLResult<bool> RemoveFandomFromRecentList(QString name, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.success = false;

    QString qs = QString("delete from recent_fandoms where fandom = :name");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":name", name);
    if(!result.ExecAndCheck(q))
        return result;
    result.success = true;
    return result;
}


DiagnosticSQLResult<int> GetLastExecutedTaskID(QSqlDatabase db)
{
    DiagnosticSQLResult<int> result;
    result.data = -1;
    QString qs = QString("select max(id) from pagetasks");

    QSqlQuery q(db);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;

    result.data = q.value(0).toInt();
    return result;
}

bool GetTaskSuccessByID(int id, QSqlDatabase db)
{
    QString qs = QString("select success from pagetasks where id = :id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id", id);
    if(!ExecAndCheck(q))
        return false;
    auto success = q.value(0).toBool();
    return success;
}

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

void FillPageTaskBaseFromQuery(BaseTaskPtr task, QSqlQuery& q){
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


void FillPageTaskFromQuery(PageTaskPtr task, QSqlQuery& q){
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

void FillSubTaskFromQuery(SubTaskPtr task, QSqlQuery& q){
    if(!NullPtrGuard(task))
        return;

    FillPageTaskBaseFromQuery(task, q);
    task->parentId= q.value("task_id").toInt();
    task->id = q.value("sub_id").toInt();
    SubTaskContentBasePtr content;

    if(task->type == 0)
    {
        content = SubTaskAuthorContent::NewContent();
        auto cast = static_cast<SubTaskAuthorContent*>(content.data());
        cast->authors = q.value("content").toString().split("\n");
    }
    if(task->type == 1)
    {
        content = SubTaskFandomContent::NewContent();
        auto cast = static_cast<SubTaskFandomContent*>(content.data());
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
    DiagnosticSQLResult<PageTaskPtr> result; // = PageTask::CreateNewTask();
    result.data = PageTask::CreateNewTask();
    QString qs = QString("select * from pagetasks where id = :id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id", id);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;

    FillPageTaskFromQuery(result.data, q);
    return result;
}

DiagnosticSQLResult<SubTaskList> GetSubTaskData(int id, QSqlDatabase db)
{
    DiagnosticSQLResult<SubTaskList> result;
    QString qs = QString("select * from PageTaskParts where task_id = :id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id", id);
    if(!result.ExecAndCheck(q))
        return result;
    while(q.next())
    {
        auto subtask = PageSubTask::CreateNewSubTask();
        FillSubTaskFromQuery(subtask, q);
        result.data.push_back(subtask);
    }
    return result;
}

void FillPageFailuresFromQuery(PageFailurePtr failure, QSqlQuery& q){
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


DiagnosticSQLResult<SubTaskErrors> GetErrorsForSubTask(int id,  QSqlDatabase db, int subId)
{
    DiagnosticSQLResult<SubTaskErrors> result;
    QString qs = QString("select * from PageWarnings where task_id = :id");
    bool singleSubTask = subId != -1;
    if(singleSubTask)
        qs+= " and sub_id = :sub_id";

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id", id);
    if(singleSubTask)
        q.bindValue(":sub_id", subId);
    if(!result.ExecAndCheck(q))
        return result;
    while(q.next())
    {
        auto failure = PageFailure::CreateNewPageFailure();
        FillPageFailuresFromQuery(failure, q);
        result.data.push_back(failure);
    }
    return result;
}

void FillActionFromQuery(PageTaskActionPtr action, QSqlQuery& q){
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


DiagnosticSQLResult<PageTaskActions> GetActionsForSubTask(int id, QSqlDatabase db, int subId)
{
    DiagnosticSQLResult<PageTaskActions> result;
    QString qs = QString("select * from PageTaskActions where task_id = :id");
    bool singleSubTask = subId != -1;
    if(singleSubTask)
        qs+= " and sub_id = :sub_id";

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id", id);
    if(singleSubTask)
        q.bindValue(":sub_id", subId);
    if(!result.ExecAndCheck(q))
        return result;
    while(q.next())
    {
        auto action = PageTaskAction::CreateNewAction();
        FillActionFromQuery(action, q);
        result.data.push_back(action);
    }
    return result;
}

DiagnosticSQLResult<int> CreateTaskInDB(PageTaskPtr task, QSqlDatabase db)
{
    DiagnosticSQLResult<int> result;
    result.data = -1;
    bool dbIsOpen = db.isOpen();
    Transaction transaction(db);
    QString qs = QString("insert into PageTasks(type, parts, created_at, scheduled_to,  allowed_retry_count, "
                         "allowed_subtask_retry_count, cache_mode, refresh_if_needed, task_comment, task_size, success, finished,"
                         "parsed_pages, updated_fics,inserted_fics,inserted_authors,updated_authors) "
                         "values(:type, :parts, :created_at, :scheduled_to, :allowed_retry_count,"
                         ":allowed_subtask_retry_count, :cache_mode, :refresh_if_needed, :task_comment,:task_size, 0, 0,"
                         " :parsed_pages, :updated_fics,:inserted_fics,:inserted_authors,:updated_authors) ");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":type", task->type);
    q.bindValue(":parts", task->parts);
    q.bindValue(":created_at", task->created);
    q.bindValue(":scheduled_to", task->scheduledTo);

    q.bindValue(":allowed_retry_count", task->allowedRetries);
    q.bindValue(":allowed_subtask_retry_count", task->allowedSubtaskRetries);
    q.bindValue(":cache_mode", static_cast<int>(task->cacheMode));
    q.bindValue(":refresh_if_needed", task->refreshIfNeeded);
    q.bindValue(":task_comment", task->taskComment);
    q.bindValue(":task_size", task->size);
    q.bindValue(":parsed_pages",      task->parsedPages);
    q.bindValue(":updated_fics",      task->updatedFics);
    q.bindValue(":inserted_fics",     task->addedFics);
    q.bindValue(":inserted_authors",  task->addedAuthors);
    q.bindValue(":updated_authors",   task->updatedAuthors);

    if(!result.ExecAndCheck(q))
        return result;

    qs = QString("select max(id) from PageTasks");
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;

    result.data = q.value(0).toInt();
    transaction.finalize();
    return result;
}

DiagnosticSQLResult<bool> CreateSubTaskInDB(SubTaskPtr subtask, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.data = false;
    Transaction transaction(db);
    QString qs = QString("insert into PageTaskParts(task_id, type, sub_id, created_at, scheduled_to, content,task_size, success, finished, parse_up_to,"
                         "custom_data1, parsed_pages, updated_fics,inserted_fics,inserted_authors,updated_authors) "
                         "values(:task_id, :type, :sub_id, :created_at, :scheduled_to, :content,:task_size, 0,0, :parse_up_to,"
                         ":custom_data1, :parsed_pages, :updated_fics,:inserted_fics,:inserted_authors,:updated_authors) ");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":task_id", subtask->parentId);
    q.bindValue(":type", subtask->type);
    q.bindValue(":sub_id", subtask->id);
    q.bindValue(":created_at", subtask->created);
    q.bindValue(":scheduled_to", subtask->scheduledTo);
    q.bindValue(":content", subtask->content->ToDB());
    q.bindValue(":task_size", subtask->size);
    q.bindValue(":parse_up_to", subtask->updateLimit);
    QString customData = subtask->content->CustomData1();
    q.bindValue(":custom_data1",      customData);
    q.bindValue(":parsed_pages",      subtask->parsedPages);
    q.bindValue(":updated_fics",      subtask->updatedFics);
    q.bindValue(":inserted_fics",     subtask->addedFics);
    q.bindValue(":inserted_authors",  subtask->addedAuthors);
    q.bindValue(":updated_authors",   subtask->updatedAuthors);


    if(!result.ExecAndCheck(q))
        return result;
    result.data = true;
    transaction.finalize();
    return result;
}

DiagnosticSQLResult<bool> CreateActionInDB(PageTaskActionPtr action, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.data = false;
    Transaction transaction(db);
    QString qs = QString("insert into PageTaskActions(action_uuid, task_id, sub_id, started_at, finished_at, success) "
                         "values(:action_uuid, :task_id, :sub_id, :started_at, :finished_at, :success) ");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":action_uuid", action->id.toString());
    q.bindValue(":task_id", action->taskId);
    q.bindValue(":sub_id", action->subTaskId);
    q.bindValue(":started_at", action->started);
    q.bindValue(":finished_at", action->finished);
    q.bindValue(":success", action->success);

    if(!result.ExecAndCheck(q))
        return result;

    result.data = true;
    transaction.finalize();
    return result;
}

DiagnosticSQLResult<bool> CreateErrorsInDB(SubTaskErrors errors, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.data = false;
    Transaction transaction(db);
    QString qs = QString("insert into PageWarnings(action_uuid, task_id, sub_id, url, attempted_at, last_seen_at, error_code, error_level, error) "
                         "values(:action_uuid, :task_id, :sub_id, :url, :attempted_at, :last_seen, :error_code, :error_level, :error) ");

    for(auto error: errors)
    {
        QSqlQuery q(db);
        q.prepare(qs);
        q.bindValue(":action_uuid", error->action->id.toString());
        q.bindValue(":task_id", error->action->taskId);
        q.bindValue(":sub_id", error->action->subTaskId);
        q.bindValue(":url", error->url);
        q.bindValue(":attempted_at", error->attemptTimeStamp);
        q.bindValue(":last_seen", error->lastSeen);
        q.bindValue(":error_code", static_cast<int>(error->errorCode));
        q.bindValue(":error_level", static_cast<int>(error->errorlevel));
        q.bindValue(":error", error->error);
        if(!result.ExecAndCheck(q))
            return result;
    }

    result.data = true;
    transaction.finalize();
    return result;
}

DiagnosticSQLResult<bool> UpdateTaskInDB(PageTaskPtr task, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.data = false;
    Transaction transaction(db);
    QString qs = QString("update PageTasks set scheduled_to = :scheduled_to, started_at = :started, finished_at = :finished_at,"
                         " results = :results, retries = :retries, success = :success, task_size = :size, finished = :finished,"
                         " parsed_pages = :parsed_pages, updated_fics = :updated_fics, inserted_fics = :inserted_fics,"
                         " inserted_authors = :inserted_authors, updated_authors = :updated_authors"
                         " where id = :id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":scheduled_to",    task->scheduledTo);
    q.bindValue(":started_at",         task->startedAt);
    q.bindValue(":finished_at",        task->finishedAt);
    q.bindValue(":finished",        task->finished);
    q.bindValue(":results",         task->results);
    q.bindValue(":retries",         task->retries);
    q.bindValue(":success",         task->success);
    q.bindValue(":id",              task->id);
    q.bindValue(":size",              task->size);
    q.bindValue(":parsed_pages",      task->parsedPages);
    q.bindValue(":updated_fics",      task->updatedFics);
    q.bindValue(":inserted_fics",     task->addedFics);
    q.bindValue(":inserted_authors",  task->addedAuthors);
    q.bindValue(":updated_authors",   task->updatedAuthors);

    if(!result.ExecAndCheck(q))
        return result;

    result.data = true;
    transaction.finalize();
    return result;
}

DiagnosticSQLResult<bool> UpdateSubTaskInDB(SubTaskPtr task, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.data = false;
    Transaction transaction(db);
    QString qs = QString("update PageTaskParts set scheduled_to = :scheduled_to, started_at = :started, finished_at = :finished,"
                         " retries = :retries, success = :success, finished = :finished, "
                         " parsed_pages = :parsed_pages, updated_fics = :updated_fics, inserted_fics = :inserted_fics,"
                         " inserted_authors = :inserted_authors, updated_authors = :updated_authors, custom_data1 = :custom_data1"
                         " where task_id = :task_id and sub_id = :sub_id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":scheduled_to",     task->scheduledTo);
    q.bindValue(":started_at",       task->startedAt);
    q.bindValue(":finished_at",      task->finished);
    q.bindValue(":retries",          task->retries);
    q.bindValue(":success",          task->success);
    q.bindValue(":finished",         task->finished);
    q.bindValue(":task_id",          task->parentId);
    q.bindValue(":sub_id",           task->id);
    QString customData = task->content->CustomData1();
    q.bindValue(":custom_data1",      customData);
    q.bindValue(":parsed_pages",      task->parsedPages);
    q.bindValue(":updated_fics",      task->updatedFics);
    q.bindValue(":inserted_fics",     task->addedFics);
    q.bindValue(":inserted_authors",  task->addedAuthors);
    q.bindValue(":updated_authors",   task->updatedAuthors);

    if(!result.ExecAndCheck(q))
        return result;

    result.data = true;
    transaction.finalize();
    return result;
}

DiagnosticSQLResult<bool> SetTaskFinished(int id, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.data = false;
    Transaction transaction(db);
    QString qs = QString("update PageTasks set finished = 1 where id = :task_id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":task_id", id);
    if(!result.ExecAndCheck(q))
        return result;

    result.data = true;
    transaction.finalize();
    return result;
}

DiagnosticSQLResult<TaskList> GetUnfinishedTasks(QSqlDatabase db)
{
    DiagnosticSQLResult<TaskList> result;

    QString qs = QString("select id from pagetasks where finished = 0");

    QSqlQuery q(db);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;

    do{
        QSqlQuery tq(db);
        qs = QString("select * from pagetasks where id = :task_id");
        tq.prepare(qs);
        tq.bindValue(":task_id", q.value(0).toInt());
        if(!result.ExecAndCheck(tq) || !result.CheckDataAvailability(tq))
            continue;
        auto task = PageTask::CreateNewTask();
        FillPageTaskFromQuery(task, tq);
        result.data.push_back(task);
    }while(q.next());
    return result;
}

DiagnosticSQLResult<bool> ExportTagsToDatabase(QSqlDatabase originDB, QSqlDatabase targetDB)
{
    DiagnosticSQLResult<bool> result;
    QString qs = QString("select fic_id, tag from fictags order by fic_id, tag");

    QSqlQuery q(originDB);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    Transaction transaction(targetDB);
    QString insertQS = QString("insert into UserFicTags(ffn_id, ao3_id, sb_id, sv_id, tag) values(:ffn_id, :ao3_id, :sb_id, :sv_id, :tag)");
    QSqlQuery insertQ(targetDB);
    insertQ.prepare(insertQS);
    static auto idHash = GetGlobalIDHash(originDB, " where id in (select distinct fic_id from fictags)");
    while(q.next())
    {

        auto record = idHash.GetRecord(q.value("fic_id").toInt());
        insertQ.bindValue(":ffn_id", record.GetID(("ffn")));
        insertQ.bindValue(":ao3_id", record.GetID(("ao3")));
        insertQ.bindValue(":sb_id",  record.GetID(("sb")));
        insertQ.bindValue(":sv_id",  record.GetID(("sv")));
        insertQ.bindValue(":tag", q.value("tag").toString());
        if(!result.ExecAndCheck(insertQ))
            return result;
    }

    // now we need to export actual user tags table
    qs = QString("select * from tags");
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    QList<QPair<QString, int>> tags;
    while(q.next())
    {
        tags.push_back({q.value("tag").toString(),
                        q.value("id").toInt()});
    }

    insertQS = QString("insert into UserTags(id, tag) values(:id, :tag)");
    insertQ.prepare(insertQS);
    for(auto pair : tags)
    {
        insertQ.bindValue(":tag", pair.first);
        insertQ.bindValue(":id", pair.second);
        if(!result.ExecAndCheck(insertQ))
            return result;
    }

    transaction.finalize();
    return result;
}


DiagnosticSQLResult<bool> ImportTagsFromDatabase(QSqlDatabase currentDB,QSqlDatabase tagImportSourceDB)
{
    DiagnosticSQLResult<bool> result;
    // first we wipe the original tag tables
    QString qs = QString("delete from fictags");
    QSqlQuery q(currentDB);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    qs = QString("delete from tags");
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;


    Transaction transaction(currentDB);
    // now we install new tag selection
    QString insertQS = QString("insert into Tags(id, tag) values(:id, :tag)");
    QSqlQuery insertQ(currentDB);
    insertQ.prepare(insertQS);

    qs = QString("select * from UserTags");
    if(!tagImportSourceDB.isOpen())
        qDebug() << "not open";
    QSqlQuery importTagsQ(tagImportSourceDB);
    importTagsQ.prepare(qs);
    if(!result.ExecAndCheck(importTagsQ))
        return result;

    while(importTagsQ.next())
    {
        insertQ.bindValue(":tag", importTagsQ.value("tag").toString());
        insertQ.bindValue(":id", importTagsQ.value("id").toInt());
        if(!result.ExecAndCheck(insertQ))
            return result;
    }
    // and finally restore fictags (which is the hardest part)
    qs = QString("select * from UserFicTags");
    importTagsQ.prepare(qs);
    if(!result.ExecAndCheck(importTagsQ))
        return result;
    insertQS = QString("insert into FicTags(fic_id, tag) values(:id, :tag)");
    insertQ.prepare(insertQS);
    while(importTagsQ.next())
    {
        auto& idRecord = GrabFicIDFromQuery(importTagsQ, currentDB);
        auto dbId = idRecord.GetID("db");
        if(dbId == -1)
        {
            auto recResult = idRecord.CreateRecord(currentDB);
            qDebug() << "Creating fic record";
            if(!recResult.success)
            {
                result.success = false;
                result.oracleError = recResult.oracleError;
                return result;
            }
            dbId = recResult.data;

        }
        insertQ.bindValue(":tag", importTagsQ.value("tag").toString());
        insertQ.bindValue(":id", dbId);
        if(!result.ExecAndCheck(insertQ))
        {
            qDebug() << "Ffn id: " << idRecord.GetID("ffn") <<  " Db id: " << dbId << " tag:" <<  importTagsQ.value("tag").toString();
            return result;
        }
    }
    transaction.finalize();
    result.data = true;
    return result;
}

//QString qs = QString("select ffn_id, slash_keywords_result, slash_keywords, not_slash_keywords, first_slash_iteration, second_slash_iteration from fanfics "
//                     " where slash_keywords_result = 1 or slash_keywords = 1 or not_slash_keywords = 1 or first_slash_iteration = 1 or second_slash_iteration = 1 "
//                     " order by id");


DiagnosticSQLResult<bool> ExportSlashToDatabase(QSqlDatabase originDB, QSqlDatabase targetDB)
{
    DiagnosticSQLResult<bool> result;
    QString qs = QString("select * from slash_data_ffn  ");

    QSqlQuery q(originDB);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    Transaction transaction(targetDB);
    QString insertQS = QString("insert into slash_data_ffn(ffn_id, keywords_result, keywords_yes, keywords_no, filter_pass_1, filter_pass_2) "
                               " values(:ffn_id, :keywords_result, :keywords_yes, :keywords_no, :filter_pass_1, :filter_pass_2) ");
    QSqlQuery insertQ(targetDB);
    insertQ.prepare(insertQS);

    while(q.next())
    {

        insertQ.bindValue(":ffn_id",           q.value("ffn_id").toInt());
        insertQ.bindValue(":keywords_result",  q.value("keywords_result").toInt());
        insertQ.bindValue(":keywords_yes",     q.value("keywords_yes").toInt());
        insertQ.bindValue(":keywords_no",      q.value("keywords_no").toInt());
        insertQ.bindValue(":filter_pass_1",    q.value("filter_pass_1").toInt());
        insertQ.bindValue(":filter_pass_2",    q.value("filter_pass_2").toInt());

        if(!result.ExecAndCheck(insertQ))
            return result;
    }

    transaction.finalize();
    return result;
}

DiagnosticSQLResult<bool> ImportSlashFromDatabase(QSqlDatabase slashImportSourceDB, QSqlDatabase appDB)
{
    DiagnosticSQLResult<bool> result;

    Transaction transaction(appDB);
    // first we wipe the original slash table
    QString qs = QString("delete from slash_data_ffn");
    QSqlQuery q(appDB);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;

    QString insertQS = QString("insert into slash_data_ffn(ffn_id, keywords_yes, keywords_no, keywords_result, filter_pass_1, filter_pass_2)"
                               "  values(:ffn_id, :keywords_yes, :keywords_no, :keywords_result, :filter_pass_1, :filter_pass_2)");
    QSqlQuery insertQ(appDB);
    insertQ.prepare(insertQS);

    qs = QString("select * from slash_data");
    if(!slashImportSourceDB.isOpen())
        qDebug() << "not open";
    QSqlQuery importTagsQ(slashImportSourceDB);
    importTagsQ.prepare(qs);
    if(!result.ExecAndCheck(importTagsQ))
        return result;

    while(importTagsQ.next())
    {
        insertQ.bindValue(":ffn_id", importTagsQ.value("ffn_id").toInt());
        insertQ.bindValue(":keywords_yes", importTagsQ.value("keywords_yes").toInt());
        insertQ.bindValue(":keywords_no", importTagsQ.value("keywords_no").toInt());
        insertQ.bindValue(":keywords_result", importTagsQ.value("keywords_result").toInt());
        insertQ.bindValue(":filter_pass_1", importTagsQ.value("filter_pass_1").toInt());
        insertQ.bindValue(":filter_pass_2", importTagsQ.value("filter_pass_2").toInt());

        if(!result.ExecAndCheck(insertQ))
            return result;
    }
    transaction.finalize();
    result.data = true;
    return result;
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
    // will create an empty record with just ids to be filled later on
    DiagnosticSQLResult<int> result;
    result.success = false;
    QString query = "INSERT INTO FANFICS (ffn_id, sb_id, sv_id, ao3_id, for_fill) "
                    "VALUES ( :ffn_id, :sb_id, :sv_id, :ao3_id, 1)";
    QSqlQuery q(db);
    q.prepare(query);
    q.bindValue(":ffn_id", ids["ffn"]);
    q.bindValue(":sb_id",  ids["sb"]);
    q.bindValue(":sv_id",  ids["sv"]);
    q.bindValue(":ao3_id", ids["ao3"]);
    if(!result.ExecAndCheck(q))
        return result;
    query = "select max(id) from fanfics";
    q.prepare(query);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    result.success = true;
    result.data = q.value(0).toInt();
    return result;
}




bool AddFandomLink(int oldId, int newId, QSqlDatabase db)
{
    QString qs = QString("select * from fandoms where id = :old_id");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":old_id", oldId);
    if(!ExecAndCheck(q))
        return false;
    q.next();
    QStringList urls;
    urls << q.value("normal_url").toString().trimmed();
    urls << q.value("crossover_url").toString().trimmed();
    urls.removeAll("");
    urls.removeAll("none");
    QString custom = q.value("section").toString();
    for(auto url : urls)
    {
        if(url.trimmed().isEmpty())
            continue;

        qs = QString("insert into fandomurls (global_id, url, website, custom) values(:new_id, :url, 'ffn', :custom)");
        q.prepare(qs);
        q.bindValue(":new_id", newId);
        q.bindValue(":url", url);
        q.bindValue(":custom", custom);
        if(!ExecAndCheck(q))
            return false;
    }
    return true;
}

DiagnosticSQLResult<bool> WriteAuthorFavouriteStatistics(core::AuthorPtr author, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.success = false;

    QString query = "INSERT INTO AuthorFavouritesStatistics ("
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
    QSqlQuery q(db);
    q.prepare(query);
    q.bindValue(":author_id", author->id);
    q.bindValue(":favourites", author->stats.favouriteStats.favourites);
    q.bindValue(":favourites_wordcount", author->stats.favouriteStats.ficWordCount);
    q.bindValue(":average_words_per_chapter", author->stats.favouriteStats.wordsPerChapter);
    q.bindValue(":esrb_type", static_cast<int>(author->stats.favouriteStats.esrbType));
    q.bindValue(":prevalent_mood", static_cast<int>(author->stats.favouriteStats.prevalentMood));
    q.bindValue(":most_favourited_size", static_cast<int>(author->stats.favouriteStats.mostFavouritedSize));
    q.bindValue(":favourites_type", static_cast<int>(author->stats.favouriteStats.sectionRelativeSize));
    q.bindValue(":average_favourited_length", author->stats.favouriteStats.averageLength);
    q.bindValue(":favourite_fandoms_diversity", author->stats.favouriteStats.fandomsDiversity);
    q.bindValue(":explorer_factor", author->stats.favouriteStats.explorerFactor);
    q.bindValue(":mega_explorer_factor", author->stats.favouriteStats.megaExplorerFactor);
    q.bindValue(":crossover_factor", author->stats.favouriteStats.crossoverFactor);
    q.bindValue(":unfinished_factor", author->stats.favouriteStats.unfinishedFactor);
    q.bindValue(":esrb_uniformity_factor", author->stats.favouriteStats.esrbUniformityFactor);
    q.bindValue(":esrb_kiddy", author->stats.favouriteStats.esrbKiddy);
    q.bindValue(":esrb_mature", author->stats.favouriteStats.esrbMature);
    q.bindValue(":genre_diversity_factor", author->stats.favouriteStats.genreDiversityFactor);
    q.bindValue(":mood_uniformity_factor", author->stats.favouriteStats.moodUniformity);
    q.bindValue(":mood_sad", author->stats.favouriteStats.moodSad);
    q.bindValue(":mood_neutral", author->stats.favouriteStats.moodNeutral);
    q.bindValue(":mood_happy", author->stats.favouriteStats.moodHappy);
    q.bindValue(":crack_factor", author->stats.favouriteStats.crackRatio);
    q.bindValue(":slash_factor", author->stats.favouriteStats.slashRatio);
    q.bindValue(":smut_factor", author->stats.favouriteStats.smutRatio);
    q.bindValue(":prevalent_genre", author->stats.favouriteStats.prevalentGenre);
    q.bindValue(":size_tiny", author->stats.favouriteStats.sizeFactors[0]);
    q.bindValue(":size_medium", author->stats.favouriteStats.sizeFactors[1]);
    q.bindValue(":size_large", author->stats.favouriteStats.sizeFactors[2]);
    q.bindValue(":size_huge", author->stats.favouriteStats.sizeFactors[3]);
    q.bindValue(":first_published", author->stats.favouriteStats.firstPublished);
    q.bindValue(":last_published", author->stats.favouriteStats.lastPublished);

    if(!result.ExecAndCheck(q, true))
        return result;
    result.success = true;

    return result;
}

DiagnosticSQLResult<bool> WriteAuthorFavouriteGenreStatistics(core::AuthorPtr author, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.success = false;

    QString query = "INSERT INTO AuthorFavouritesGenreStatistics (author_id, "
                    "General_,Humor,Poetry, Adventure, Mystery, Horror,Parody,Angst, Supernatural, Suspense, "
                    " Romance,SciFi, Fantasy,Spiritual,Tragedy, Drama, Western,Crime,Family,HurtComfort,Friendship, NoGenre) "
                    "VALUES ("
                    ":author_id, :General_,:Humor,:Poetry, :Adventure, :Mystery, :Horror,:Parody,:Angst, :Supernatural, :Suspense, "
                    " :Romance,:SciFi, :Fantasy,:Spiritual,:Tragedy, :Drama, :Western,:Crime,:Family,:HurtComfort,:Friendship, :NoGenre  "
                    ")";
    QSqlQuery q(db);
    q.prepare(query);
    q.bindValue(":author_id", author->id);
    q.bindValue(":General_", author->stats.favouriteStats.genreFactors["General"]);
    q.bindValue(":Humor", author->stats.favouriteStats.genreFactors["Humor"]);
    q.bindValue(":Poetry", author->stats.favouriteStats.genreFactors["Poetry"]);
    q.bindValue(":Adventure", author->stats.favouriteStats.genreFactors["Adventure"]);
    q.bindValue(":Mystery", author->stats.favouriteStats.genreFactors["Mystery"]);
    q.bindValue(":Horror", author->stats.favouriteStats.genreFactors["Horror"]);
    q.bindValue(":Parody", author->stats.favouriteStats.genreFactors["Parody"]);
    q.bindValue(":Angst", author->stats.favouriteStats.genreFactors["Angst"]);
    q.bindValue(":Supernatural", author->stats.favouriteStats.genreFactors["Supernatural"]);
    q.bindValue(":Suspense", author->stats.favouriteStats.genreFactors["Suspense"]);
    q.bindValue(":Romance", author->stats.favouriteStats.genreFactors["Romance"]);
    q.bindValue(":NoGenre", author->stats.favouriteStats.genreFactors["not found"]);
    q.bindValue(":SciFi", author->stats.favouriteStats.genreFactors["Sci-Fi"]);
    q.bindValue(":Fantasy", author->stats.favouriteStats.genreFactors["Fantasy"]);
    q.bindValue(":Spiritual", author->stats.favouriteStats.genreFactors["Spiritual"]);
    q.bindValue(":Tragedy", author->stats.favouriteStats.genreFactors["Tragedy"]);
    q.bindValue(":Drama", author->stats.favouriteStats.genreFactors["Drama"]);
    q.bindValue(":Western", author->stats.favouriteStats.genreFactors["Western"]);
    q.bindValue(":Crime", author->stats.favouriteStats.genreFactors["Crime"]);
    q.bindValue(":Family", author->stats.favouriteStats.genreFactors["Family"]);
    q.bindValue(":HurtComfort", author->stats.favouriteStats.genreFactors["Hurt/Comfort"]);
    q.bindValue(":Friendship", author->stats.favouriteStats.genreFactors["Friendship"]);
    if(!result.ExecAndCheck(q, true))
        return result;
    result.success = true;

    return result;
}

DiagnosticSQLResult<bool> WriteAuthorFavouriteFandomStatistics(core::AuthorPtr author, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.success = false;

    QString query = "INSERT INTO AuthorFavouritesFandomRatioStatistics ("
                    "author_id, fandom_id, fandom_ratio, fic_count) "
                    "VALUES ("
                    ":author_id, :fandom_id, :fandom_ratio, :fic_count"
                    ")";
    QSqlQuery q(db);
    q.prepare(query);
    for(auto fandom : author->stats.favouriteStats.fandomFactorsConverted.keys())
    {
        q.bindValue(":author_id", author->id);
        q.bindValue(":fandom_id", fandom);
        q.bindValue(":fandom_ratio", author->stats.favouriteStats.fandomFactorsConverted[fandom]);
        q.bindValue(":fic_count", author->stats.favouriteStats.fandomsConverted[fandom]);
        if(!result.ExecAndCheck(q, true))
            return result;
    }
    result.success = true;

    return result;
}


DiagnosticSQLResult<bool> WipeAuthorStatistics(core::AuthorPtr author, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.success = false;

    QString deleteGenre = "delete from AuthorFavouritesGenreStatistics where author_id = :author_id";
    QString deleteStats = "delete from AuthorFavouritesStatistics where author_id = :author_id";
    QString deleteFandoms = "delete from AuthorFavouritesFandomRatioStatistics where author_id = :author_id";
    QSqlQuery q(db);
    q.prepare(deleteGenre);
    q.bindValue(":author_id", author->id);
    if(!result.ExecAndCheck(q))
        return result;

    q.prepare(deleteStats);
    q.bindValue(":author_id", author->id);
    if(!result.ExecAndCheck(q))
        return result;

    q.prepare(deleteFandoms);
    q.bindValue(":author_id", author->id);
    if(!result.ExecAndCheck(q))
        return result;

    result.success = true;

    return result;
}

DiagnosticSQLResult<QList<int>> GetAllAuthorRecommendations(int id, QSqlDatabase db)
{
    DiagnosticSQLResult<QList<int>> result;
    result.success = false;

    QString qs = QString("select count(recommender_id) from recommendations where recommender_id = :id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id",id);
    if(!result.ExecAndCheck(q))
        return result;

    if(!result.CheckDataAvailability(q))
        return result;

    int size = q.value(0).toInt();

    result.data.reserve(size);
    qs = QString("select * from recommendations where recommender_id = :id");
    q.prepare(qs);
    q.bindValue(":id", id);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    while(q.next())
    {
        result.data.push_back(q.value("fic_id").toInt());
    }
    result.success = true;
    return result;
}

DiagnosticSQLResult<QSet<int> > GetAllKnownSlashFics(QSqlDatabase db)
{
    DiagnosticSQLResult<QSet<int>> result;
    result.success = false;

    QString qs = QString("select count(fic_id) from algopasses where keywords_pass_result = 1");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;

    if(!result.CheckDataAvailability(q))
        return result;

    int size = q.value(0).toInt();

    result.data.reserve(size);
    qs = QString("select fic_id from algopasses where keywords_pass_result = 1");
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    while(q.next())
    {
        result.data.insert(q.value("fic_id").toInt());
    }
    result.success = true;
    return result;
}


DiagnosticSQLResult<QSet<int> > GetAllKnownNotSlashFics(QSqlDatabase db)
{
    DiagnosticSQLResult<QSet<int>> result;
    result.success = false;

    QString qs = QString("select count(fic_id) from algopasses where keywords_no = 1");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;

    if(!result.CheckDataAvailability(q))
        return result;

    int size = q.value(0).toInt();

    result.data.reserve(size);
    qs = QString("select fic_id from algopasses where keywords_no = 1");
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    do
    {
        result.data.insert(q.value("fic_id").toInt());
    }while(q.next());
    result.success = true;
    return result;
}

DiagnosticSQLResult<QSet<int> > GetSingularFicsInLargeButSlashyLists(QSqlDatabase db)
{
    DiagnosticSQLResult<QSet<int>> result;
    result.success = false;


    QString qs = QString("select fic_id from "
                         " ( "
                         " select fic_id, count(fic_id) as cnt from recommendations where recommender_id in (select author_id from AuthorFavouritesStatistics where slash_factor > 0.5 and slash_factor < 0.85  and favourites > 1000) and fic_id not in ( "
                         " select distinct fic_id from recommendations where recommender_id in (select author_id from AuthorFavouritesStatistics where slash_factor <= 0.5 or slash_factor > 0.85) ) group by fic_id "
                         " ) "
                         " where cnt = 1 ");
    QSqlQuery q(db);

    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    do
    {
        result.data.insert(q.value("fic_id").toInt());
    }while(q.next());
    result.success = true;
    return result;
}
DiagnosticSQLResult<QHash<int, double> > GetDoubleValueHashForFics(QString fieldName, QSqlDatabase db)
{
    DiagnosticSQLResult<QHash<int, double> > result;
    result.success = false;


    QString qs = QString("select id, %1 from fanfics order by id");
    qs= qs.arg(fieldName);
    QSqlQuery q(db);

    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    do
    {
        result.data[q.value("id").toInt()] =q.value(fieldName).toDouble();

    }while(q.next());
    result.success = true;
    return result;
}
DiagnosticSQLResult<QHash<int, std::array<double, 21>>> GetListGenreData(QSqlDatabase db)
{
    DiagnosticSQLResult<QHash<int, std::array<double, 21>>> result;
    result.success = false;


    QString qs = QString("select * from AuthorFavouritesGenreStatistics order by author_id");
    QSqlQuery q(db);

    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    do
    {
        std::size_t counter = 0;
        result.data[q.value("author_id").toInt()][counter++] =q.value("General_").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Humor").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Poetry").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Adventure").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Mystery").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Horror").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Parody").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Angst").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Supernatural").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Romance").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("NoGenre").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("SciFi").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Fantasy").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Spiritual").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Tragedy").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Western").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Crime").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Family").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("HurtComfort").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Friendship").toDouble();
        result.data[q.value("author_id").toInt()][counter++] =q.value("Drama").toDouble();

    }while(q.next());
    result.success = true;
    return result;
}
DiagnosticSQLResult<QHash<int, std::array<double, 21> > > GetFullFicGenreData(QSqlDatabase db)
{
    DiagnosticSQLResult<QHash<int, std::array<double, 21>>> result;
    result.success = false;


    QString qs = QString("select * from FicGenreStatistics order by fic_id");
    QSqlQuery q(db);

    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    do
    {
        std::size_t counter = 0;
        result.data[q.value("fic_id").toInt()][counter++] =q.value("General_").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Humor").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Poetry").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Adventure").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Mystery").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Horror").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Parody").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Angst").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Supernatural").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Romance").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("NoGenre").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("SciFi").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Fantasy").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Spiritual").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Tragedy").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Western").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Crime").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Family").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("HurtComfort").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Friendship").toDouble();
        result.data[q.value("fic_id").toInt()][counter++] =q.value("Drama").toDouble();

    }while(q.next());
    result.success = true;
    return result;
}

DiagnosticSQLResult<QHash<int, double> > GetFicGenreData(QString genre, QString cutoff, QSqlDatabase db)
{
    DiagnosticSQLResult<QHash<int, double> > result;
    result.success = false;


    QString qs = QString("select fic_id, %1 from FicGenreStatistics where %2 order by fic_id");
    QSqlQuery q(db);
    qs = qs.arg(genre).arg(cutoff);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    do
    {
        result.data[q.value("fic_id").toInt()] =q.value(genre).toDouble();

    }while(q.next());
    result.success = true;
    return result;
}



DiagnosticSQLResult<QSet<int> > GetAllKnownFicIds(QString where, QSqlDatabase db)
{
    DiagnosticSQLResult<QSet<int>> result;
    result.success = false;

    QString qs = QString("select count(id) from fanfics where " + where);
    QSqlQuery q(db);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;

    if(!result.CheckDataAvailability(q))
        return result;

    int size = q.value(0).toInt();

    result.data.reserve(size);
    qs = QString("select id from fanfics where " + where);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    while(q.next())
    {
        result.data.insert(q.value("id").toInt());
    }
    result.success = true;
    return result;
}

DiagnosticSQLResult<bool> FillRecommendationListWithData(int listId,
                                                         QHash<int, int> fics,
                                                         QSqlDatabase db)
{
    QString qs = "INSERT INTO RecommendationListData ("
                    "fic_id, list_id, match_count) "
                    "VALUES ("
                    ":fic_id, :list_id, :match_count)";

    SqlContext<bool> ctx(db, qs);
    ctx.q.bindValue(":list_id", listId);
    ctx.ExecuteWithArgsBind({":fic_id", ":match_count"}, fics);
    return ctx.result;
}


DiagnosticSQLResult<bool> CreateSlashInfoPerFic(QSqlDatabase db)
{
    return SqlContext<bool>(db, "insert into algopasses(fic_id) select id from fanfics")();
}

DiagnosticSQLResult<bool> WipeSlashMetainformation(QSqlDatabase db)
{
    QString qs = "update algopasses set "
                 " keywords_yes = 0, keywords_no = 0, keywords_pass_result = 0, "
                 " pass_1 = 0, pass_2 = 0,pass_3 = 0,pass_4 = 0,pass_5 = 0 ";

    return SqlContext<bool>(db, qs)();
}


DiagnosticSQLResult<bool> ProcessSlashFicsBasedOnWords(std::function<SlashPresence(QString, QString, QString)> func, QSqlDatabase db)
{
    DiagnosticSQLResult<bool> result;
    result.success = false;

    CreateSlashInfoPerFic(db);
    qDebug() << "finished creating records for fics";
    WipeSlashMetainformation(db);
    qDebug() << "finished wiping slash info";

    QSet<int> slashFics;
    QSet<int> slashKeywords;
    QSet<int> notSlashFics;
    QString qs = QString("select id, summary, characters, fandom from fanfics order by id asc");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;
    if(!result.CheckDataAvailability(q))
        return result;
    do
    {
        auto result = func(q.value("summary").toString(),
                           q.value("characters").toString(),
                           q.value("fandom").toString());
        if(result.containsSlash)
            slashKeywords.insert(q.value("id").toInt());
        if(result.IsSlash())
            slashFics.insert(q.value("id").toInt());
        if(result.containsNotSlash)
            notSlashFics.insert(q.value("id").toInt());


    }while(q.next());
    qDebug() << "finished processing slash info";
    q.prepare("update algopasses set keywords_pass_result = 1 where fic_id = :id");
    QList<int> list = slashFics.values();
    int counter = 0;
    for(auto tempId: list)
    {
        counter++;
        q.bindValue(":id", tempId);
        if(!result.ExecAndCheck(q))
        {
            qDebug() << "failed to write slash";
            return result;
        }
    }
    q.prepare("update algopasses set keywords_no = 1 where fic_id = :id");
    list = notSlashFics.values();
    counter = 0;
    for(auto tempId: list)
    {
        counter++;
        q.bindValue(":id", tempId);
        if(!result.ExecAndCheck(q))
        {
            qDebug() << "failed to write slash";
            return result;
        }
    }
    q.prepare("update algopasses set keywords_yes = 1 where fic_id = :id");
    list = slashKeywords.values();
    counter = 0;
    for(auto tempId: list)
    {
        counter++;
        q.bindValue(":id", tempId);
        if(!result.ExecAndCheck(q))
        {
            qDebug() << "failed to write slash";
            return result;
        }
    }
    qDebug() << "finished writing slash info into DB";

    result.success = true;
    return result;
}

DiagnosticSQLResult<bool> WipeAuthorStatisticsRecords(QSqlDatabase db)
{
    QString qs = QString("delete from AuthorFavouritesStatistics");
    return SqlContext<bool>(db, qs)();
}

DiagnosticSQLResult<bool> CreateStatisticsRecordsForAuthors(QSqlDatabase db)
{
    QString qs = QString("insert into AuthorFavouritesStatistics(author_id, favourites) select r.id, (select count(*) from recommendations where recommender_id = r.id) from recommenders r");
    return SqlContext<bool>(db, qs)();
}

DiagnosticSQLResult<bool> CalculateSlashStatisticsPercentages(QString usedField,  QSqlDatabase db)
{
    QString qs = QString("update AuthorFavouritesStatistics set slash_factor  = "
                         " cast( (select count (ff.fic_id) from (select fic_id from recommendations where recommender_id = author_id) rs left join  (select fic_id, %1 from algopasses where %1 = 1) ff on ff.fic_id  = rs.fic_id) as float) "
                         " /cast( (select count (ff.fic_id) from (select fic_id from recommendations where recommender_id = author_id) rs left join  (select fic_id, %1 from algopasses) ff on ff.fic_id  = rs.fic_id) as float)");
    qs = qs.arg(usedField);
    return SqlContext<bool>(db, qs)();
}

DiagnosticSQLResult<bool> AssignIterationOfSlash(QString iteration, QSqlDatabase db)
{
    QString qs = QString("update algopasses set %1 = 1 where keywords_pass_result = 1 or fic_id in "
                         "(select fic_id from recommendationlistdata where list_id in (select id from recommendationlists where name = 'SlashCleaned'))");
    qs = qs.arg(iteration);
    return SqlContext<bool>(db, qs)();
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

    QString qs = QString("update ficgenrestatistics set %1 =  CASE WHEN  (select count(distinct recommender_id) from recommendations r where r.fic_id = ficgenrestatistics .fic_id) >= 5 "
                         " THEN (select avg(%1) from AuthorFavouritesGenreStatistics afgs where afgs.author_id in (select distinct recommender_id from recommendations r where r.fic_id = ficgenrestatistics .fic_id))  "
                         " ELSE 0 END ");
    SqlContext<bool> ctx(db, qs);
    ctx.ExecuteWithArgs(result.keys());
    return ctx.result;

}














}

}



//bool RebindFicsToIndex(int oldId, int newId, QSqlDatabase db)
//{
//    QString qs = QString("update ficfandoms set fandom_id = :new_id, reassigned = 1 where fandom_id = :old_id and reassigned != 1");

//    QSqlQuery q(db);
//    q.prepare(qs);
//    q.bindValue(":old_id", oldId);
//    q.bindValue(":new_id", newId);
//    if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
//    {
//        qDebug() << q.lastError();
//        qDebug() << q.lastQuery();
//        return false;
//    }
//    return true;
//}

