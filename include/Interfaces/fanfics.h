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
#pragma once
#include "Interfaces/base.h"
#include "core/section.h"
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
    virtual ~Fanfics();
    void ClearQueues() {
        updateQueue.clear();
        insertQueue.clear();
    }

    void ClearIndex();
    void ClearIndexWithIdIndex();
    void Reindex();
    void AddFicToIndex(core::FicPtr);

    int EnsureId(core::FicPtr fic);
    void EnsureId(QString website, int webId);

    bool EnsureFicLoaded(int id, QString website);
    bool LoadFicFromDB(int id, QString website);
    bool LoadFicToDB(core::FicPtr);

    core::FicPtr GetFicById(int);

    virtual int GetIDFromWebID(int, QString website);
    virtual int GetWebIDFromID(int, QString website);

    virtual bool IsEmptyQueues();
    virtual int GetIdForUrl(QString url) = 0;
//    QList<core::Fic>  GetCurrentFicSetForQML();

    bool ReprocessFics(QString where, QString website,  bool useDirectIds = false, std::function<void(int)> f = std::function<void(int)>());

    void AddRecommendations(QList<core::FicRecommendation> recommendations);

    void ProcessIntoDataQueues(QList<core::FicPtr> fics, bool alwaysUpdateIfNotInsert = false);
    void CalcStatsForFics(QList<QSharedPointer<core::Fic>>);
    bool WriteRecommendations();
    bool FlushDataQueues();

    virtual bool DeactivateFic(int ficId, QString website);
    virtual bool DeactivateFic(int ficId) = 0;
    void ClearProcessedHash();

    QStringList GetFandomsForFicAsNames(int ficId);
    QSet<int> GetAllKnownSlashFics();
    QSet<int> GetAllKnownNotSlashFics();
    QSet<int> GetAllKnownFicIDs(QString where);
    bool ProcessSlashFicsBasedOnWords( std::function<SlashPresence (QString, QString, QString)> func);

    bool AssignChapter(int, int);
    bool AssignSlashForFic(int, int source);
    bool AssignIterationOfSlash(QString iteration);

    // update interface
    QReadWriteLock mutex;

    QHash<int, core::FicPtr> updateQueue;
    QHash<int, core::FicPtr> insertQueue;

    QList<core::FicRecommendation> ficRecommendations;

    QSet<int> processedHash;

    int skippedCounter = 0;
    int insertedCounter = 0;
    int updatedCounter = 0;
    QList<core::FicPtr> fics;
    QHash<int, core::FicPtr> idIndex;
    QHash<QString, QHash<int, core::FicPtr>> webIdIndex;

    QSharedPointer<Fandoms> fandomInterface;
    QSharedPointer<Authors> authorInterface;
    QSqlDatabase db;
private:
    int GetIdFromDatabase(QString website, int id);
    // index for ids only, for cases where I don't need to operate on whole fics
    struct IdResult
    {
        bool exists = false;
        bool valid = false;
        int id = -1;
    };
    struct FicIds{
        void Clear();
        IdResult GetIdByWebId(QString, int);
        IdResult GetWebIdById(QString, int);
        void Add(QString website, int webId, int id);
//        void AddToAccessedWebIds(QString website, int webId);
//        void AddToAccessedIds(int id);
        //QSet<QPair<QString, int>> accessedWebIDs;
        //QSet<int> accessedIDs;
        QHash<int, QHash<QString, int>> idIndex;
        QHash<QString, QHash<int, int>> webIdIndex;
    };
    FicIds idOnly;

};
}

