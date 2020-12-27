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
#include "Interfaces/fanfics.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/authors.h"
#include "include/pure_sql.h"
#include "include/transaction.h"
#include "include/in_tag_accessor.h"
#include <QVector>
#include <QDebug>

namespace interfaces {

void Fanfics::ClearIndex()
{
    //idOnly.Clear();
    fanficIndex.clear();
}

void Fanfics::ClearIndexWithIdIndex()
{
    idToWebsiteMappings.Clear();
    ClearIndex();
}

void Fanfics::AddFicToIndex(core::FicPtr fic)
{
    if(!fic)
        return;

    LoadFicIntoIdHash(fic);
    fanficIndex.insert(fic->identity.id, fic);
}

int Fanfics::LoadFicIntoIdHash(core::FicPtr fic)
{
    auto webIdentity = fic->identity.web.GetPrimaryIdentity();
    auto result = idToWebsiteMappings.GetDBIdByWebId(webIdentity);
    if(result.exists && result.valid)
        return result.id;

    database::Transaction transaction(db);
    if(!LoadFicToDB(fic))
        return result.id;

    result.id = GetIdFromDatabase(webIdentity);
    transaction.finalize();
    fic->identity.id = result.id;
    idToWebsiteMappings.Add(webIdentity.website, webIdentity.identity, result.id);
    return result.id;
}

void Fanfics::LoadFicIntoIdHash(QString website, int webId)
{
    auto result = idToWebsiteMappings.GetDBIdByWebId(website,webId);
    if(result.exists && result.valid)
        return;

    result.id = GetIdFromDatabase(website, webId);

    idToWebsiteMappings.Add(website, webId, result.id);
    return;
}

bool Fanfics::EnsureFicLoaded(int id, QString website)
{
    bool result = false;
    if(idToWebsiteMappings.GetDBIdByWebId({website, id}).valid)
        return true;

    if(LoadFicFromDB(id,website))
        result = true;

    return result;
}

bool Fanfics::LoadFicFromDB(int id, QString website)
{
    auto result = sql::GetFicByWebId(website, id, db);
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
    bool insertResult = sql::InsertIntoDB(fic, db).success;
    if(!insertResult)
        return false;
    auto webIdentity = fic->identity.web.GetPrimaryIdentity();
    int idResult = GetIdFromDatabase(webIdentity);
    if(idResult == -1)
        return false;

    transaction.finalize();
    fic->identity.id = idResult;
    AddFicToIndex(fic);
    return true;
}

core::FicPtr Fanfics::GetFicById(int id)
{
    if(fanficIndex.contains(id))
        return fanficIndex[id];

    auto result = sql::GetFicById(id, db);
    auto fic= result.data;
    if(!result.success || !fic)
        return fic;

    AddFicToIndex(fic);
    return fic;
}


int Fanfics::GetIDFromWebID(int id, QString website)
{
    LoadFicIntoIdHash(website, id);
    auto result = idToWebsiteMappings.GetDBIdByWebId(website, id);
    return result.id;
}

int Fanfics::GetWebIDFromID(int id, QString website)
{
    LoadFicIntoIdHash(website, id);
    auto result = idToWebsiteMappings.GetWebIdByDBId(website, id);
    return result.id;
}

bool Fanfics::ReprocessFics(QString where, QString website, bool useDirectIds, const std::function<void (int)>& f)
{
    QVector<int> list;
    if(useDirectIds)
        list = sql::GetIdList(where, db).data;
    else
        list = sql::GetWebIdList(where, website, db).data;
    if(list.empty())
        return false;
    for(auto id : std::as_const(list))
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
    return sql::DeactivateStory(ficId, website, db).success;
}

void Fanfics::ClearProcessedHash()
{
    processedHash.clear();
}

QStringList Fanfics::GetFandomsForFicAsNames(int ficId)
{
    return sql::GetFandomNamesForFicId(ficId, db).data;
}

QSet<int> Fanfics::GetAllKnownSlashFics()
{
    return sql::GetAllKnownSlashFics(db).data;
}

QSet<int> Fanfics::GetAllKnownNotSlashFics()
{
    return sql::GetAllKnownNotSlashFics(db).data;
}

QSet<int> Fanfics::GetSingularFicsInLargeButSlashyLists()
{
    return sql::GetSingularFicsInLargeButSlashyLists(db).data;
}

QSet<int> Fanfics::GetAllKnownFicIDs(QString where)
{
    auto data = sql::GetAllKnownFicIds(where, db);
    return data.data;
}

QSet<int> Fanfics::GetFicIDsWithUnsetAuthors()
{
    return sql::GetFicIDsWithUnsetAuthors(db).data;
}

QHash<int, core::FanficCompletionStatus> Fanfics::GetSnoozeInfo()
{
    return sql::GetSnoozeInfo(db).data;
}

QHash<int, core::FanficSnoozeStatus> Fanfics::GetUserSnoozeInfo(bool fetchExpired, bool useLimitedSelection)
{
    return sql::GetUserSnoozeInfo(fetchExpired, useLimitedSelection, db).data;
}

bool Fanfics::WriteExpiredSnoozes(QSet<int> data)
{
    return sql::WriteExpiredSnoozes(data, db).success;
}

bool Fanfics::SnoozeFic(core::FanficSnoozeStatus data)
{
    return sql::SnoozeFic(data, db).success;
}

bool Fanfics::RemoveSnooze(int ficId)
{
    return sql::RemoveSnooze(ficId, db).success;
}

void UploadFicIdsForSelection(QVector<core::Fanfic> * fics){
    auto* userThreadData = ThreadData::GetUserData();
    userThreadData->ficsForSelection.clear();
    for(auto& fic : *fics)
        userThreadData->ficsForSelection.insert(fic.identity.id);
}


bool Fanfics::FetchSnoozesForFics(QVector<core::Fanfic> * fics)
{
    if(!fics)
        return false;

    UploadFicIdsForSelection(fics);

    auto snoozes = GetUserSnoozeInfo(true, true);
    for(auto& fic: *fics)
    {
        if(snoozes.contains(fic.identity.id))
        {
            auto& snooze = snoozes[fic.identity.id];
            fic.userData.chapterTillSnoozed = snooze.snoozedTillChapter;
            fic.userData.chapterSnoozed = snooze .snoozedAtChapter;
            if(snooze.untilFinished)
            {
                fic.userData.snoozeMode = core::Fanfic::EFicSnoozeMode::efsm_til_finished;
            }
            else if((snooze.snoozedTillChapter - snooze.snoozedAtChapter) == 1)
            {
                fic.userData.snoozeMode = core::Fanfic::EFicSnoozeMode::efsm_next_chapter;
            }
            else
                fic.userData.snoozeMode = core::Fanfic::EFicSnoozeMode::efsm_target_chapter;
        }
    }
    return true;
}

bool Fanfics::FetchNotesForFics(QVector<core::Fanfic> * fics)
{
    if(!fics)
        return false;

    UploadFicIdsForSelection(fics);
    auto notes = sql::GetNotesForFics(true, db).data;

    for(auto& fic: *fics)
    {
        if(notes.contains(fic.identity.id))
        {
            fic.notes = notes[fic.identity.id];
        }
    }

    return true;
}

bool Fanfics::FetchChaptersForFics(QVector<core::Fanfic> * fics)
{
    if(!fics)
        return false;

    UploadFicIdsForSelection(fics);
    auto chapters = sql::GetReadingChaptersForFics(true, db).data;

    for(auto& fic: *fics)
    {
        if(chapters.contains(fic.identity.id))
        {
            fic.userData.atChapter = chapters[fic.identity.id];
        }
        else
            fic.userData.atChapter = 0;
    }

    return true;
}

bool Fanfics::AddNoteToFic(int ficId, QString note)
{
    return sql::AddNoteToFic(ficId,note, db).success;
}

bool Fanfics::RemoveNoteFromFic(int ficId)
{
    return sql::RemoveNoteFromFic(ficId, db).success;
}

QVector<core::FicWeightPtr> Fanfics::GetAllFicsWithEnoughFavesForWeights(int faves)
{
    return sql::GetAllFicsWithEnoughFavesForWeights(faves, db).data;
}

QHash<int, core::FicWeightPtr> Fanfics::GetHashOfAllFicsWithEnoughFavesForWeights(int faves)
{
    QHash<int, core::FicWeightPtr> result;
    auto temp = GetAllFicsWithEnoughFavesForWeights(faves);
    for(const auto& fic : temp)
        result[fic->id] = fic;
    return result;
}

bool Fanfics::ProcessSlashFicsBasedOnWords( std::function<SlashPresence (QString, QString, QString)> func)
{
     auto result = sql::ProcessSlashFicsBasedOnWords(func, db);
     return result.success;
}

bool Fanfics::AssignChapter(int ficId, int chapter)
{
    return sql::AssignChapterToFanfic(ficId,chapter, db).success;
}

bool Fanfics::AssignScore(int score, int ficId)
{
    return sql::AssignScoreToFanfic(score, ficId, db).success;
}

bool Fanfics::AssignSlashForFic(int ficId, int source)
{
    return sql::AssignSlashToFanfic(ficId, source, db).success;
}

bool Fanfics::AssignQueuedForFic(int ficId)
{
    return sql::AssignQueuedToFanfic(ficId, db).success;
}

bool Fanfics::AssignIterationOfSlash(QString iteration)
{
    return sql::AssignIterationOfSlash(iteration, db).data;
}

bool Fanfics::PerformGenreAssignment()
{
    return sql::PerformGenreAssignment(db).success;
}

QHash<int, double> Fanfics::GetFicGenreData(QString genre, QString cutoff)
{
    return sql::GetFicGenreData(genre, cutoff, db).data;
}

QHash<int, std::array<double, 22> > Fanfics::GetFullFicGenreData()
{
    return sql::GetFullFicGenreData(db).data;
}

QHash<int, double> Fanfics::GetDoubleValueHashForFics(QString fieldName)
{
    return sql::GetDoubleValueHashForFics(fieldName, db).data;
}

QHash<int, QString> Fanfics::GetGenreForFics()
{
    return sql::GetGenreForFics(db).data;
}

QHash<int, int> Fanfics::GetScoresForFics()
{
    return sql::GetScoresForFics(db).data;
}

QSet<int> Fanfics::ConvertFFNSourceFicsToDB(QString userToken)
{
    return sql::ConvertFFNSourceFicsToDB(userToken, db).data;
}

QHash<uint32_t, core::FicWeightPtr> Fanfics::GetFicsForRecCreation()
{
    return sql::GetFicsForRecCreation(db).data;
}

bool Fanfics::ConvertFFNTaggedFicsToDB(QHash<int, int>& hash)
{
    return sql::ConvertFFNTaggedFicsToDB(hash, db).success;
}

bool Fanfics::ConvertDBFicsToFFN(QHash<int, int> &hash)
{
    return sql::ConvertDBFicsToFFN(hash, db).success;
}

void Fanfics::ResetActionQueue()
{
    sql::ResetActionQueue(db);
}

void Fanfics::AddRecommendations(QList<core::FicRecommendation> recommendations)
{
    QWriteLocker lock(&mutex);
    ficRecommendations.reserve(ficRecommendations.size() + recommendations.size());
    ficRecommendations += recommendations;
}

void Fanfics::CalcStatsForFics(QList<QSharedPointer<core::Fanfic>> fics)
{
    for(const auto& fic: std::as_const(fics))
    {
        if(!fic)
            continue;

        fic->statistics.wcr = 200000; // default
        if(fic->wordCount.toInt() > 1000 && fic->reviews > 0)
            fic->statistics.wcr = fic->wordCount.toDouble()/fic->reviews.toDouble();
        fic->statistics.reviewsTofavourites = 0;
        if(fic->favourites.toInt())
            fic->statistics.reviewsTofavourites = fic->reviews.toDouble()/fic->favourites.toDouble();
        fic->statistics.age = std::abs(QDateTime::currentDateTimeUtc().daysTo(fic->published));
        fic->statistics.daysRunning = std::abs(fic->updated.daysTo(fic->published));
    }
}

bool Fanfics::WriteRecommendations()
{
    database::Transaction transaction(db);
    for(auto recommendation: std::as_const(ficRecommendations))
    {
        if(!recommendation.IsValid() || !authorInterface->EnsureId(recommendation.author))
            continue;
        auto webIdentity = recommendation.fic->identity.web.GetPrimaryIdentity();
        auto id = GetIDFromWebID(webIdentity.identity, webIdentity.website);
        sql::WriteRecommendation(recommendation.author, id, db);
    }

    if(!transaction.finalize())
        return false;
    ficRecommendations.clear();
    return true;
}

bool Fanfics::WriteFicRelations(QList<core::FicWeightResult> result)
{
    return sql::WriteFicRelations(result, db).success;
}

bool Fanfics::WriteAuthorsForFics(QHash<uint32_t, uint32_t> data)
{
    return sql::WriteAuthorsForFics(data, db).success;
}


void Fanfics::ProcessIntoDataQueues(QList<QSharedPointer<core::Fanfic>> fics, bool alwaysUpdateIfNotInsert)
{
    CalcStatsForFics(fics);
    skippedCounter = 0;
    updatedCounter = 0;
    insertedCounter = 0;
    for(const auto& fic: std::as_const(fics))
    {
        if(!fic)
            continue;
        auto id = fic->identity.web.GetPrimaryId();

        if(!processedHash.contains(fic->identity.web.GetPrimaryId()))
        {
            sql::SetUpdateOrInsert(fic, db, alwaysUpdateIfNotInsert);
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
        processedHash.insert(fic->identity.web.GetPrimaryId());
    }
}

bool Fanfics::FlushDataQueues()
{
    database::Transaction transaction(db);
    int insertCounter = 0;
    int updateCounter = 0;
    bool hasFailures = false;
    for(const auto& fic: std::as_const(insertQueue))
    {
        insertCounter++;
        bool writeResult = sql::InsertIntoDB(fic, db).success;
        hasFailures = hasFailures && writeResult;
        fic->identity.id = GetIDFromWebID(fic->identity.web.ffn, "ffn");
        for(const auto& fandom: std::as_const(fic->fandoms))
        {
            bool result = sql::AddFandomForFic(fic->identity.id, fandomInterface->GetIDForName(fandom), db).success;
            hasFailures = hasFailures && result;
            if(!result)
            {
                qDebug() << "failed to write fandom for: " << fic->identity.web.ffn;
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

    for(const auto& fic: std::as_const(updateQueue))
    {
        fic->identity.id = GetIDFromWebID(fic->identity.web.ffn, "ffn");
        auto result = sql::UpdateInDB(fic, db).success;
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
    return sql::GetFicIdByWebId(website, id, db).data;
}

int Fanfics::GetIdFromDatabase(core::SiteId siteId)
{
    if(!siteId.website.isEmpty())
        return GetIdFromDatabase(siteId.website, siteId.identity);
    return -1;
}


void Fanfics::FicIdToWebsiteMapping::Clear()
{
    idIndex.clear();
    webIdIndex.clear();
}

Fanfics::IdResult Fanfics::FicIdToWebsiteMapping::GetDBIdByWebId(core::SiteId siteId)
{
    if(!siteId.website.isEmpty())
        return GetDBIdByWebId(siteId.website, siteId.identity);
    return {};
}

Fanfics::IdResult Fanfics::FicIdToWebsiteMapping::GetDBIdByWebId(QString website, int webId)
{
    Fanfics::IdResult result;
    auto itOuter = webIdIndex.constFind(website);
    if(itOuter == webIdIndex.cend())
        return result;
    auto itInner = (*itOuter).constFind(webId);
    if(itInner == (*itOuter).cend())
        return result;

    result.exists = true;
    result.id = *itInner;
    result.valid = result.id != -1;

    return result;
}

Fanfics::IdResult Fanfics::FicIdToWebsiteMapping::GetWebIdByDBId(QString website, int id)
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

void Fanfics::FicIdToWebsiteMapping::Add(QString website, int webId, int id)
{
    idIndex[id][website] = webId;
    webIdIndex[website][webId] = id;
}

void Fanfics::FicIdToWebsiteMapping::ProcessIdentityIntoMappings(core::Identity identity)
{
    if(identity.web.ffn != -1)
        webIdIndex["ffn"][identity.web.ffn] = identity.id;
    if(identity.web.ao3 != -1)
        webIdIndex["ao3"][identity.web.ao3] = identity.id;
    if(identity.web.sb != -1)
        webIdIndex["sb"][identity.web.sb] = identity.id;
    if(identity.web.sv != -1)
        webIdIndex["sv"][identity.web.sv] = identity.id;
}

}
