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
#include <QSet>
#include <QReadWriteLock>
#include <QSharedPointer>
#include "GlobalHeaders/SingletonHolder.h"
#include "core/fandom_list.h"
struct UserData{
    void Clear(){
        allTaggedFics.clear();
        allSnoozedFics.clear();
        usedAuthors.clear();
        ficIDsForActivetags.clear();
        ficsForAuthorSearch.clear();
        ficsForSelection.clear();
        ignoredFandoms.clear();
        token = QStringLiteral("");
        hasWhitelistedFandoms = false;
        fandomStates.clear();
    };
    QSet<int> allTaggedFics;
    QSet<int> allSnoozedFics;
    QSet<int> usedAuthors;
    //QSet<int> likedAuthors;
    QSet<int> ficIDsForActivetags;
    QSet<int> ficsForAuthorSearch;
    QSet<int> ficsForSelection;
    QHash<int, bool> ignoredFandoms;
    std::unordered_map<int,core::fandom_lists::FandomSearchStateToken> fandomStates;
    QString token;
    bool hasWhitelistedFandoms = false;
};
struct RecommendationsData{
    QSet<int> sourceFics;
    QSet<int> matchedAuthors;
    QHash<int, int> listData;
    std::unordered_map<int,int> ficMetascores;
    std::unordered_map<int,int> ficVotes;
    QHash<int,int> scoresList;
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


