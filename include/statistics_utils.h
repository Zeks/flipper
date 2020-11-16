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
#pragma once
#include <QHash>
#include <QWriteLocker>
#include <QReadLocker>
#include "include/webpage.h"
#include "include/core/section.h"
namespace statistics_utils
{
struct UserPagePageResult
{
    WebPage page;
    int authorId = -1;
    bool finished = false;
};


struct UserPageSource
{
    QList<WebPage>pages;
    int initialSize = 0;
    UserPagePageResult Get(){
        QWriteLocker locker(&lock);

        UserPagePageResult result;
        if(pages.size() == 0)
        {
            result.finished = true;
            return result;
        }
        result.page = pages.last();
        pages.pop_back();
        return result;
    }
    void Push(WebPage token){
        QWriteLocker locker(&lock);
        pages.push_back(token);
    }
    void Reserve(int size)
    {
        QReadLocker locker(&lock);
        pages.clear();
        pages.reserve(size);
    }
    int GetInitialSize() const {
        QReadLocker locker(&lock);
        return initialSize;
    }
    mutable QReadWriteLock lock;
};
//core::FicSectionStatsTemporaryToken token
template <typename T>
struct UserPageSink
{
    QList<T> tokens;
    int currentSize = 0;
    void Push(T token){
        QWriteLocker locker(&lock);
        //token.Log();
        tokens.push_back(token);
//        for(auto token: tokens)
//            token.Log();
        currentSize++;
    }
    int GetSize() const {
        QReadLocker locker(&lock);
        return tokens.size();
    }
    mutable QReadWriteLock lock;
};

template <typename T, typename Y>
void Combine(QHash<T, Y>& firstHash, QHash<T, Y> secondHash)
{
    for(auto key: secondHash.keys())
    {
        firstHash[key] += secondHash[key];
    }
}
}
