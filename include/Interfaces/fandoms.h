#pragma once
#include "Interfaces/base.h"
#include "Interfaces/db_interface.h"
//#include "section.h"
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QList>
#include <QVector>
#include <QReadWriteLock>
#include <functional>


namespace interfaces {

class Fandoms : public IDBPersistentData{
public:
    Fandoms() = default;
    void Reindex();
    void AddToIndex(QSharedPointer<core::Fandom>);
    virtual ~Fandoms();
    virtual int GetIDForName(QString) ;
    virtual void SetTracked(QString, bool value, bool immediate = true);
    virtual bool IsTracked(QString);
    virtual QStringList ListOfTrackedNames();
    virtual QList<QSharedPointer<core::Fandom>> ListOfTrackedFandoms();
//    virtual QList<int> AllTracked();
//    virtual QStringList AllTrackedStr();

    virtual bool CreateFandom(QSharedPointer<core::Fandom>);
    virtual bool LoadFandom(QString name);
    virtual bool EnsureFandom(QString name);
    virtual QSharedPointer<core::Fandom> GetFandom(QString);

    virtual bool AssignTagToFandom(QString, QString tag) = 0;
    virtual QStringList PushFandomToTopOfRecent(QString);
    void RebaseFandomsToZero();
    QStringList GetRecentFandoms();
    QStringList GetFandomList();



    //virtual QStringList FetchRecentFandoms() = 0;
    //virtual void RebaseFandomsToZero() = 0;
    //QString DBFandomsBase::GetCurrentCrossoverUrl()

    virtual void CalculateFandomAverages() = 0;
    virtual void CalculateFandomFicCounts() = 0;

    virtual bool Load() override;
    bool LoadTrackedFandoms();
    bool LoadAllFandoms();
    virtual void Clear() override;
    virtual bool Sync(bool forcedSync = false) override;
    virtual bool IsDataLoaded() override;

    QList<QSharedPointer<core::Fandom>> FilterFandoms(std::function<bool(QSharedPointer<core::Fandom>)>);

    QList<QSharedPointer<core::Fandom>> fandoms;
    QHash<QString, int> indexFandomsByName;
    QHash<QString, QSharedPointer<core::Fandom>> nameIndex;
    QList<QSharedPointer<core::Fandom>> updateQueue;
    QList<QSharedPointer<core::Fandom>> recentFandoms;
    QList<QSharedPointer<core::Fandom>> trackedFandoms;
    QSqlDatabase db;
    QSharedPointer<database::IDBWrapper> portableDBInterface;
    QStringList fandomsList;
private:
    void AddToTopOfRecent(QString);
};

}
