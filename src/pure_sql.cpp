/*
FFSSE is a replacement search engine for fanfiction.net search results
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
#include "section.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVector>
#include <QVariant>
#include <QDebug>
namespace database {
namespace puresql{

bool ExecAndCheck(QSqlQuery& q)
{
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << "Error while performing a query: ";
        qDebug() << q.lastQuery();
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

bool SetFandomTracked(int id, bool tracked,  QSqlDatabase db)
{
    QSqlQuery q1(db);
    QString qsl = " UPDATE fandoms SET tracked = :tracked where id = :id";
    q1.prepare(qsl);
    q1.bindValue(":tracked",QString(tracked ? "1" : "0"));
    q1.bindValue(":id",id);
    if(!ExecAndCheck(q1))
        return false;

    return true;
}

void CalculateFandomsAverages(QSqlDatabase db)
{
    QString qs = QString("update fandoms set average_faves_top_3 =  (select sum(favourites)/3 from fanfics f where f.fandom = fandoms.fandom and f.id "
                         "in (select id from fanfics where fanfics.fandom = fandoms.fandom order by favourites desc limit 3))");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return;
    return;
}

void CalculateFandomsFicCounts(QSqlDatabase db)
{
    QString qs = QString("update fandoms set fic_count = (select count(fic_id) from ficfandoms where fandom_id = fandoms.id");
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
    QString qs = QString("update fandoms set fic_count = "
                         " (select count(fic_id) from ficfandoms where fandom_id = fandoms.id),"
                         " average_faves_top_3 = (select sum(favourites)/3 from fanfics f where f.fandom = fandoms.fandom and f.id "
                         " in (select id from fanfics where fanfics.fandom = fandoms.fandom order by favourites desc limit 3))"
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
    QString qs = QString("Select max(updated) as updated from fanfics where id in (select distinct fic_id from FicFandoms where fandom_id = :fandom_id %1");
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
    QString qs = QString("Select fandom from fandoms where normal_url is not null");
    QSqlQuery q(qs, db);
    QStringList result;
    result.append("");
    while(q.next())
        result.append(q.value(0).toString());
    CheckExecution(q);

    return result;
}

void AssignTagToFandom(QString tag, int fandom_id, QSqlDatabase db)
{
    QString qs = "INSERT INTO FicTags(fic_id, tag) SELECT fic_id, '%1' as tag from FicFandoms f WHERE fandom_id = :fandom_id "
                 " and NOT EXISTS(SELECT 1 FROM FicTags WHERE fic_id = f.fic_id and tag = '%1')";
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
    qs=qs.arg(tag);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fic_id", fic_id);
    q.bindValue(":tag", tag);
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

bool CreateFandomInDatabase(QSharedPointer<core::Fandom> fandom, QSqlDatabase db)
{
    QString qs = QString("insert into fandoms(fandom, section, normal_url, crossover_url) "
                         " values(:fandom, :section, :normal_url, :crossover_url)");
    QSqlQuery q(db);
    q.prepare(qs);

    // I probably need to delete all the bullshit japanese stuff from fandom names
    q.bindValue(":fandom", fandom->name);
    q.bindValue(":section", fandom->section);
    q.bindValue(":normal_url", fandom->url);
    q.bindValue(":crossover_url", fandom->crossoverUrl);

    if(!ExecAndCheck(q))
        return false;
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
    //fic->tags = q1.value("TAGS").toString();
    //fic->language = q1.value("LANGUAGE").toString();
    fic->published = q1.value("PUBLISHED").toDateTime();
    fic->updated = q1.value("UPDATED").toDateTime();
    fic->characters = q1.value("CHARACTERS").toString().split(",");
    fic->authorId = q1.value("AUTHOR_ID").toInt();
    fic->author->name = q1.value("AUTHOR").toString();
    fic->ffn_id = q1.value("FFN_ID").toInt();
    fic->ao3_id = q1.value("AO3_ID").toInt();
    fic->sb_id = q1.value("SB_ID").toInt();
    fic->sv_id = q1.value("SV_ID").toInt();
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
                          " ( select count(id) from FANFICS where  %1_id = :site_id and updated <> :updated) as count_updated"
                          ;

    QString filledQuery = getKeyQuery.arg(fic->webSite);
    if(fic->title.contains("Fire Princess"))
        qDebug() << filledQuery;
    QSqlQuery q(db);
    q.prepare(filledQuery);
    q.bindValue(":updated", fic->updated);
    q.bindValue(":site_id", fic->webId);
    //qDebug() << getKeyQuery;
    if(!ExecAndCheck(q))
        return false;

    if(!q.next())
        return false;

    bool requiresInsert = q.value(0).toInt() == 0;
    if(alwaysUpdateIfNotInsert || (q.value(0).toInt() > 0 && q.value(1).toInt() > 0))
        fic->updateMode = core::UpdateMode::update;
    if(requiresInsert)
        fic->updateMode = core::UpdateMode::insert;
    return true;
}

bool InsertIntoDB(QSharedPointer<core::Fic> section, QSqlDatabase db)
{
    QString query = "INSERT INTO FANFICS (%1_id, FANDOM, AUTHOR, TITLE,WORDCOUNT, CHAPTERS, FAVOURITES, REVIEWS, "
                    " CHARACTERS, COMPLETE, RATED, SUMMARY, GENRES, PUBLISHED, UPDATED, AUTHOR_ID, AUTHOR_WEB_ID,"
                    " wcr, reviewstofavourites, age, daysrunning ) "
                    "VALUES ( :site_id,  :fandom, :author, :title, :wordcount, :CHAPTERS, :FAVOURITES, :REVIEWS, "
                    " :CHARACTERS, :COMPLETE, :RATED, :summary, :genres, :published, :updated, :author_id,:author_web_id,"
                    " :wcr, :reviewstofavourites, :age, :daysrunning )";
    query=query.arg(section->webSite);
    QSqlQuery q(db);
    q.prepare(query);
    q.bindValue(":site_id",section->webId); //?
    q.bindValue(":fandom",section->fandom);
    q.bindValue(":author",section->author->name); //?
    q.bindValue(":author_id",section->author->id);
    q.bindValue(":author_web_id",section->author->webId);
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
        return false;
    }
    return true;

}
bool UpdateInDB(QSharedPointer<core::Fic> section, QSqlDatabase db)
{
    QString query = "UPDATE FANFICS set fandom = :fandom, wordcount= :wordcount, CHAPTERS = :CHAPTERS,  "
                    "COMPLETE = :COMPLETE, FAVOURITES = :FAVOURITES, REVIEWS= :REVIEWS, CHARACTERS = :CHARACTERS, RATED = :RATED, "
                    "summary = :summary, genres= :genres, published = :published, updated = :updated, author_id = :author_id,  author_web_id = :author_web_id,"
                    "wcr= :wcr, reviewstofavourites = :reviewstofavourites, age = :age, daysrunning = :daysrunning "
                    " where %1_id = :site_id";
    query=query.arg(section->webSite);
    QSqlQuery q(db);
    q.prepare(query);
    q.bindValue(":fandom",section->fandom);
    q.bindValue(":author",section->author->name);
    q.bindValue(":author_id",section->author->id);
    q.bindValue(":author_web_id",section->author->webId);
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

core::AuthorPtr AuthorFromQuery(QSqlQuery& q)
{
    core::AuthorPtr result(new core::Author);
    result->AssignId(q.value("id").toInt());
    result->name = q.value("name").toString();
    result->website = q.value("website").toString();
    result->recCount = q.value("rec_count").toInt();
    result->SetUrl("ffn", q.value("url").toString());
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

    qs = QString("select distinct id,name, url, website_type as website, "
                 "(select count(fic_id) from recommendations where recommender_id = recommenders.id) as rec_count "
                 " from recommenders where website_type = :site");
    q.prepare(qs);
    q.bindValue(":site",website);
    if(!ExecAndCheck(q))
        return result;

    result.reserve(size);
    while(q.next())
    {
        auto author = AuthorFromQuery(q);
        result.push_back(author);
    }
    return result;
}



QList<core::AuthorPtr> GetAuthorsForRecommendationList(int listId,  QSqlDatabase db)
{
    QList<core::AuthorPtr> result;

    QSqlQuery q(db);
    QString qs = QString("select id,name, url, website_type as website from recommenders where id in ( select author_id from RecommendationListAuthorStats where list_id = :list_id )");
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
    QString qs = QString("select id,name, url, website_type as website from recommenders where website_type = :site and name = :name");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":site",website);
    q.bindValue(":name",name);
    if(!ExecAndCheck(q))
        return result;

    if(!q.next())
        return result;

    result = AuthorFromQuery(q);
    result->website = website;
    return result;
}

core::AuthorPtr GetAuthorByUrl(QString url, QSqlDatabase db)
{
    core::AuthorPtr result;
    QString qs = QString("select id,name, url, website_type as website from recommenders where url = :url");

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
    QString qs = QString("select id,name, url, website_type as website,  "
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

    q.next();
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
    if(!ExecAndCheck(q))
        return false;
    return true;
}
bool CreateOrUpdateRecommendationList(QSharedPointer<core::RecommendationList> list, QDateTime creationTimestamp, QSqlDatabase db)
{
    QString qs = QString("insert into RecommendationLists(name) select '%1' where not exists(select 1 from RecommendationLists where name = '%1')");
    qs= qs.arg(list->name);
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return false;
    //CURRENT_TIMESTAMP not portable
    qs = QString("update RecommendationLists set minimum = :minimum, pick_ratio = :pick_ratio, always_pick_at = :always_pick_at,  created = :created where name = :name");
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

    QString qs = QString("update RecommendationLists set fic_count=(select count(fic_id) from RecommendationListData where list_id = :list_id) where id = :list_id");
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

bool WriteAuthor(core::AuthorPtr author, QDateTime timestamp, QSqlDatabase db)
{
    QSqlQuery q1(db);
    QString qsl = " insert into recommenders(name, url, page_updated) values(:name, :url,  :timestamp) ";
    q1.prepare(qsl);
    q1.bindValue(":name", author->name);
    q1.bindValue(":time", timestamp);
    q1.bindValue(":url", author->url("ffn"));

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
    for(QString tag : tagList)
    {
        QString qs = QString("INSERT INTO TAGS (TAG) VALUES (:tag)");
        QSqlQuery q(db);
        q.prepare(qs);
        q.bindValue(":tag", tag);
        q.exec();
        if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
        {
            success = false;
            qDebug() << q.lastError().text();
        }
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

static core::FandomPtr FandomfromQuery (QSqlQuery& q, core::FandomPtr fandom = core::FandomPtr())
{
    if(!fandom)
    {
        fandom = core::Fandom::NewFandom();
        fandom->id = q.value("ID").toInt();
        fandom->url = q.value("NORMAL_URL").toString();
        fandom->crossoverUrl = q.value("CROSSOVER_URL").toString();
        fandom->ficCount = q.value("fic_count").toInt();
        fandom->averageFavesTop3 = q.value("average_faves_top_3").toDouble();
        fandom->name = q.value("fandom").toString();
        fandom->source = q.value("source").toString();
        fandom->dateOfCreation = q.value("date_of_creation").toDate();
        fandom->dateOfFirstFic = q.value("date_of_first_fic").toDate();
        fandom->dateOfLastFic = q.value("date_of_last_fic").toDate();
        fandom->lastUpdateDate = q.value("last_update").toDate();
        fandom->tracked = q.value("tracked").toInt();
        fandom->mergedUrls.push_back(fandom->url);
        fandom->mergedUrls.push_back(fandom->crossoverUrl);
    }
    else
    {
        fandom->mergedUrls.push_back(q.value("NORMAL_URL").toString());
        fandom->mergedUrls.push_back(q.value("CROSSOVER_URL").toString());
    }
    return fandom;
};

static core::FandomPtr FandomfromOldSingleTableQuery (QSqlQuery& q, core::FandomPtr fandom = core::FandomPtr())
{
    if(!fandom)
    {
        fandom = core::Fandom::NewFandom();
        fandom->id = q.value("ID").toInt();
        fandom->url = q.value("NORMAL_URL").toString();
        fandom->crossoverUrl = q.value("CROSSOVER_URL").toString();
        fandom->ficCount = q.value("fic_count").toInt();
        fandom->averageFavesTop3 = q.value("average_faves_top_3").toDouble();
        fandom->name = q.value("fandom").toString();
        fandom->source = q.value("source").toString();
        fandom->dateOfCreation = q.value("date_of_creation").toDate();
        fandom->dateOfFirstFic = q.value("date_of_first_fic").toDate();
        fandom->dateOfLastFic = q.value("date_of_last_fic").toDate();
        fandom->lastUpdateDate = q.value("last_update").toDate();
        fandom->tracked = q.value("tracked").toInt();
        fandom->mergedUrls.push_back(fandom->url);
        fandom->mergedUrls.push_back(fandom->crossoverUrl);
    }
    else
    {
        fandom->mergedUrls.push_back(q.value("NORMAL_URL").toString());
        fandom->mergedUrls.push_back(q.value("CROSSOVER_URL").toString());
    }
    return fandom;
};


QList<core::FandomPtr> GetAllFandoms(QSqlDatabase db)
{
    QList<core::FandomPtr> result;

    QString qs = QString(" select count(id) from fandoms");

    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return result;

    result.reserve(q.value(0).toInt());

    qs = QString(" select * from fandoms order by fandom");
    q.prepare(qs);

    if(!ExecAndCheck(q))
        return result;
    QString lastName;
    core::FandomPtr currentFandom;
    while(q.next())
    {
        auto currentName = q.value("fandom").toString();
        if(lastName != currentName)
        {
            currentFandom = FandomfromQuery(q);
            result.push_back(currentFandom);
        }
        else
            currentFandom = FandomfromQuery(q, currentFandom);
        lastName = currentName;
    }

    return result;
}

QList<core::FandomPtr> GetAllFandomsFromSingleTable(QSqlDatabase db)
{
    QList<core::FandomPtr> result;

    QString qs = QString(" select count(id) from fandoms");

    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return result;
    q.next();
    result.reserve(q.value(0).toInt());

    qs = QString(" select * from fandoms order by fandom");
    q.prepare(qs);

    if(!ExecAndCheck(q))
        return result;
    QString lastName;
    core::FandomPtr currentFandom;
    while(q.next())
    {
        auto currentName = q.value("fandom").toString();
        if(lastName != currentName)
        {
            currentFandom = FandomfromOldSingleTableQuery(q);
            result.push_back(currentFandom);
        }
        else
            currentFandom = FandomfromOldSingleTableQuery(q, currentFandom);
        lastName = currentName;
    }

    return result;
}

core::FandomPtr GetFandom(QString name, QSqlDatabase db)
{
    core::FandomPtr result;

    QString qs = QString(" select * from fandoms where fandom = :fandom ");

    if(name == "Voltron: Legendary Defender")
        qDebug() << "ensuring voltron";

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fandom", name);
    if(!ExecAndCheck(q))
        return result;

    QString lastName;
    core::FandomPtr currentFandom;
    while(q.next())
    {
        auto currentName = q.value("fandom").toString();
        if(lastName != currentName)
        {
            currentFandom = FandomfromQuery(q);
        }
        else
            currentFandom = FandomfromQuery(q, currentFandom);
        lastName = currentName;
    }
    result = currentFandom;
    return result;
}


bool CleanuFandom(int fandom_id, QSqlDatabase db)
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


QStringList GetTrackedFandomList(QSqlDatabase db)
{
    QStringList result;

    QString qs = QString(" select * from fandoms where tracked = 1 order by name asc");

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
    QString qs = QString("Select count(fandom) from fandoms");
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
    QString qs = QString("select fandom from fandoms where fandoms.id in (select fandom_id from ficfandoms ff where ff.fic_id = :fic_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":fic_id",ficId);
    if(!ExecAndCheck(q))
        return result;
    while(q.next())
    {
       auto fandom = q.value("fandom").toString().trimmed();
        if(!fandom.contains("????"))
            result.push_back(fandom);
    }
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

bool UploadLinkedAuthorsForAuthor(int authorId, QStringList list, QSqlDatabase db)
{
    QSqlQuery q(db);
    QString qs = QString("insert into  LinkedAuthors(recommender_id, url) values(:author_id, :url)");
    q.prepare(qs);
    for(auto url :list)
    {
        q.bindValue(":author_id",authorId);
        q.bindValue(":url",url);
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

QStringList GetLinkedPagesForList(int listId, QSqlDatabase db)
{
    QStringList result;
    QString qs = QString("Select distinct url from LinkedAuthors where recommender_id in ( select author_id from RecommendationListAuthorStats where list_id = :list_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return result;
    while(q.next())
        result.push_back(q.value("url").toString());
    return result;
}

bool RemoveAuthorRecommendationStatsFromDatabase(int listId, int authorId, QSqlDatabase db)
{
    QString qs = QString("delete from recommendationlistauthorstats where list_id = :list_id and author_id = :author_id");

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
//    if(urls.size())
//        qDebug() << urls;
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

bool RebindFicsToIndex(int oldId, int newId, QSqlDatabase db)
{
    QString qs = QString("update ficfandoms set fandom_id = :new_id, reassigned = 1 where fandom_id = :old_id and reassigned != 1");

    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":old_id", oldId);
    q.bindValue(":new_id", newId);
    if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return false;
    }
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


}

}
