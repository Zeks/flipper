#include "include/db_ffn.h"
#include "url_utils.h"
#include <quazip/quazip.h>
#include <quazip/JlCompress.h>

#include <QFile>
#include <QTextStream>
#include <QSqlQuery>
#include <QString>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <algorithm>
#include <QSqlDriver>
#include <QPluginLoader>
#include "sqlite/sqlite3.h"
#include "sqlitefunctions.h"
#include "pure_sql.h"

namespace database{
///////////////////// TRACKED FANDOMS BLOCK ////////////////////////
void FFN::SetFandomTracked(QString fandom, bool tracked)
{
    data.fandoms->SetTracked(fandom, tracked, true);
}
bool FFN::FetchTrackStateForFandom( QString fandom)
{
    return data.fandoms->IsTracked(fandom);
}

QStringList FFN::FetchTrackedFandoms()
{
    return data.fandoms->AllTrackedStr();
}

///////////////////// END TRACKED FANDOMS BLOCK ////////////////////////
QHash<QString, core::Author> FetchRecommenders()
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = "select * from recommenders authors order by name asc";
    q1.prepare(qsl);
    q1.exec();
    QHash<QString, core::Author> result;
    while(q1.next())
    {
        core::Author rec;
        rec.id = q1.value("ID").toInt();
        rec.name= q1.value("name").toString();
        rec.SetUrl("ffn", q1.value("url").toString());
        result[rec.name] = rec;
    }
    CheckExecution(q1);
    return result;
}


int GetMatchCountForRecommenderOnList(int recommender_id, int list)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = "select fic_count from RecommendationListAuthorStats where list_id = :list_id and author_id = :author_id";
    q1.prepare(qsl);
    q1.bindValue(":list_id", list);
    q1.bindValue(":author_id", recommender_id);
    q1.exec();
    q1.next();
    CheckExecution(q1);
    int matches  = q1.value(0).toInt();
    qDebug() << "Using query: " << q1.lastQuery();
    qDebug() << "Matches found: " << matches;
    return matches;
}



void DropAllFanficIndexes()
{
    QStringList commands;
    commands.push_back("Drop index if exists main.I_FANFICS_IDENTITY");
    commands.push_back("Drop index if exists main.I_FANFICS_FANDOM");
    commands.push_back("Drop index if exists main.I_FANFICS_AUTHOR");
    commands.push_back("Drop index if exists main.I_FANFICS_TITLE");
    commands.push_back("Drop index if exists main.I_FANFICS_WORDCOUNT");
    commands.push_back("Drop index if exists main.I_FANFICS_TAGS");
    commands.push_back("Drop index if exists main.I_FANFICS_ID");
    commands.push_back("Drop index if exists main.I_FANFICS_GENRES");
    commands.push_back("Drop index if exists main.I_RECOMMENDATIONS_FIC_TAG");
    commands.push_back("Drop index if exists main.I_RECOMMENDATIONS_TAG");
    commands.push_back("Drop index if exists main.I_RECOMMENDATIONS_FIC");

    QSqlDatabase db = QSqlDatabase::database();
    for(QString command: commands)
    {
        QSqlQuery q1(db);
        q1.prepare(command);
        ExecAndCheck(q1);
    }
}

void RebuildAllFanficIndexes()
{
    QStringList commands;
    commands.push_back("CREATE INDEX  if  not exists  main.I_FANFICS_IDENTITY ON FANFICS (AUTHOR ASC, TITLE ASC)");
    commands.push_back("CREATE  INDEX if  not exists  main.I_FANFICS_AUTHOR ON FANFICS (AUTHOR ASC)");
    commands.push_back("CREATE  INDEX if  not exists  main.I_FANFICS_TITLE ON FANFICS (TITLE ASC)");
    commands.push_back("CREATE  INDEX if  not exists  main.I_FANFICS_FANDOM ON FANFICS (FANDOM ASC)");
    commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_WORDCOUNT ON FANFICS (WORDCOUNT ASC)");
    commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_TAGS ON FANFICS (TAGS ASC)");
    commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_ID ON FANFICS (ID ASC)");
    commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_GENRES ON FANFICS (GENRES ASC)");

    commands.push_back("CREATE INDEX if not exists I_RECOMMENDATIONS_REC_FIC ON Recommendations (recommender_id ASC, fic_id asc)");
    commands.push_back("CREATE INDEX if not exists I_RECOMMENDATIONS_TAG ON Recommendations (tag ASC)");
    commands.push_back("CREATE INDEX if not exists I_RECOMMENDATIONS_FIC ON Recommendations (fic_id ASC)");

    QSqlDatabase db = QSqlDatabase::database();
    for(QString command: commands)
    {
        QSqlQuery q1(db);
        q1.prepare(command);
        ExecAndCheck(q1);
    }
}



