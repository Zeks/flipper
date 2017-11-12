#include "Interfaces/fanfics.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/authors.h"
#include "include/pure_sql.h"
#include "include/transaction.h"
#include <QVector>

namespace interfaces {


Fanfics::~Fanfics()
{

}

void Fanfics::ClearIndex()
{
    //idOnly.Clear();
    idIndex.clear();
    webIdIndex.clear();
}

void Fanfics::ClearIndexWithIdIndex()
{
    idOnly.Clear();
    ClearIndex();
}

void Fanfics::Reindex()
{
    ClearIndex();
    for(auto fic : fics)
        AddFicToIndex(fic);
}

void Fanfics::AddFicToIndex(core::FicPtr fic)
{
    if(!fic)
        return;

    EnsureId(fic->webSite, fic->webId);
    idIndex.insert(fic->id, fic);
    webIdIndex[fic->webSite][fic->webId] = fic;
}

int Fanfics::EnsureId(core::FicPtr fic)
{
    auto result = idOnly.GetIdByWebId(fic->webSite,fic->webId);
    if(result.exists && result.valid)
        return result.id;

    database::Transaction transaction(db);
    if(!LoadFicToDB(fic))
        return result.id;

    result.id = GetIdFromDatabase(fic->webSite, fic->webId);
    transaction.finalize();
    fic->id = result.id;
    idOnly.Add(fic->webSite, fic->webId, result.id);

    return result.id;
}

void Fanfics::EnsureId(QString website, int webId)
{
    auto result = idOnly.GetIdByWebId(website,webId);
    if(result.exists && result.valid)
        return;

    result.id = GetIdFromDatabase(website, webId);

    idOnly.Add(website, webId, result.id);
    return;
}

bool Fanfics::EnsureFicLoaded(int id, QString website)
{
    bool result = false;
    if(webIdIndex.contains(website) && webIdIndex[website].contains(id))
        return true;

    if(LoadFicFromDB(id,website))
        result = true;

    return result;
}

bool Fanfics::LoadFicFromDB(int id, QString website)
{
    auto fic = database::puresql::GetFicByWebId(website, id, db);

    if(!fic)
        return false;

    AddFicToIndex(fic);
    return true;
}

bool Fanfics::LoadFicToDB(core::FicPtr fic)
{
    if(!fic)
        return false;

    database::Transaction transaction(db);
    bool insertResult = database::puresql::InsertIntoDB(fic, db);
    if(!insertResult)
        return false;
    int idResult = GetIdFromDatabase(fic->webSite, fic->webId);
    if(idResult == -1)
        return false;

    transaction.finalize();
    fic->id = idResult;
    AddFicToIndex(fic);
    return true;
}


int Fanfics::GetIDFromWebID(int id, QString website)
{
    EnsureId(website, id);
    auto result = idOnly.GetIdByWebId(website, id);
    return result.id;
}

int Fanfics::GetWebIDFromID(int id, QString website)
{
    EnsureId(website, id);
    auto result = idOnly.GetWebIdById(website, id);
    return result.id;
}

bool Fanfics::ReprocessFics(QString where, QString website, std::function<void (int)> f)
{
    auto list = database::puresql::GetWebIdList(where, website, db);
    if(list.empty())
        return false;
    for(auto id : list)
    {
        f(id);
    }
    return true;
}

bool Fanfics::IsEmptyQueues()
{
    return updateQueue.empty() && insertQueue.empty();
}

bool Fanfics::DeactivateFic(int ficId, QString website)
{
    return database::puresql::DeactivateStory(ficId, website, db);
}

bool Fanfics::AssignChapter(int ficId, int chapter)
{
    return database::puresql::AssignChapterToFanfic(chapter, ficId, db);
}

void Fanfics::AddRecommendations(QList<core::FicRecommendation> recommendations)
{
    QWriteLocker lock(&mutex);
    ficRecommendations.reserve(ficRecommendations.size() + recommendations.size());
    ficRecommendations += recommendations;
}

void Fanfics::CalcStatsForFics(QList<QSharedPointer<core::Fic>> fics)
{
    for(QSharedPointer<core::Fic> fic: fics)
    {
        if(!fic)
            continue;

        fic->calcStats.wcr = 200000; // default
        if(fic->wordCount.toInt() > 1000 && fic->reviews > 0)
            fic->calcStats.wcr = fic->wordCount.toDouble()/fic->reviews.toDouble();
        fic->calcStats.reviewsTofavourites = 0;
        if(fic->favourites.toInt())
            fic->calcStats.reviewsTofavourites = fic->reviews.toDouble()/fic->favourites.toDouble();
        fic->calcStats.age = std::abs(QDateTime::currentDateTimeUtc().daysTo(fic->published));
        fic->calcStats.daysRunning = std::abs(fic->updated.daysTo(fic->published));
    }
}

void Fanfics::ProcessIntoDataQueues(QList<QSharedPointer<core::Fic>> fics, bool alwaysUpdateIfNotInsert)
{
    CalcStatsForFics(fics);
    for(QSharedPointer<core::Fic> fic: fics)
    {
        if(!fic)
            continue;
        auto id = fic->webId;
        database::puresql::SetUpdateOrInsert(fic, db, alwaysUpdateIfNotInsert);
        {
            QWriteLocker lock(&mutex);
            if(fic->updateMode == core::UpdateMode::update && !updateQueue.contains(id))
                updateQueue[id] = fic;
            if(fic->updateMode == core::UpdateMode::insert && !insertQueue.contains(id))
                insertQueue[id] = fic;
        }
    }
}

bool Fanfics::FlushDataQueues()
{
    database::Transaction transaction(db);
    for(auto fic: insertQueue)
    {
        database::puresql::InsertIntoDB(fic, db);
        for(auto fandom: fic->fandoms)
            database::puresql::AddFandomForFic(fic->id, fandomInterface->GetIDForName(fandom), db);
    }

    for(auto fic: updateQueue)
        database::puresql::UpdateInDB(fic, db);

    for(auto recommendation: ficRecommendations)
    {
        if(!recommendation.IsValid() || !authorInterface->EnsureId(recommendation.author))
            continue;
        auto id = GetIDFromWebID(recommendation.fic->webId, recommendation.fic->webSite);
        database::puresql::WriteRecommendation(recommendation.author, id, db);
    }

    if(!transaction.finalize())
        return false;
    insertQueue.clear();
    updateQueue.clear();
    return true;
}

int Fanfics::GetIdFromDatabase(QString website, int id)
{
    return database::puresql::GetFicIdByWebId(website, id, db);
}


void Fanfics::FicIds::Clear()
{
    idIndex.clear();
    webIdIndex.clear();
}

Fanfics::IdResult Fanfics::FicIds::GetIdByWebId(QString website, int webId)
{
    Fanfics::IdResult result;
    if(webIdIndex.contains(website) && webIdIndex[website].contains(webId))
    {
        result.exists = true;
        result.id = webIdIndex[website][webId];
        result.valid = result.id != -1;
    }
    return result;
}

Fanfics::IdResult Fanfics::FicIds::GetWebIdById(QString website, int id)
{
    Fanfics::IdResult result;
    if(idIndex.contains(id) && idIndex[id].contains(website))
    {
        result.exists = true;
        result.id = idIndex[id][website];
        result.valid = result.id != -1;
    }
    return result;
}

void Fanfics::FicIds::Add(QString website, int webId, int id)
{
    idIndex[id][website] = webId;
    webIdIndex[website][webId] = id;
}

}
