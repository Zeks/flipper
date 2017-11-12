#pragma once
#include "Interfaces/base.h"
#include "section.h"
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


    virtual int GetIDFromWebID(int, QString website);
    virtual int GetWebIDFromID(int, QString website);

    virtual bool IsEmptyQueues();
    virtual int GetIdForUrl(QString url) = 0;
//    QList<core::Fic>  GetCurrentFicSetForQML();

    bool ReprocessFics(QString where, QString website, std::function<void(int)> f);

    void AddRecommendations(QList<core::FicRecommendation> recommendations);

    void ProcessIntoDataQueues(QList<core::FicPtr> fics, bool alwaysUpdateIfNotInsert = false);
    void CalcStatsForFics(QList<QSharedPointer<core::Fic>>);
    bool FlushDataQueues();

    virtual bool DeactivateFic(int ficId, QString website);
    virtual bool DeactivateFic(int ficId) = 0;


    bool AssignChapter(int, int);

    // update interface
    QReadWriteLock mutex;

    QHash<int, core::FicPtr> updateQueue;
    QHash<int, core::FicPtr> insertQueue;

    QList<core::FicRecommendation> ficRecommendations;


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
