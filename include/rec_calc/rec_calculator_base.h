/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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

#include <QList>

#include "include/data_code/data_holders.h"
#include "include/data_code/rec_calc_data.h"



namespace core{

struct RecommendationListResult{
    void AddToBreakdown(uint32_t fic, AuthorWeightingResult::EAuthorType type, double value){
        if(!breakdowns.contains(fic))
            breakdowns[fic].ficId = fic;
        breakdowns[fic].AddAuthor(type, value);
    }
    QHash<int, int> recommendations;
    QHash<int, int> matchReport;
    QHash<uint32_t, MatchBreakdown> breakdowns;
    QHash<int, int> pureMatches;
    QHash<int, int> decentMatches;
    QList<int> authors;
};


struct UserMatchesInput{
    Roaring userFavourites;
    Roaring userIgnoredFandoms;
};

struct FicBaseScoreCalculator{
    int totalVotes = 0;
    bool over50votes = false;
    bool deadForAYear = false;
    bool matchedFicBelow500 = false;
    bool size1090 = false;
    int moodAsalignment = 0; // +1 to -2
    int authorLikes = 0;

};

struct RecInputVectors{
    const DataHolder::FavType& faves;
    const DataHolder::FicType& fics;
    const core::AuthorMoodDistributions& moods;
};



class RecCalculatorImplBase
{
public:
    typedef QList<std::function<bool(AuthorResult&,QSharedPointer<RecommendationList>)>> FilterListType;
    typedef QList<std::function<void(RecCalculatorImplBase*,AuthorResult &)>> ActionListType;

    RecCalculatorImplBase(RecInputVectors input):inputs(input){}

    virtual ~RecCalculatorImplBase(){}

    void Calc();
    Roaring BuildIgnoreList();
    void FetchAuthorRelations();
    void CollectFicMatchQuality();
    void Filter(QList<std::function<bool(AuthorResult&,QSharedPointer<RecommendationList>)>> filters,
                QList<std::function<void(RecCalculatorImplBase*,AuthorResult &)>> actions);
    virtual void CollectVotes();

    virtual void CalcWeightingParams() = 0;
    virtual FilterListType GetFilterList() = 0;
    virtual ActionListType  GetActionList() = 0;
    virtual std::function<AuthorWeightingResult(AuthorResult&, int, int)> GetWeightingFunc() = 0;

    virtual std::optional<double> GetNeutralDiffForLists(uint32_t){return {};}
    virtual std::optional<double> GetTouchyDiffForLists(uint32_t){return {};}

    int matchSum = 0;
    RecInputVectors inputs;
    QSharedPointer<RecommendationList> params;
    //QList<int> matchedAuthors;
    QHash<uint32_t, core::FicWeightPtr> fetchedFics;
    QHash<int, AuthorResult> allAuthors;
    int maximumMatches = 0;
    int prevMaximumMatches = 0;
    QList<int> filteredAuthors;
    Roaring ownFavourites;
    RecommendationListResult result;

    int votesBase = 1;
};
static auto matchesFilter = [](AuthorResult& author, QSharedPointer<RecommendationList> params){
    return author.matches >= params->minimumMatch || author.matches >= params->alwaysPickAt;
};
static auto ratioFilter = [](AuthorResult& author, QSharedPointer<RecommendationList> params)
{
    return author.ratio <= params->pickRatio && author.matches > 0;
};
static auto authorAccumulator = [](RecCalculatorImplBase* ptr,AuthorResult & author)
{
    ptr->filteredAuthors.push_back(author.id);
};


//auto ratioAccumulator = [&ratioSum](RecCalculatorImplBase* ,AuthorResult & author){ratioSum+=author.ratio;};
class RecCalculatorImplDefault: public RecCalculatorImplBase{
public:
    RecCalculatorImplDefault(RecInputVectors input): RecCalculatorImplBase(input){}
    virtual FilterListType GetFilterList(){
        return {matchesFilter, ratioFilter};
    }
    virtual ActionListType GetActionList(){
        return {authorAccumulator};
    }
    virtual std::function<AuthorWeightingResult(AuthorResult&, int, int)> GetWeightingFunc(){
        return [](AuthorResult&, int, int){return AuthorWeightingResult();};
    }
    void CalcWeightingParams(){
        // does nothing
    }
};

}
