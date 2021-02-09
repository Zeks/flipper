#pragma once
#include "include/ECacheMode.h"
namespace fetching{

struct CacheStrategy{
    bool useCache = true;
    bool abortIfCacheUnavailable = false;
    bool fetchIfCacheIsOld = false;
    int cacheExpirationDays = 0;
};


}
