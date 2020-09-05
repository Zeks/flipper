#pragma once

#include "GlobalHeaders/SingletonHolder.h"

#include <QSqlDatabase>
#include <mutex>

namespace discord {
struct SqliteConnectionToken{
    QString databaseName;
    QString initFileName;
    QString folder;
};

struct LockedDatabase{
    LockedDatabase(std::recursive_mutex& lock):lockGuard(lock){}
    ~LockedDatabase();
    std::lock_guard<std::recursive_mutex> lockGuard;
    QSqlDatabase db;
};



class DatabaseVendor{
public:
    QSharedPointer<LockedDatabase> GetDatabase(QString name);
    void AddConnectionToken(QString, SqliteConnectionToken);
private:
    QSqlDatabase InstantiateDatabase(const SqliteConnectionToken&);
    SqliteConnectionToken users;
    SqliteConnectionToken pageCache;
    std::recursive_mutex usersLock;
    std::recursive_mutex pageCacheLock;
};

}
BIND_TO_SELF_SINGLE(discord::DatabaseVendor);
