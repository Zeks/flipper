#pragma once
#include "Interfaces/base.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"
#include <functional>


namespace interfaces {

class Fanfics : public IDBWebIDIndex, public IDBPersistentData {
public:
    virtual ~Fanfics();
    void ClearQueues() {
        updateQueue.clear();
        insertQueue.clear();
    }

    virtual int GetIDFromWebID(int, QString website) override;
    virtual int GetWebIDFromID(int, QString website) override;

    virtual bool IsEmptyQueues();
    virtual int GetIdForUrl(QString url) = 0;

    bool ReprocessFics(QString where, QString website, std::function<void(int)> f);
    virtual bool DeactivateFic(int ficId, QString website);
    virtual bool DeactivateFic(int ficId) = 0;
    void AddRecommendations(QList<core::FicRecommendation> recommendations);
    void ProcessIntoDataQueues(QList<QSharedPointer<core::Fic>> fics, bool alwaysUpdateIfNotInsert = false);
    void CalculateFandomAverages();
    void CalculateFandomFicCounts();
    void FlushDataQueues();
    QList<core::Fic>  GetCurrentFicSet();
    // queued by webid
    QReadWriteLock mutex;
    QHash<int, QSharedPointer<core::Fic>> updateQueue;
    QHash<int, QSharedPointer<core::Fic>> insertQueue;
    QList<core::FicRecommendation> ficRecommendations;
    QSharedPointer<Authors> authorInterface;
    QList<core::Fic> currentSet;
    QSqlDatabase db;

};
}