core::AuthorRecommendationStats CreateRecommenderStats(int recommenderId, core::RecommendationList list)
{
    core::AuthorRecommendationStats result;
    result.listName = list.name;
    result.usedTag = list.tagToUse;
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select count(distinct fic_id) from recommendations where recommender_id = :id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id",recommenderId);
    q.exec();
    q.next();
    if(!CheckExecution(q))
        return result;

    result.authorId = recommenderId;
    result.totalFics = q.value(0).toInt();

    //auto listId= GetRecommendationListIdForName(list.name);
    //!!! проверить
    qs = QString("select count(distinct fic_id) from FicTags ft where ft.tag = :tag and exists (select 1 from Recommendations where ft.fic_id = fic_id and recommender_id = :recommender_id)");
    q.prepare(qs);
    q.bindValue(":tag",list.tagToUse);
    q.bindValue(":recommender_id",recommenderId);
    q.exec();
    if(!CheckExecution(q))
        return result;

    bool hasData = q.next();
    //    if(!hasData)
    //        return result;
    if(q.value(0).toInt() != 0)
    {
        result.matchesWithReferenceTag = q.value(0).toInt();
    }
    result.matchesWithReferenceTag = q.value(0).toInt();
    if(result.matchesWithReferenceTag == 0)
        result.matchRatio = 999999;
    else
        result.matchRatio = (double)result.totalFics/(double)result.matchesWithReferenceTag;
    result.isValid = true;
    return result;
}


bool DeleteRecommendationList(QString listName)
{
    QSqlDatabase db = QSqlDatabase::database();
    auto listId= GetRecommendationListIdForName(listName);
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

bool CopyAllAuthorRecommendationsToList(int authorId, QString listName)
{
    QSqlDatabase db = QSqlDatabase::database();
    auto listId= GetRecommendationListIdForName(listName);

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

//!!! проверить
bool IncrementAllValuesInListMatchingAuthorFavourites(int authorId, QString listName)
{
    QSqlDatabase db = QSqlDatabase::database();
    auto listId= GetRecommendationListIdForName(listName);

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


void WriteRecommenderStatsForTag(core::AuthorRecommendationStats stats, int listId)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("insert into RecommendationListAuthorStats (author_id, fic_count, match_count, match_ratio, list_id) "
                         "values(:author_id, :fic_count, :match_count, :match_ratio, :list_id)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":author_id",stats.authorId);
    q.bindValue(":fic_count",stats.totalFics);
    q.bindValue(":match_count",stats.matchesWithReferenceTag);
    q.bindValue(":match_ratio",stats.matchRatio);
    //auto listId= GetRecommendationListIdForName(stats.listName);
    q.bindValue(":list_id",listId);
    ExecAndCheck(q);
}

QVector<int> GetAllFicIDsFromRecommendationList(QString listName)
{
    QVector<int> result;
    auto listId= GetRecommendationListIdForName(listName);

    QSqlDatabase db = QSqlDatabase::database();
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


void EnsureFFNUrlsShort()
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("update fanfics set url = cfReturnCapture('(/s/\\d+/)', url)");
    QSqlQuery q(db);
    q.prepare(qs);
    ExecAndCheck(q);
}

void PassTagsIntoTagsTable()
{
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QString qs = QString("select id, tags from fanfics where tags <> ' none ' order by tags");
    QSqlQuery q(db);
    q.prepare(qs);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
    }
    while(q.next())
    {
        auto id = q.value("id").toInt();
        auto tags = q.value("tags").toString();
        if(tags.trimmed() == "none")
            continue;
        QStringList split = tags.split(" ", QString::SkipEmptyParts);
        split.removeDuplicates();
        split.removeAll("none");
        qDebug() << split;
        for(auto tag : split)
        {
            qs = QString("insert into fictags(fic_id, tag) values(:fic_id, :tag)");
            QSqlQuery iq(db);
            iq.prepare(qs);
            iq.bindValue(":fic_id",id);
            iq.bindValue(":tag", tag);
            iq.exec();
            if(iq.lastError().isValid())
            {
                qDebug() << iq.lastError();
                qDebug() << iq.lastQuery();
            }
        }
    }
    db.commit();
}

int GetRecommendationListIdForName(QString name)
{
    int result = 0;
    QSqlDatabase db = QSqlDatabase::database();
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

bool CreateOrUpdateRecommendationList(core::RecommendationList& list)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("insert into RecommendationLists(name) select '%1' where not exists(select 1 from RecommendationLists where name = '%1')");
    qs= qs.arg(list.name);
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return false;

    qs = QString("update RecommendationLists set minimum = :minimum, pick_ratio = :pick_ratio, always_pick_at = :always_pick_at,  created = CURRENT_TIMESTAMP where name = :name");
    q.prepare(qs);
    q.bindValue(":minimum",list.minimumMatch);
    q.bindValue(":pick_ratio",list.pickRatio);
    q.bindValue(":always_pick_at",list.alwaysPickAt);
    q.bindValue(":name",list.name);
    if(!ExecAndCheck(q))
        return false;


    qs = QString("select id from RecommendationLists where name = :name");
    q.prepare(qs);
    q.bindValue(":name",list.name);
    if(!ExecAndCheck(q))
        return false;

    q.next();
    list.id = q.value(0).toInt();
    if(list.id > 0)
        return true;
    return false;
}

bool UpdateFicCountForRecommendationList(core::RecommendationList& list)
{
    if(list.id == -1)
        return false;

    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("update RecommendationLists set fic_count=(select count(fic_id) from RecommendationListData where list_id = :list_id) where list_id = :list_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id",list.id);
    if(!ExecAndCheck(q))
        return false;

    qs = QString("select fic_count from RecommendationLists where list_id = :list_id");
    q.prepare(qs);
    q.bindValue(":list_id",list.id);
    if(!ExecAndCheck(q))
        return false;

    q.next();
    list.id = q.value(0).toInt();
    if(list.id > 0)
        return true;
    return false;
}

void DeleteTagfromDatabase(QString tag)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("delete from FicTags where tag = :tag");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":tag",tag);
    if(!ExecAndCheck(q))
        return;
    return;
}





