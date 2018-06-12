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

struct UserPageSink
{
    QList<core::FicSectionStatsTemporaryToken>tokens;
    int currentSize = 0;
    void Push(core::FicSectionStatsTemporaryToken token){
        QWriteLocker locker(&lock);
        tokens.push_back(token);
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
