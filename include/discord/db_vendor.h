#pragma once

#include "GlobalHeaders/SingletonHolder.h"

#include "database.h"
#include <mutex>

namespace discord {
struct SqliteConnectionToken{
    QString databaseName;
    QString initFileName;
    QString folder;
};

struct LockedDatabase : public core::Database{
    LockedDatabase(std::recursive_mutex& lock):core::Database(), lockGuard(lock){}
    ~LockedDatabase();
    std::lock_guard<std::recursive_mutex> lockGuard;
    sql::Database db;
};



class DatabaseVendor{
public:
    QSharedPointer<LockedDatabase> GetDatabase(QString name);
    void AddConnectionToken(QString, const SqliteConnectionToken &);
private:
    sql::Database InstantiateDatabase(const SqliteConnectionToken&);
    SqliteConnectionToken users;
    SqliteConnectionToken pageCache;
    std::recursive_mutex usersLock;
    std::recursive_mutex pageCacheLock;
};

}
BIND_TO_SELF_SINGLE(discord::DatabaseVendor);
