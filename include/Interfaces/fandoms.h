#pragma once
#include "Interfaces/base.h"
#include "Interfaces/db_interface.h"
#include "section.h"
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QList>
#include <QVector>
#include <QReadWriteLock>
#include <functional>


namespace interfaces {

class Fandoms{
public:
    Fandoms() = default;
    virtual ~Fandoms();

    virtual void Clear() ;
    virtual void ClearIndex() ;
    virtual bool Sync(bool forcedSync = false) ;
    virtual bool IsDataLoaded() ;

    virtual bool Load() ;
    bool LoadTrackedFandoms(bool forced = false);
    bool LoadAllFandoms(bool forced = false);
    virtual bool LoadFandom(QString name);
    virtual bool EnsureFandom(QString name);
    QSet<QString> EnsureFandoms(QList<core::FicPtr>);
    bool RecalculateFandomStats(QStringList fandoms);
    void Reindex();
    void AddToIndex(core::FandomPtr);

    bool WipeFandom(QString name);
    int GetFandomCount();

    virtual int GetIDForName(QString) ;
    virtual core::FandomPtr GetFandom(QString);

    virtual void SetTracked(QString, bool value, bool immediate = true);
    virtual bool IsTracked(QString);
    virtual QStringList ListOfTrackedNames();
    virtual QList<core::FandomPtr> ListOfTrackedFandoms();

    virtual bool CreateFandom(core::FandomPtr);
    virtual bool CreateFandom(QString);
    virtual bool AssignTagToFandom(QString, QString tag);
    virtual void PushFandomToTopOfRecent(QString);

    QStringList GetRecentFandoms();
    QStringList GetFandomList();

    virtual void CalculateFandomsAverages();
    virtual void CalculateFandomsFicCounts();

    QList<core::FandomPtr> FilterFandoms(std::function<bool(core::FandomPtr)>);

    QSqlDatabase db;
    QSharedPointer<database::IDBWrapper> portableDBInterface;
private:
    bool AddToTopOfRecent(QString);
    void FillFandomList(bool forced = false);

    QList<core::FandomPtr> fandoms;
    QHash<QString, int> indexFandomsByName;
    QHash<QString, core::FandomPtr> nameIndex;
    QHash<int, core::FandomPtr> idIndex;
    QList<core::FandomPtr> updateQueue;
    QList<core::FandomPtr> recentFandoms;
    QList<core::FandomPtr> trackedFandoms;
    QStringList fandomsList;
    int fandomCount = 0;


    bool isLoaded = false;
};

}