//will need to add genre tracker on ffn in case it'sever expanded
bool IsGenreList(QStringList list, QString website)
{
    // first we need to process hurt/comfort
    QSqlDatabase db = QSqlDatabase::database();
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

bool ReprocessFics(QString where, QString website, std::function<void(int)> f)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select id, %1_id from fanfics %2");
    qs = qs.arg(website).arg(where);
    QSqlQuery q(db);
    q.prepare(qs);
    if(!ExecAndCheck(q))
        return false;
    while(q.next())
    {
        auto id = q.value(1).toInt();
        f(id);
    }
    return true;
}

void TryDeactivate(QString url, QString website)
{
    auto id = url_utils::GetWebId(url, website);
    DeactivateStory(id.toInt(), website);
}

void DeactivateStory(int id, QString website)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("update fanfics set alive = 0 where %1_id = :id");
    qs=qs.arg(website);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id", id);
    ExecAndCheck(q);
}


QSqlDatabase FFN::GetDb() const
{
    return db;
}

void FFN::SetDb(const QSqlDatabase &value)
{
    db = value;
}

bool FFNFandoms::IsTracked(QString fandom)
{
    bool tracked = false;
    if(IsDataLoaded())
    {
        if(fandoms.contains(fandom))
            tracked = fandoms[fandom].tracked;
    }
    else
    {
        QSqlQuery q1(db);
        QString qsl = " select tracked from fandoms where id = :id ";
        auto id = GetID(fandom);
        q1.prepare(qsl);
        q1.bindValue(":id",id);
        ExecAndCheck(q1);
        q1.next();
        tracked = q1.value(0).toBool();
    }
    return tracked;
}

QList<int> FFNFandoms::AllTracked()
{
    QList<int> result;
    if(IsDataLoaded())
    {
        result.reserve(fandoms.size());
        for(auto &fandom: fandoms)
            if(fandom.tracked)
                result.push_back(fandom.id);
    }
    else
    {
        QSqlQuery q1(db);
        QString qsl = " select fandom from fandoms where tracked = 1";
        q1.prepare(qsl);
        q1.exec();
        QStringList result;
        while(q1.next())
            result.push_back(q1.value(0).toString());

        CheckExecution(q1);
        return result;
    }
}

