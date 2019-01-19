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

class RecCalculatorImplBase
{
public:
    typedef QList<std::function<bool(AuthorResult&,QSharedPointer<RecommendationList>)>> FilterListType;
    typedef QList<std::function<void(RecCalculatorImplBase*,AuthorResult &)>> ActionListType;

    RecCalculatorImplBase(const DataHolder::FavType& faves,
                          const DataHolder::FicType& fics):favs(faves),fics(fics){}

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


    int matchSum = 0;
    const DataHolder::FavType& favs;
    const DataHolder::FicType& fics;
    QSharedPointer<RecommendationList> params;
    //QList<int> matchedAuthors;
    QHash<uint32_t, core::FicWeightPtr> fetchedFics;
    QHash<int, AuthorResult> allAuthors;
    int maximumMatches = 0;
    int prevMaximumMatches = 0;
    QList<int> filteredAuthors;
    Roaring ownFavourites;
    RecommendationListResult result;
};
static auto matchesFilter = [](AuthorResult& author, QSharedPointer<RecommendationList> params){
    return author.matches >= params->minimumMatch || author.matches >= params->alwaysPickAt;
};
static auto ratioFilter = [](AuthorResult& author, QSharedPointer<RecommendationList> params)
{return author.ratio <= params->pickRatio && author.matches > 0;};
static auto authorAccumulator = [](RecCalculatorImplBase* ptr,AuthorResult & author)
{
    ptr->filteredAuthors.push_back(author.id);
};


//auto ratioAccumulator = [&ratioSum](RecCalculatorImplBase* ,AuthorResult & author){ratioSum+=author.ratio;};
class RecCalculatorImplDefault: public RecCalculatorImplBase{
public:
    RecCalculatorImplDefault(const DataHolder::FavType& faves, const DataHolder::FicType& fics): RecCalculatorImplBase(faves, fics){}
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
