#pragma once
#include <string>
#include <QString>
#include <QQueue>
#include <QReadWriteLock>
#include <QReadLocker>
#include "sql_abstractions/sql_database.h"
#include <QWriteLocker>
#include <QHash>
#include <chrono>
#include "core/section.h"

//#include "Interfaces/discord/users.h"
#include "GlobalHeaders/SingletonHolder.h"

namespace interfaces{
class Users;
}

namespace discord{
struct IgnoreFandom{
   int id;
    bool withCrosses;
};
enum EListType{
    elt_favourites = 0,
    elt_urls = 1
};
}
