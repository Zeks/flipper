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
#pragma once
#include "Interfaces/base.h"
#include "core/section.h"
#include "core/experimental/fic_relations.h"
#include "regex_utils.h"
#include "QScopedPointer"
#include <QSet>
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"
#include <functional>


namespace interfaces {

class Fanfics : public IDBWebIDIndex {
    public:
    virtual ~Fanfics() = default;
    void ClearQueues() {
        updateQueue.clear();
        insertQueue.clear();
    }

    void ClearIndex();
    void ClearIndexWithIdIndex();
    void AddFicToIndex(core::FicPtr);

    int LoadFicIntoIdHash(core::FicPtr fic);
    void LoadFicIntoIdHash(QString website, int webId);

    bool EnsureFicLoaded(int id, QString website);
    bool LoadFicFromDB(int id, QString website);
    bool LoadFicToDB(core::FicPtr);

    core::FicPtr GetFicById(int);

    virtual int GetIDFromWebID(int, QString website);
    virtual int GetWebIDFromID(int, QString website);

    virtual bool IsEmptyQueues();
    virtual int GetIdForUrl(QString url) = 0;

    bool ReprocessFics(QString where, QString website,  bool useDirectIds = false, const std::function<void(int)>& f = std::function<void(int)>());

    void AddRecommendations(QList<core::FicRecommendation> recommendations);

    void ProcessIntoDataQueues(QList<core::FicPtr> fics, bool alwaysUpdateIfNotInsert = false);
    void CalcStatsForFics(QList<QSharedPointer<core::Fanfic>>);
    bool WriteRecommendations();
    bool WriteFicRelations(QList<core::FicWeightResult> result);
    bool WriteAuthorsForFics(QHash<uint32_t, uint32_t>);

    bool FlushDataQueues();

    virtual bool DeactivateFic(int ficId, QString website);
    virtual bool DeactivateFic(int ficId) = 0;
    void ClearProcessedHash();

    QStringList GetFandomsForFicAsNames(int ficId);
    QSet<int> GetAllKnownSlashFics();
    QSet<int> GetAllKnownNotSlashFics();
    QSet<int> GetSingularFicsInLargeButSlashyLists();
    QSet<int> GetAllKnownFicIDs(QString where);
    QSet<int> GetFicIDsWithUnsetAuthors();
    // used on the server with threaded user data
    QHash<int, core::FanficCompletionStatus> GetSnoozeInfo();
    // used on the client to fetch snoozed fics to pass to the server
    QHash<int, core::FanficSnoozeStatus> GetUserSnoozeInfo(bool fetchExpired = true, bool useLimitedSelection = false);
    bool WriteExpiredSnoozes(QSet<int>);
    bool SnoozeFic(core::FanficSnoozeStatus);
    bool RemoveSnooze(int ficId);

    bool FetchSnoozesForFics(QVector<core::Fanfic>*);
    bool FetchNotesForFics(QVector<core::Fanfic>*);
    bool FetchChaptersForFics(QVector<core::Fanfic>*);


    bool AddNoteToFic(int ficId, QString note);
    bool RemoveNoteFromFic(int ficId);

    QVector<core::FicWeightPtr> GetAllFicsWithEnoughFavesForWeights(int faves);
    QHash<int, core::FicWeightPtr> GetHashOfAllFicsWithEnoughFavesForWeights(int faves);

    bool ProcessSlashFicsBasedOnWords( std::function<SlashPresence (QString, QString, QString)> func);

    bool AssignChapter(int, int);
    bool AssignScore(int, int);
    bool AssignSlashForFic(int, int source);
    bool AssignQueuedForFic(int ficId);
    bool AssignIterationOfSlash(QString iteration);
    bool PerformGenreAssignment();
    QHash<int, double> GetFicGenreData(QString genre, QString cutoff);
    QHash<int, std::array<double, 22>> GetFullFicGenreData();
    QHash<int, double> GetDoubleValueHashForFics(QString fieldName);
    QHash<int, QString> GetGenreForFics();
    QHash<int, int> GetScoresForFics();
    // this uses preloaded fics from static data
    // to return an ID list for recommendations creator
    QSet<int> ConvertFFNSourceFicsToDB(QString userToken);
    QHash<uint32_t, core::FicWeightPtr> GetFicsForRecCreation();

    bool ConvertFFNTaggedFicsToDB(QHash<int, int>& hash);
    bool ConvertDBFicsToFFN(QHash<int, int>& hash);
    void ResetActionQueue();


    // update interface
    QReadWriteLock mutex;

    QHash<int, core::FicPtr> updateQueue;
    QHash<int, core::FicPtr> insertQueue;

    QList<core::FicRecommendation> ficRecommendations;

    QSet<int> processedHash;

    int skippedCounter = 0;
    int insertedCounter = 0;
    int updatedCounter = 0;
    QHash<int, core::FicPtr> fanficIndex;

    QSharedPointer<Fandoms> fandomInterface;
    QSharedPointer<Authors> authorInterface;
    QSqlDatabase db;
private:
    int GetIdFromDatabase(QString website, int id);
    int GetIdFromDatabase(core::SiteId);
    // index for ids only, for cases where I don't need to operate on whole fics
    struct IdResult
    {
        bool exists = false;
        bool valid = false;
        int id = -1;
    };
    struct FicIdToWebsiteMapping{
        void Clear();
        IdResult GetDBIdByWebId(core::SiteId);
        IdResult GetDBIdByWebId(QString, int);
        IdResult GetWebIdByDBId(QString, int);
        void Add(QString website, int webId, int id);
        void ProcessIdentityIntoMappings(core::Identity);
        QHash<int, QHash<QString, int>> idIndex;
        QHash<QString, QHash<int, int>> webIdIndex;
    };
    FicIdToWebsiteMapping idToWebsiteMappings;

};
}

