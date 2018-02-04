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
#include "core/section.h"


namespace interfaces {

struct WriteStats
{
    QList<QSharedPointer<core::Fic>> requiresInsert;
    QList<QSharedPointer<core::Fic>> requiresUpdate;
    bool HasData(){return requiresInsert.size() > 0 || requiresUpdate.size() > 0;}
};


class IDBWrapper;
//class IDBPersistentData{
//  public:
//    virtual ~IDBPersistentData();
//    virtual bool IsDataLoaded() = 0;
//    virtual bool Sync(bool forcedSync = false) = 0;
//    virtual bool Load() = 0;
//    virtual void Clear() = 0;
//    bool isLoaded = false;
//};

class IDBWebIDIndex{
  public:
    virtual ~IDBWebIDIndex();
    virtual int GetIDFromWebID(int) = 0;
    virtual int GetWebIDFromID(int) = 0;
    virtual int GetIDFromWebID(int, QString website) = 0;
    virtual int GetWebIDFromID(int, QString website) = 0;

};

class IDbService
{
public:
    virtual ~IDbService();
    virtual void DropAllFanficIndexes() = 0;
    virtual void RebuildAllFanficIndexes() = 0;
    virtual void EnsureWebIdsFilled() = 0;
    virtual void EnsureFFNUrlsShort() = 0;

};
class Fandoms;
class Fanfics;
class Authors;
class IDBWrapper;

class DataInterfaces
{
public:
    QSharedPointer<Fandoms> fandoms;
    QSharedPointer<Fanfics> fanfics;
    QSharedPointer<Authors> authors;
    QSharedPointer<IDBWrapper> portableDBInterface;
};


}
