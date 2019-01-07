/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2018  Marchenko Nikolai

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
#include "Interfaces/fanfics.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/authors.h"
#include "include/pure_sql.h"
#include "include/transaction.h"
#include <QVector>
#include <QDebug>

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
    auto result = database::puresql::GetFicByWebId(website, id, db);
    auto fic  = result.data;
    if(!result.success || !fic)
        return false;


    AddFicToIndex(fic);
    return true;
}

bool Fanfics::LoadFicToDB(core::FicPtr fic)
{
    if(!fic)
        return false;

    database::Transaction transaction(db);
    bool insertResult = database::puresql::InsertIntoDB(fic, db).success;
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

core::FicPtr Fanfics::GetFicById(int id)
{
    if(idIndex.contains(id))
        return idIndex[id];

    auto result = database::puresql::GetFicById(id, db);
    auto fic= result.data;
    if(!result.success || !fic)
        return fic;

    AddFicToIndex(fic);
    return fic;
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

bool Fanfics::ReprocessFics(QString where, QString website, bool useDirectIds, std::function<void (int)> f)
{
    QVector<int> list;
    if(useDirectIds)
        list = database::puresql::GetIdList(where, db).data;
    else
        list = database::puresql::GetWebIdList(where, website, db).data;
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
    return database::puresql::DeactivateStory(ficId, website, db).success;
}

void Fanfics::ClearProcessedHash()
{
    processedHash.clear();
}

QStringList Fanfics::GetFandomsForFicAsNames(int ficId)
{
    return database::puresql::GetFandomNamesForFicId(ficId, db).data;
}

QSet<int> Fanfics::GetAllKnownSlashFics()
{
    return database::puresql::GetAllKnownSlashFics(db).data;
}

QSet<int> Fanfics::GetAllKnownNotSlashFics()
{
    return database::puresql::GetAllKnownNotSlashFics(db).data;
}

QSet<int> Fanfics::GetSingularFicsInLargeButSlashyLists()
{
    return database::puresql::GetSingularFicsInLargeButSlashyLists(db).data;
}

QSet<int> Fanfics::GetAllKnownFicIDs(QString where)
{
    auto data = database::puresql::GetAllKnownFicIds(where, db);
    return data.data;
}

QSet<int> Fanfics::GetFicIDsWithUnsetAuthors()
{
    return database::puresql::GetFicIDsWithUnsetAuthors(db).data;
}

QVector<core::FicWeightPtr> Fanfics::GetAllFicsWithEnoughFavesForWeights(int faves)
{
    return database::puresql::GetAllFicsWithEnoughFavesForWeights(faves, db).data;
}

bool Fanfics::ProcessSlashFicsBasedOnWords( std::function<SlashPresence (QString, QString, QString)> func)
{
     auto result = database::puresql::ProcessSlashFicsBasedOnWords(func, db);
     return result.success;
}

bool Fanfics::AssignChapter(int ficId, int chapter)
{
    return database::puresql::AssignChapterToFanfic(chapter, ficId, db).success;
}

bool Fanfics::AssignSlashForFic(int ficId, int source)
{
    return database::puresql::AssignSlashToFanfic(ficId, source, db).success;
}

bool Fanfics::AssignQueuedForFic(int ficId)
{
    return database::puresql::AssignQueuedToFanfic(ficId, db).success;
}

bool Fanfics::AssignIterationOfSlash(QString iteration)
{
    return database::puresql::AssignIterationOfSlash(iteration, db).data;
}

bool Fanfics::PerformGenreAssignment()
{
    return database::puresql::PerformGenreAssignment(db).success;
}

QHash<int, double> Fanfics::GetFicGenreData(QString genre, QString cutoff)
{
    return database::puresql::GetFicGenreData(genre, cutoff, db).data;
}

QHash<int, std::array<double, 22> > Fanfics::GetFullFicGenreData()
{
    return database::puresql::GetFullFicGenreData(db).data;
}

QHash<int, double> Fanfics::GetDoubleValueHashForFics(QString fieldName)
{
    return database::puresql::GetDoubleValueHashForFics(fieldName, db).data;
}

QHash<int, QString> Fanfics::GetGenreForFics()
{
    return database::puresql::GetGenreForFics(db).data;
}

QSet<int> Fanfics::ConvertFFNSourceFicsToDB(QString userToken)
{
    return database::puresql::ConvertFFNSourceFicsToDB(userToken, db).data;
}

QHash<uint32_t, core::FicWeightPtr> Fanfics::GetFicsForRecCreation()
{
    return database::puresql::GetFicsForRecCreation(db).data;
}

bool Fanfics::ConvertFFNTaggedFicsToDB(QHash<int, int>& hash)
{
    return database::puresql::ConvertFFNTaggedFicsToDB(hash, db).success;
}

bool Fanfics::ConvertDBFicsToFFN(QHash<int, int> &hash)
{
    return database::puresql::ConvertDBFicsToFFN(hash, db).success;
}

void Fanfics::ResetActionQueue()
{
    database::puresql::ResetActionQueue(db);
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

bool Fanfics::WriteRecommendations()
{
    database::Transaction transaction(db);
    for(auto recommendation: ficRecommendations)
    {
        if(!recommendation.IsValid() || !authorInterface->EnsureId(recommendation.author))
            continue;
        auto id = GetIDFromWebID(recommendation.fic->webId, recommendation.fic->webSite);
        database::puresql::WriteRecommendation(recommendation.author, id, db);
    }

    if(!transaction.finalize())
        return false;
    ficRecommendations.clear();
    return true;
}

bool Fanfics::WriteFicRelations(QList<core::FicWeightResult> result)
{
    return database::puresql::WriteFicRelations(result, db).success;
}

bool Fanfics::WriteAuthorsForFics(QHash<uint32_t, uint32_t> data)
{
    return database::puresql::WriteAuthorsForFics(data, db).success;
}


void Fanfics::ProcessIntoDataQueues(QList<QSharedPointer<core::Fic>> fics, bool alwaysUpdateIfNotInsert)
{
    CalcStatsForFics(fics);
    skippedCounter = 0;
    updatedCounter = 0;
    insertedCounter = 0;
    for(QSharedPointer<core::Fic> fic: fics)
    {
        if(!fic)
            continue;
        auto id = fic->webId;

        if(!processedHash.contains(fic->webId))
        {
            database::puresql::SetUpdateOrInsert(fic, db, alwaysUpdateIfNotInsert);
            {
                QWriteLocker lock(&mutex);
                bool insert = false;
                   Q_UNUSED(insert)
                if(fic->updateMode == core::UpdateMode::update && !updateQueue.contains(id))
                {
                    updateQueue[id] = fic;
                    updatedCounter++;
                }
                if(fic->updateMode == core::UpdateMode::insert && !insertQueue.contains(id))
                {
                    insertQueue[id] = fic;
                    insert = true;
                    insertedCounter++;
                }
//                if(!insert)
//                    updateQueue[id] = fic;
            }
        }
        else
            skippedCounter++;
        processedHash.insert(fic->webId);
    }
}

bool Fanfics::FlushDataQueues()
{
    database::Transaction transaction(db);
    int insertCounter = 0;
    int updateCounter = 0;
    bool hasFailures = false;
    for(auto fic: insertQueue)
    {
        insertCounter++;
        bool writeResult = database::puresql::InsertIntoDB(fic, db).success;
        hasFailures = hasFailures && writeResult;
        fic->id = GetIDFromWebID(fic->webId, "ffn");
        for(auto fandom: fic->fandoms)
        {
            bool result = database::puresql::AddFandomForFic(fic->id, fandomInterface->GetIDForName(fandom), db).success;
            hasFailures = hasFailures && result;
            if(!result)
            {
                qDebug() << "failed to write fandom for: " << fic->webId;
                fandomInterface->GetIDForName(fandom);
            }
            if(hasFailures)
                break;
        }
        if(hasFailures)
            break;
    }
    if(hasFailures)
        return false;

    for(auto fic: updateQueue)
    {
        fic->id = GetIDFromWebID(fic->webId, "ffn");
        auto result = database::puresql::UpdateInDB(fic, db).success;
        hasFailures = hasFailures && result;
        updateCounter++;
        if(hasFailures)
            break;
    }
    if(hasFailures)
        return false;

    WriteRecommendations();
    if(insertCounter > 0)
        qDebug() << "inserted: " << insertCounter;
    if(updateCounter > 0)
        qDebug() << "updated: " << updateCounter;
    if(!transaction.finalize())
        return false;
    insertQueue.clear();
    updateQueue.clear();
    return true;
}

int Fanfics::GetIdFromDatabase(QString website, int id)
{
    return database::puresql::GetFicIdByWebId(website, id, db).data;
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
