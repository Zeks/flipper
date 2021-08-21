#pragma once
#include "include/ECacheMode.h"
#include <QDateTime>
#include <functional>
namespace fetching{

struct CacheStrategy{
    bool useCache = false;
    bool abortIfCacheUnavailable = false;
    bool fetchIfCacheUnavailable = true;
    bool fetchIfCacheIsOld = false;
    bool saveResults = true;
    int cacheExpirationDays = 0;
    std::function<bool(QString)> pageChecker;

    bool CacheIsExpired(QDate generationDate){
        if(cacheExpirationDays != 0){
            return QDate::currentDate().addDays(-1*cacheExpirationDays) > generationDate;
        }
        return false;
    };

    static CacheStrategy CacheThenFetchIfNA(int cacheExpirationDays = 0){
        fetching::CacheStrategy cacheStrategy;
        cacheStrategy.useCache = true;
        cacheStrategy.fetchIfCacheUnavailable = true;
        cacheStrategy.cacheExpirationDays = cacheExpirationDays;
        return cacheStrategy;
    }
    static CacheStrategy NetworkOnly(){
        fetching::CacheStrategy cacheStrategy;
        cacheStrategy.useCache = false;
        cacheStrategy.fetchIfCacheUnavailable = true;
        return cacheStrategy;
    }
    static CacheStrategy CacheOnly(){
        fetching::CacheStrategy cacheStrategy;
        cacheStrategy.useCache = true;
        cacheStrategy.abortIfCacheUnavailable = true;
        return cacheStrategy;
    }
};


}
