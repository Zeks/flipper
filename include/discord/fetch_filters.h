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

#include <QVector>
#include <QSharedPointer>
#include "Interfaces/discord/users.h"
#include "include/storyfilter.h"
#include "grpc/grpc_source.h"

namespace discord{
struct StoryFilterDisplayToken{
    struct RollMemo{
        bool usingRoll = false;
        QString rollType = "all";
        int perfectRngFicsSize = 0;
        int goodRngFicsSize = 0;
    };
    bool isArchivedPage = false;
    QString ficId;
    QSet<int> fandoms;
    WordcountFilter wordCountFilter;
    QString publishedFilter;
    QString finishedFilter;
    bool usingLikedFilter = false;
    bool usingFreshSort = false;
    bool usingGemsSort = false;
    bool usingCompleteFilter = false;
    bool usingDeadFilter = false;
    RollMemo rollMemo;
};

class FicFetcherBase{
public:
    FicFetcherBase(){};
    virtual ~FicFetcherBase(){};
    virtual void Fetch(core::StoryFilter, QVector<core::Fanfic>* fics);
    virtual int FetchPageCount(core::StoryFilter partialfilter);
    virtual void FillFilterMemoToken();
    virtual void FillUserPart();
    virtual core::StoryFilter CreateFilter() = 0;
    virtual void FillFicData() = 0;

    void ResetFilterForSimilarityList(core::StoryFilter&);

    bool displayingSimilarityList = false;;
    int pageToUse = 0;
    int recordLimit = 0;
    core::StoryFilter filter;
    QSharedPointer<FicSourceGRPC> source;
    QSharedPointer<discord::User> user;
    QSharedPointer<core::RecommendationListFicData> sourceficsData;
    StoryFilterDisplayToken storyFilterDisplayToken;
};

class FicFetcherPage : public FicFetcherBase{
public:
    FicFetcherPage(){};
    virtual ~FicFetcherPage(){};
    virtual core::StoryFilter CreateFilter() override;
    virtual void FillFicData() override;

};

class FicFetcherRNG: public FicFetcherBase{
public:
    FicFetcherRNG(){};
    virtual ~FicFetcherRNG(){};
    virtual core::StoryFilter CreateFilter() override;
    virtual void FillFicData() override;
    int qualityCutoff = 0;
};

// the only func that doesn't need its own class
// because it won't be called again against the same message
void FetchFicsForShowIdCommand(QSharedPointer<FicSourceGRPC> source,
                                    QList<int> ficIds,
                                    QVector<core::Fanfic>* fics);



}