QStringList FFNFandoms::AllTrackedStr()
{
    QStringList result;
    auto tracked = AllTracked();
    for(auto bit : tracked)
        result.push_back(QString::number(bit));
    return result;
}

int FFNFandoms::GetID(QString value)
{
    if(!indexFandomsByName.contains(value))
        return -1;
    return indexFandomsByName[value];
}

void FFNFandoms::SetTracked(QString fandom, bool value, bool immediate)
{
    if(!fandoms.contains(fandom) && !LoadFandom(fandom))
        return;

    if(immediate)
    {
        auto id = fandoms[fandom]->id;
        database::puresql::SetFandomTracked(id, value, db);
    }
    else
        fandoms[fandom]->hasChanges = fandoms[fandom]->tracked == false;

    fandoms[fandom]->tracked = value;
}

bool FFNFandoms::IsTracked(QString name)
{
    if(!fandoms.contains(name) || !fandoms[name])
        return false;
    return fandoms[name]->tracked;
}

QStringList FFNFandoms::ListOfTracked()
{
    QStringList names;
    for(auto fandom: fandoms)
        if(fandom && fandom->tracked)
            names.push_back(fandom->name);
    return names;
}

bool FFNFandoms::IsDataLoaded()
{
    return isLoaded;
}

bool FFNFandoms::Sync(bool forcedSync)
{
    for(auto fandom: fandoms)
    {
        if(fandom->hasChanges || forcedSync)
        {
            database::puresql::WriteMaxUpdateDateForFandom(fandom, db);
        }
    }
}

bool FFNFandoms::Load()
{
    Clear();
    auto recentFandoms = sqlite::FetchRecentFandoms(db);
    for(auto bit: recentFandoms)
    {
        bool fandomPresent = false;
        if(fandoms.contains(bit) || LoadFandom(bit))
           fandomPresent = true;

        if(fandomPresent)
            this->recentFandoms.push_back(fandoms[bit]);
    }
}

void FFNFandoms::Clear()
{
    this->recentFandoms.clear();
    this->fandoms.clear();
    this->indexFandomsByName.clear();
    this->updateQueue->clear();
}

QStringList FFNFandoms::PushFandomToTopOfRecent(QString fandom)
{
    QStringList result = GetRecentFandoms();
    if(!fandoms.contains(fandom))
        return result;
    // needs to be done on Sync ideally
    sqlite::PushFandomToTopOfRecent(fandom, db);
    sqlite::RebaseFandomsToZero(db);
    AddToTopOfRecent(fandom);
    result = GetRecentFandoms();
    return result;

}

QSharedPointer<core::Fandom> FFNFandoms::GetFandom(QString name)
{
    QSharedPointer<core::Fandom> result;
    if(!EnsureFandom(name))
        return result;

    return fandoms[name];
}

bool FFNFandoms::EnsureFandom(QString name)
{
    if(!fandoms.contains(fandom) && !LoadFandom(fandom))
        return false;
    return true;
}



QStringList FFNFandoms::GetRecentFandoms() const
{
    QStringList result;
    std::sort(std::begin(recentFandoms), std::end(recentFandoms), [](QSharedPointer<core::Fandom> f1, QSharedPointer<core::Fandom> f2){
        return f1->idInRecentFandoms < f1->idInRecentFandoms;
    });
    std::for_each(std::begin(recentFandoms), std::end(recentFandoms), [&result](QSharedPointer<core::Fandom> f){
        result.push_back(f->name);
    });
    return result;
}

void FFNFandoms::AddToTopOfRecent(QString fandom)
{
    if(!fandoms.contains(fandom))
        return;
    bool updatedThis = false;
    for(auto bit: recentFandoms)
    {
        if(bit->name != fandom)
            bit->idInRecentFandoms = bit->idInRecentFandoms+1;
        else
            bit->idInRecentFandoms = 0;
    }
    if(!updatedThis)
    {
        recentFandoms.push_back(fandoms[fandom]);
        fandoms[fandom]->idInRecentFandoms = 0;
    }
}

