/*Flipper is a recommendation and search engine for fanfiction.net
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
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
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
