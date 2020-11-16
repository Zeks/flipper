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
#include "Interfaces/base.h"
#include "Interfaces/fanfics.h"
#include "core/section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace interfaces {

class FFNFanfics : public Fanfics{
public:
    virtual ~FFNFanfics(){}
    virtual int GetIDFromWebID(int);
    virtual int GetWebIDFromID(int);
    virtual bool DeactivateFic(int ficId);
    virtual int GetIdForUrl(QString url);
    void ProcessIntoDataQueues(QList<QSharedPointer<core::Fanfic>> fics, bool alwaysUpdateIfNotInsert = false);
    void AddRecommendations(QList<core::FicRecommendation> recommendations);

    // IDBPersistentData interface
public:
    bool IsDataLoaded();
    bool Sync(bool forcedSync);
    bool Load();
    void Clear();
};
}