bool FFNFandoms::LoadFandom(QString name)
{
    QSharedPointer<core::Fandom> fandom(new core::Fandom);
    QSqlQuery q(db);
    q.prepare("select * from fandoms where fandom = :fandom_name and source = 'ffn'");
    if(!ExecAndCheck(q))
        return false;
    q.next();

    //crossoverurl is subject to weird changes so its better to load that on first fandom read each time
    fandom->name = name;
    fandom->url = q.value("NORMAL_URL").toString();
    fandom->crossoverUrl = q.value("CROSSOVER_URL").toString() + "/0/"; //!todo fix it
    fandom->dateOfCreation = q.value("date_of_first_fic").toString();
    fandom->dateOfLastFic= q.value("date_of_last_fic").toString();
    fandom->lastUpdateDate = q.value("last_update").toString();
    fandom->lastCrossoverUpdateDate = q.value("last_update_crossover").toString();
    fandom->id = q.value("id").toInt();
    fandom->section = q.value("section").toString();
    fandom->tracked = q.value("tracked").toBool();
    fandom->ficCount = q.value("fic_count").toInt();
    fandom->averageFavesTop3 = q.value("average_faves_top_3").toInt();
    fandoms[name] = fandom;
    return true;
}

bool FFNFandoms::AssignTagToFandom(QString fandom, QString tag)
{
    if(!EnsureFandom(name))
        return false;
    auto id = fandoms[fandom]->id;
    database::puresql::AssignTagToFandom(tag, id, db);
}

bool FFNFandoms::CreateFandom(QSharedPointer<core::Fandom> fandom)
{
    if(fandoms.contains(fandom->name) && fandoms[fandom->name]->id != -1)
        return true;

    auto result = database::puresql::CreateFandomInDatabase(fandom, db);

    if(!result)
        return false;
    fandom->id = database::sqlite::GetLastIdForTable("fandoms", db);
    EnsureFandom(fandom->name);
    return true;
}

QString FFNFandoms::GetCurrentCrossoverUrl()
{
    //will need to parse the page for that
}

void FFNFandoms::CalculateFandomAverages()
{
    database::puresql::CalculateFandomAverages(db);
}

void FFNFandoms::CalculateFandomFicCounts()
{
    database::puresql::CalculateFandomFicCounts(db);
}

void FFNFanfics::ProcessIntoDataQueues(QList<QSharedPointer<core::Fic>> fics, bool alwaysUpdateIfNotInsert)
{
    for(QSharedPointer<core::Fic> fic: fics)
    {
        database::puresql::SetUpdateOrInsert(fic, db, alwaysUpdateIfNotInsert);
        {
            QWriteLocker lock(&mutex);
            if(fic->updateMode == core::UpdateMode::update && !updateQueue.contains(fic->id))
                updateQueue[id] = fic;
            if(fic->updateMode == core::UpdateMode::insert && !insertQueue.contains(fic->id))
                insertQueue[id] = fic;
        }
    }

}

void FFNFanfics::AddRecommendations(QList<core::FicRecommendation> recommendations)
{
    QWriteLocker lock(&mutex);
    ficRecommendations.reserve(ficRecommendations.size() + recommendations.size());
    ficRecommendations += recommendations;
}

void FFNFanfics::FlushDataQueues()
{
    db.transaction();
    for(auto fic: insertQueue)
        database::InsertIntoDB(fic, db);

    for(auto fic: insertQueue)
        database::UpdateInDB(fic, db);

    for(auto recommendation: ficRecommendations)
    {
        if(!recommendation.IsValid() || !authorInterface->EnsureId(recommendation.author))
            continue;
        auto id = GetIDFromWebID(recommendation.fic->webId);
        database::WriteRecommendation(recommendation.author, id, db);
    }

    db.commit();
}

bool FFNAuthors::EnsureId(QSharedPointer<core::Author> author)
{
    if(!author)
        return false;

    if(author->GetIdStatus() == core::AuthorIdStatus::unassigned)
        author->AssignId(database::puresql::GetAuthorIdFromUrl(author->url("ffn"), db));
    if(author->GetIdStatus() == core::AuthorIdStatus::not_found)
    {
        database::sqlite::WriteAuthor(author, db);
        author->AssignId(database::puresql::GetAuthorIdFromUrl(author->url("ffn"), db));
    }
    if(author->id < 0)
        return false;
    return true;
}

bool FFNAuthors::LoadAuthors(QString website, bool additionMode)
{
    if(!additionMode)
        Clear();
    authors = database::puresql::GetAllAuthors(website, db);
    Reindex();

}

bool FFNRecommendationLists::Load()
{

}

void FFNRecommendationLists::Clear()
{
    ClearIndex();
    lists.clear();
}

void FFNRecommendationLists::LoadAvailableRecommendationLists()
{
    lists = database::puresql::GetAvailableRecommendationLists(db);
    Reindex();
}



}
