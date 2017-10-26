#pragma once
#include "Interfaces/base.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace database {

class DBFandomsBase : public IDBPersistentData{
public:
    virtual ~DBFandomsBase();
    virtual int GetID(QString) ;
    virtual void SetTracked(QString, bool value, bool immediate);
    virtual bool IsTracked(QString);
    virtual QStringList ListOfTracked();
    virtual QList<int> AllTracked();
    virtual QStringList AllTrackedStr();

    virtual bool CreateFandom(QSharedPointer<core::Fandom>);
    virtual bool LoadFandom(QString name);
    virtual bool EnsureFandom(QString name);
    virtual QSharedPointer<core::Fandom> GetFandom(QString);

    virtual bool AssignTagToFandom(QString, QString tag) = 0;
    virtual QStringList PushFandomToTopOfRecent(QString);
    QStringList GetRecentFandoms() const;
    void AddToTopOfRecent(QString);

    //virtual QStringList FetchRecentFandoms() = 0;
    //virtual void RebaseFandomsToZero() = 0;

    virtual void CalculateFandomAverages() = 0;
    virtual void CalculateFandomFicCounts() = 0;

    virtual bool Load() override;
    virtual void Clear() override;
    virtual bool Sync(bool forcedSync = false) override;
    virtual bool IsDataLoaded() override;


    QHash<QString, int> indexFandomsByName;
    QHash<QString, QSharedPointer<core::Fandom>> fandoms;
    QList<QSharedPointer<core::Fandom>> updateQueue;
    QList<QSharedPointer<core::Fandom>> recentFandoms;
    QSqlDatabase db;
    QSharedPointer<IDBWrapper> portableDBInterface;

};

}
