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
#include "include/rec_calc/rec_calculator_weighted.h"
#include <cmath>
namespace core {



double quadratic_coef(double ratio,
                      double median,
                      double sigma,
                      int authorSize,
                      int maximumMatches,
                      ECalcType type)
{
    Q_UNUSED(ratio);
    Q_UNUSED(median);
    Q_UNUSED(sigma);
    static QSet<int> bases;
    if(!bases.contains(authorSize))
    {
        bases.insert(authorSize);
        qDebug() << "Author size: " << authorSize;
    }

    double result = 0;
    switch(type)
    {
    case ECalcType::unique:
        //qDebug() << "casting max vote: " << "matches: " << maximumMatches << " value: " << 0.2*static_cast<double>(maximumMatches);
        result =  0.2*static_cast<double>(maximumMatches);

        break;
    case ECalcType::rare:

        result = 0.05*static_cast<double>(maximumMatches);

        break;
    case ECalcType::uncommon:
    {
        //        return 5;
        auto val = 0.005*static_cast<double>(maximumMatches);
        result = val < 1 ? 1 : val;
    }

        break;
    default:
        result = 1;
    }
    return result;
}

double sqrt_coef(double ratio, double median, double sigma, int base, int scaler)
{
    auto  distanceFromMedian = median - ratio;
    if(distanceFromMedian < 0)
        return 0;
    double tau = distanceFromMedian/sigma;
    return base + std::sqrt(tau)*scaler;
}


RecCalculatorImplWeighted::FilterListType RecCalculatorImplWeighted::GetFilterList(){
    return {matchesFilter, ratioFilter, negativeFilter};
}
RecCalculatorImplBase::ActionListType RecCalculatorImplWeighted::GetActionList(){
    auto ratioAccumulator = [ratioSum = std::reference_wrapper<double>(this->ratioSum)](RecCalculatorImplBase* calc,AuthorResult & author)
    {
        if(calc->ownProfileId != static_cast<int>(author.id))
            ratioSum+=author.ratio;
    };
    return {authorAccumulator, ratioAccumulator};
};
std::function<AuthorWeightingResult(AuthorResult&, int, int)> RecCalculatorImplWeighted::GetWeightingFunc(){
    return std::bind(&RecCalculatorImplWeighted::CalcWeightingForAuthor,
                     this,
                     std::placeholders::_1,
                     std::placeholders::_2,
                     std::placeholders::_3);
}

void RecCalculatorImplWeighted::CalcWeightingParams(){
    auto authorList = filteredAuthors.values();
    QLOG_INFO() << "inputs to weighting:";
    QLOG_INFO() << "matchsum:" << matchSum;
    QLOG_INFO() << "inputs.faves.size():" << inputs.faves.size();
    QLOG_INFO() << "ratioSum:" << ratioSum;
    QLOG_INFO() << "filteredAuthors.size():" << authorList.size();
    needsRangeAdjustment = false;
    int matchMedian = matchSum/inputs.faves.size();

    ratioMedian = static_cast<double>(ratioSum)/static_cast<double>(authorList.size());

    double normalizer = 1./static_cast<double>(authorList.size()-1.);
    double sum = 0;
    for(auto author: authorList)
    {
        if(this->ownProfileId != static_cast<int>(allAuthors[author].id))
            sum+=std::pow(allAuthors[author].ratio - ratioMedian, 2);
    }

    quadraticDeviation = std::sqrt(normalizer * sum);

    qDebug () << "median of match value is: " << matchMedian;
    qDebug () << "median of ratio is: " << ratioMedian;

    //auto keysRatio = inputs.faves.keys();
    auto keysMedian = inputs.faves.keys();
    std::sort(keysMedian.begin(), keysMedian.end(),[&](const int& i1, const int& i2){
        return allAuthors[i1].matches < allAuthors[i2].matches;
    });

    std::sort(authorList.begin(), authorList.end(),[&](const int& i1, const int& i2){
        return allAuthors[i1].ratio < allAuthors[i2].ratio;
    });

    auto ratioMedianIt = std::lower_bound(authorList.cbegin(), authorList.cend(), ratioMedian, [&](const int& i1, const int& ){
        return allAuthors[i1].ratio < ratioMedian;
    });
    auto beginningOfQuadraticToMedianRange = ratioMedianIt - authorList.cbegin();
    qDebug() << "distance to median is: " << beginningOfQuadraticToMedianRange;
    qDebug() << "vector size is: " << authorList.size();

    qDebug() << "sigma: " << quadraticDeviation;
    qDebug() << "2 sigma: " << quadraticDeviation * 2;

    auto ratioSigma2 = std::lower_bound(authorList.cbegin(), authorList.cend(), ratioMedian, [&](const int& i1, const int& ){
        return allAuthors[i1].ratio < (ratioMedian - quadraticDeviation*2);
    });
    endOfUniqueAuthorRange = ratioSigma2 - authorList.cbegin();
    qDebug() << "distance to sigma15 is: " << endOfUniqueAuthorRange;
    if(endOfUniqueAuthorRange == 0)
        needsRangeAdjustment = true;

    // okay, I need to find the limits for deviation that pull a certain % of lists into rarity ranges
    int uncommonAuthorCount = (authorList.size()/100.)*8.;
    int rareAuthorCount = (authorList.size()/100.)*2.5;
    int uniqueAuthorCount = (authorList.size()/100.)*0.5;
    int seenAuthorCount = 0;

    for(auto authorId: authorList){
        seenAuthorCount++;
        if(seenAuthorCount == uncommonAuthorCount){
            auto ratio = allAuthors[authorId].ratio;
            auto currentRange = ratioMedian - ratio;
            uncommonRange = currentRange;
        }else if(seenAuthorCount == rareAuthorCount){
            auto ratio = allAuthors[authorId].ratio;
            auto currentRange = ratioMedian - ratio;
            rareRange = currentRange;
        }else if(seenAuthorCount == uniqueAuthorCount){
            auto ratio = allAuthors[authorId].ratio;
            auto currentRange = ratioMedian - ratio;
            uniqueRange = currentRange;
        }
    }

    for(auto authorId: authorList){
        auto ratio = allAuthors[authorId].ratio;
        if(ratioMedian - ratio > uniqueRange)
            this->uniqueAuthors++;
        else if(ratioMedian - ratio > rareRange)
            this->rareAuthors++;
        else if(ratioMedian - ratio > uncommonRange)
            this->uncommonAuthors++;
    }


    QLOG_INFO() << "outputs from weighting:";
    QLOG_INFO() << "quad:" << quadraticDeviation;
    QLOG_INFO() << "sigma2Dist:" << endOfUniqueAuthorRange;
    QLOG_INFO() << "ratioMedian:" << ratioMedian;

    QLOG_INFO() << "uncommon:" << uncommonRange;
    QLOG_INFO() << "rare:" << rareRange;
    QLOG_INFO() << "unique:" << uniqueRange;

    QLOG_INFO() << "uncommon authors:" << uncommonAuthors;
    QLOG_INFO() << "rare authors:" << rareAuthors;
    QLOG_INFO() << "unique authors:" << uniqueAuthors;
}

AuthorWeightingResult RecCalculatorImplWeighted::CalcWeightingForAuthor(AuthorResult& author, int authorSize, int maximumMatches){
    AuthorWeightingResult result;
    result.isValid = true;

    bool uncommon = uncommonRange <= (ratioMedian - author.ratio);
    bool rare = rareRange <= (ratioMedian - author.ratio);
    bool unique = uniqueRange <= (ratioMedian - author.ratio);

    if(this->ownProfileId == static_cast<int>(author.id))
    {
        result.value = 0;
        result.ownProfile = true;
        result.authorType = AuthorWeightingResult::EAuthorType::common;
        author.authorMatchCloseness = result.authorType;
        return result;
    }

    if(unique)
    {
        result.authorType = AuthorWeightingResult::EAuthorType::unique;
        counter2Sigma++;
        result.value = quadratic_coef(author.ratio,ratioMedian, quadraticDeviation, authorSize, maximumMatches, ECalcType::unique);
    }
    else if(rare)
    {
        result.authorType = AuthorWeightingResult::EAuthorType::rare;
        counter17Sigma++;
        result.value  = quadratic_coef(author.ratio,ratioMedian,  quadraticDeviation,  authorSize, maximumMatches, ECalcType::rare);
    }
    else if(uncommon)
    {
        result.authorType = AuthorWeightingResult::EAuthorType::uncommon;
        result.value  = quadratic_coef(author.ratio, ratioMedian, quadraticDeviation,  authorSize, maximumMatches, ECalcType::uncommon);
    }
    else
    {
        result.authorType = AuthorWeightingResult::EAuthorType::common;
    }
    author.authorMatchCloseness = result.authorType;
    return result;
}

void RecCalculatorImplWeighted::AdjustRatioForAutomaticParams()
{
    if(!params->isAutomatic)
        return;
    ratioSum = 0;
    for(auto author : std::as_const(filteredAuthors))
        ratioSum+=allAuthors[author].ratio;
}

void RecCalculatorImplWeighted::ResetAccumulatedData()
{
    RecCalculatorImplBase::ResetAccumulatedData();
    ratioSum = 0;
    ratioMedian = 0;
    quadraticDeviation = 0;
    endOfUniqueAuthorRange = 0;
    counter2Sigma = 0;
    counter17Sigma = 0;
}

bool RecCalculatorImplWeighted::WeightingIsValid() const
{
    return endOfUniqueAuthorRange >= 4;
}

}
