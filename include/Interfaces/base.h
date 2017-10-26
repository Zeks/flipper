#pragma once
#include "section.h"

namespace database {

struct WriteStats
{
    QList<QSharedPointer<core::Fic>> requiresInsert;
    QList<QSharedPointer<core::Fic>> requiresUpdate;
    bool HasData(){return requiresInsert.size() > 0 || requiresUpdate.size() > 0;}
};


class IDBWrapper;
class IDBPersistentData{
  public:
    virtual ~IDBPersistentData();
    virtual bool IsDataLoaded() = 0;
    virtual bool Sync(bool forcedSync = false) = 0;
    virtual bool Load() = 0;
    virtual void Clear() = 0;
    bool isLoaded = false;
};

class IDBWebIDIndex{
  public:
    virtual ~IDBWebIDIndex();
    virtual int GetIDFromWebID(int) = 0;
    virtual int GetWebIDFromID(int) = 0;
    virtual int GetIDFromWebID(int, QString website) = 0;
    virtual int GetWebIDFromID(int, QString website) = 0;
    QHash<int, int> indexIDByWebID;
    QHash<int, int> indexWebIDByID;
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
class DBFandomsBase;
class DBFanficsBase;
class DBAuthorsBase;
class IDBWrapper;

class DataInterfaces
{
    QSharedPointer<DBFandomsBase> fandoms;
    QSharedPointer<DBFanficsBase> fanfics;
    QSharedPointer<DBAuthorsBase> authors;
    QSharedPointer<IDBWrapper> portableDBInterface;
};


}
