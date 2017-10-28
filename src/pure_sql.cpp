#include "pure_sql.h"
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
        qDebug() << "SQLERROR: " <<  q.lastError();
        qDebug() << q.lastQuery();
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

void CalculateFandomAverages(QSqlDatabase db)
{
    QString qs = QString("update fandoms set average_faves_top_3 =  (select sum(favourites)/3 from fanfics f where f.fandom = fandoms.fandom and f.id "
                         "in (select id from fanfics where fanfics.fandom = fandoms.fandom order by favourites desc limit 3))");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return;
    return;
}

void CalculateFandomFicCounts(QSqlDatabase db)
{
    QString qs = QString("update fandoms set fic_count = (select count(fic_id) from ficfandoms where fandom_id = fandoms.id");
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return;
    return;
}

bool WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom, QSqlDatabase db)
{
    bool result = Internal::WriteMaxUpdateDateForFandom(fandom, "having count(*) = 1", db,
                                          [](QSharedPointer<core::Fandom> f, QDateTime dt){
        f->lastUpdateDate = dt;
    });

    if(fandom->source == "ffn")
        result = result && Internal::WriteMaxUpdateDateForFandom(fandom, "having count(*) > 1", db,
                                              [](QSharedPointer<core::Fandom> f, QDateTime dt){
            f->lastCrossoverUpdateDate = dt;
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
    QString qs = "INSERT INTO FicTags(fic_id, tag) SELECT fic_id, %1 as tag from FicFandoms f WHERE fandom_id = :fandom_id "
                 " and NOT EXISTS(SELECT 1 FROM FicTags WHERE fic_id = f.id and tag = %1)";
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

bool SetUpdateOrInsert(QSharedPointer<core::Fic> fic, QSqlDatabase db, bool alwaysUpdateIfNotInsert)
{
    if(!fic)
        return false;

    QString getKeyQuery = "Select ( select count(*) from FANFICS where  %1_id = :site_id) as COUNT_NAMED,"
                          " ( select count(*) from FANFICS where  %1_id = :site_id and updated <> :updated) as count_updated"
                          " FROM FANFICS WHERE 1=1 ";

    QString filledQuery = getKeyQuery.arg(fic->webSite);
    QSqlQuery q(db);
    q.prepare(filledQuery);
    q.bindValue(":updated", fic->updated);
    q.bindValue(":site_id", fic->webId);
    if(!ExecAndCheck(q))
        return false;

    q.next();
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
                    " CHARACTERS, COMPLETE, RATED, SUMMARY, GENRES, PUBLISHED, UPDATED, AUTHOR_ID) "
                    "VALUES ( :site_id,  :fandom, :author, :title, :wordcount, :CHAPTERS, :FAVOURITES, :REVIEWS, "
                    " :CHARACTERS, :COMPLETE, :RATED, :summary, :genres, :published, :updated, :author_id)";
    query=query.arg(section->webSite);
    QSqlQuery q(db);
    q.prepare(query);
    q.bindValue(":site_id",section->webId); //?
    q.bindValue(":fandom",section->fandom);
    q.bindValue(":author",section->author->name); //?
    q.bindValue(":author_id",section->author->id);
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
    q.exec();
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
    bool loaded = false;
    QString query = "UPDATE FANFICS set fandom = :fandom, wordcount= :wordcount, CHAPTERS = :CHAPTERS,  "
                    "COMPLETE = :COMPLETE, FAVOURITES = :FAVOURITES, REVIEWS= :REVIEWS, CHARACTERS = :CHARACTERS, RATED = :RATED, "
                    "summary = :summary, genres= :genres, published = :published, updated = :updated, author_id = :author_id"
                    " where %1_id = :site_id";
    query=query.arg(section->webSite);
    QSqlQuery q(db);
    q.prepare(query);
    q.bindValue(":fandom",section->fandom);
    q.bindValue(":author",section->author->name);
    q.bindValue(":author_id",section->author->id);
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
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << "failed to update: " << section->author->name << " " << section->title;
        qDebug() << q.lastError();
        return false;
    }
    return true;
}

bool WriteRecommendation(QSharedPointer<core::Author> author, int fic_id, QSqlDatabase db)
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

bool AssignNewNameToAuthorWithId(QSharedPointer<core::Author> author, QSqlDatabase db)
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

QList<QSharedPointer<core::Author>> GetAllAuthors(QString website,  QSqlDatabase db)
{
    QList<QSharedPointer<core::Author>> result;
    QString qs = QString("select count(id) from recommenders where website_type = :site");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":site",website);
    if(!ExecAndCheck(q))
        return result;

    q.next();
    int size = q.value(0).toInt();

    qs = QString("select id,name, url from recommenders where website_type = :site");
    q.prepare(qs);
    q.bindValue(":site",website);
    if(!ExecAndCheck(q))
        return result;

    result.reserve(size);
    while(q.next())
    {
        QSharedPointer<core::Author> rec(new core::Author);
        rec->AssignId(q.value("id").toInt());
        rec->name = q.value("name").toString();
        rec->SetUrl("ffn", q.value("url").toString());
        result.push_back(rec);
    }
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
QList<QSharedPointer<core::AuthorRecommendationStats>> GetRecommenderStatsForList(int listId, QString sortOn, QString order, QSqlDatabase db)
{
    QList<QSharedPointer<core::AuthorRecommendationStats>> result;
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
        QSharedPointer<core::AuthorRecommendationStats> stats(new core::AuthorRecommendationStats);
        stats->isValid = true;
        stats->matchesWithReference = q.value("match_count").toInt();
        stats->matchRatio= q.value("match_ratio").toDouble();
        stats->authorId= q.value("author_id").toInt();
        stats->listId= listId;
        stats->totalFics= q.value("fic_count").toInt();
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
    QString qs = QString("delete from RecommendationLists where list_id = :list_id");
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

    qs = QString("delete from RecommendationFicStats where list_id = :list_id");
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
bool WriteAuthorRecommendationStatsForList(int listId, QSharedPointer<core::AuthorRecommendationStats> stats, QSqlDatabase db)
{
    if(!stats)
        return false;

    QString qs = QString("insert into RecommendationListAuthorStats (author_id, fic_count, match_count, match_ratio, list_id) "
                         "values(:author_id, :fic_count, :match_count, :match_ratio, :list_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":author_id",stats->authorId);
    q.bindValue(":fic_count",stats->totalFics);
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

    QString qs = QString("update RecommendationLists set fic_count=(select count(fic_id) from RecommendationListData where list_id = :list_id) where list_id = :list_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(!ExecAndCheck(q))
        return false;
    return true;
}
bool DeleteTagfromDatabase(QString tag, QSqlDatabase db)
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

bool WriteAuthor(QSharedPointer<core::Author> author, QDateTime timestamp, QSqlDatabase db)
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

bool AssignNewNameForAuthor(QSharedPointer<core::Author> author, QString name, QSqlDatabase db)
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

    QString qs = QString("select id from recommenders");
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

}
}
