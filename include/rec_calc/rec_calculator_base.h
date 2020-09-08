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
    bool success = false;
    QHash<int, int> recommendations;
    QHash<int, int> matchReport;
    QHash<uint32_t, MatchBreakdown> breakdowns;
    QHash<int, int> pureMatches;
    QHash<int, int> decentMatches;
    QHash<int, int> sumNegativeMatchesForFic;
    QHash<int, int> sumNegativeVotesForFic;
    QHash<int, double> noTrashScore;
    QList<int> authors;
};

struct DiagnosticRecommendationListResult{
    bool isValid = false;
    RecommendationListResult recs;
    QHash<uint32_t, QVector<uint32_t>> authorsForFics;
    QVector<AuthorResult> authorData;

    double ratioMedian = 0;
    double quad = 0;
    int sigma2Dist = 0;
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

    bool Calc();
    Roaring BuildIgnoreList();
    void FetchAuthorRelations();
    void CollectFicMatchQuality();
    void Filter(QList<std::function<bool(AuthorResult&,QSharedPointer<RecommendationList>)>> filters,
                QList<std::function<void(RecCalculatorImplBase*,AuthorResult &)>> actions);

    void CalculateNegativeToPositiveRatio();
    void ReportNegativeResults();
    void FillFilteredAuthorsForFics();

    virtual bool CollectVotes();

    virtual void CalcWeightingParams() = 0;
    virtual FilterListType GetFilterList() = 0;
    virtual ActionListType  GetActionList() = 0;
    virtual std::function<AuthorWeightingResult(AuthorResult&, int, int)> GetWeightingFunc() = 0;

    virtual std::optional<double> GetNeutralDiffForLists(uint32_t){return {};}
    virtual std::optional<double> GetTouchyDiffForLists(uint32_t){return {};}

    virtual void AutoAdjustRecommendationParamsAndFilter();
    virtual void AdjustRatioForAutomaticParams();

    int ownProfileId = -1;
    uint32_t matchSum = 0;
    uint32_t negativeAverage = 0;
    RecInputVectors inputs;
    QSharedPointer<RecommendationList> params;
    //QList<int> matchedAuthors;
    QHash<uint32_t, core::FicWeightPtr> fetchedFics;
    QHash<int, AuthorResult> allAuthors;
    uint32_t maximumMatches = 0;
    uint32_t prevMaximumMatches = 0;
    double averageNegativeToPositiveMatches = 0;
    uint32_t startOfTrashCounting = 200;
    bool doTrashCounting = true;
    QList<int> filteredAuthors;
    Roaring ownFavourites;
    Roaring ownMajorNegatives;
    RecommendationListResult result;
    QHash<uint32_t, QVector<uint32_t>> authorsForFics;
    bool needsDiagnosticData = false;

    int votesBase = 1;
};
static auto matchesFilter = [](AuthorResult& author, QSharedPointer<RecommendationList> params){
    return static_cast<int>(author.matches) >= params->minimumMatch || static_cast<int>(author.matches) >= params->alwaysPickAt;
};
static auto ratioFilter = [](AuthorResult& author, QSharedPointer<RecommendationList> params)
{
    return author.ratio <= params->maxUnmatchedPerMatch && author.matches > 0;
};

static auto negativeFilter = [](AuthorResult& author, QSharedPointer<RecommendationList> list )
{
    if(!list->useDislikes)
        return true;
    bool filterResult = (static_cast<double>(author.negativeMatches)/author.matches >= 2 || static_cast<double>(author.sizeAfterIgnore)/author.negativeMatches < 15)
            && static_cast<double>(author.negativeMatches)/author.matches >= 1.5 ;
    return !filterResult;
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
        return {matchesFilter, ratioFilter, negativeFilter};
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
