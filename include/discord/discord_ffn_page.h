#pragma once
#include <QString>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>
#include <QHash>
#include <QDateTime>
#include <chrono>
#include <regex>
#include <set>

#include "GlobalHeaders/SingletonHolder.h"

namespace discord{

struct FFNPage{
    public:
    FFNPage(){}
    int getTotalParseCounter() const;
    void setTotalParseCounter(int value);

    int getDailyParseCounter() const;
    void setDailyParseCounter(int value);

    int getFavourites() const;
    void setFavourites(int value);

    QDate getLastParsed() const;
    void setLastParsed(const QDate &value);

    std::string getId() const;
    void setId(const std::string &value);

private:
    int totalParseCounter = 0 ;
    int dailyParseCounter = 0;
    int favourites = 0;

    QDate lastParsed;
    std::string id;

    mutable QReadWriteLock lock = QReadWriteLock(QReadWriteLock::Recursive);
};


struct FfnPages{
    void AddPage(QSharedPointer<FFNPage>);
    bool HasPage(const std::string&);
    QSharedPointer<FFNPage> GetPage(const std::string&) const;
    bool LoadPage(const std::string&);
    QHash<std::string,QSharedPointer<FFNPage>> pages;
    QReadWriteLock lock;
};
}

BIND_TO_SELF_SINGLE(discord::FfnPages);
