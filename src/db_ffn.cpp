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
