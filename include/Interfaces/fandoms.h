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
#include "Interfaces/db_interface.h"
#include "core/section.h"
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
    //virtual bool Sync(bool forcedSync = false) ;
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
    void FillFandomList(bool forced = false);

    bool WipeFandom(QString name);
    int GetFandomCount();
    int GetLastFandomID();

    virtual int GetIDForName(QString) ;
    virtual core::FandomPtr GetFandom(QString);

    virtual void SetLastUpdateDate(QString, QDate);
    virtual void SetTracked(QString, bool value, bool immediate = true);
    virtual bool IsTracked(QString);

    bool IgnoreFandom(QString name, bool includeCrossovers);
    bool IgnoreFandom(int id, bool includeCrossovers);
    QStringList GetIgnoredFandoms() const;
    QList<core::FandomPtr> GetAllLoadedFandoms();

    bool RemoveFandomFromIgnoredList(QString name);
    bool RemoveFandomFromIgnoredList(int id);

    bool IgnoreFandomSlashFilter(QString name);
    bool IgnoreFandomSlashFilter(int id);
    QStringList GetIgnoredFandomsSlashFilter() const;
    bool RemoveFandomFromIgnoredListSlashFilter(QString name);
    bool RemoveFandomFromIgnoredListSlashFilter(int id);


    virtual QStringList ListOfTrackedNames();
    virtual QList<core::FandomPtr> ListOfTrackedFandoms();

    virtual bool CreateFandom(core::FandomPtr);
    virtual bool CreateFandom(QString);
    virtual bool AddFandomLink(QString fandom, core::Url);
    virtual bool AssignTagToFandom(QString, QString tag, bool includeCrosses = false);
    virtual void PushFandomToTopOfRecent(QString);
    virtual void RemoveFandomFromRecentList(QString);

    QStringList GetRecentFandoms();
    void ReloadRecentFandoms();
    QStringList GetFandomList(bool forced = false);

    virtual void CalculateFandomsAverages();
    virtual void CalculateFandomsFicCounts();

    QList<core::FandomPtr> FilterFandoms(std::function<bool(core::FandomPtr)>);

    QSqlDatabase db;
    QSharedPointer<database::IDBWrapper> portableDBInterface;
private:
    bool AddToTopOfRecent(QString);



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
