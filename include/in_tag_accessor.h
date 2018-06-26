#pragma once

#include <QHash>
#include <QSet>
#include <QReadWriteLock>
#include <QSharedPointer>
#include "GlobalHeaders/SingletonHolder.h"
struct UserData{
    QSet<int> allTags;
    QSet<int> activeTags;
    QHash<int, bool> ignoredFandoms;
    QString token;
};
struct RecommendationsData{
    QSet<int> sourceFics;
    QSet<int> matchedAuthors;
    QHash<int, int> listData;
    QHash<int,int> recommendationList;
    QString token;
};

template<typename T>
struct InfoAccessor{
public:
    QHash<QString, QSharedPointer<T>> recommendatonsData;
    mutable QReadWriteLock lock;

    void SetData(QString userToken, QSharedPointer<T> data)
    {
        QWriteLocker locker(&lock);
        recommendatonsData[userToken] = data;
    }
    QSharedPointer<T> GetData(QString userToken)
    {
        QReadLocker locker(&lock);
        if(!recommendatonsData.contains(userToken))
            return QSharedPointer<T>();
        return recommendatonsData[userToken];
    }

};

struct ThreadData{
    static RecommendationsData* GetRecommendationData();
    static UserData *GetUserData();
};
using RecommendationsInfoAccessor = InfoAccessor<RecommendationsData>;
using UserInfoAccessor = InfoAccessor<UserData>;
BIND_TO_SELF_SINGLE(RecommendationsInfoAccessor);
BIND_TO_SELF_SINGLE(UserInfoAccessor);
