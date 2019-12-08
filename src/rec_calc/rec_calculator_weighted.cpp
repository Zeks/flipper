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
#include "include/rec_calc/rec_calculator_weighted.h"
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
    case ECalcType::close:
        //qDebug() << "casting max vote: " << "matches: " << maximumMatches << " value: " << 0.2*static_cast<double>(maximumMatches);
        result =  0.2*static_cast<double>(maximumMatches);

        break;
    case ECalcType::near:

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
        if(calc->ownProfileId != author.id)
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
    QLOG_INFO() << "inputs to weighting:";
    QLOG_INFO() << "matchsum:" << matchSum;
    QLOG_INFO() << "inputs.faves.size():" << inputs.faves.size();
    QLOG_INFO() << "ratioSum:" << ratioSum;
    QLOG_INFO() << "filteredAuthors.size():" << filteredAuthors.size();


    int matchMedian = matchSum/inputs.faves.size();

    ratioMedian = static_cast<double>(ratioSum)/static_cast<double>(filteredAuthors.size());

    double normalizer = 1./static_cast<double>(filteredAuthors.size()-1.);
    double sum = 0;
    for(auto author: filteredAuthors)
    {
        sum+=std::pow(allAuthors[author].ratio - ratioMedian, 2);
    }
    quad = std::sqrt(normalizer * sum);


    qDebug () << "median value is: " << matchMedian;
    qDebug () << "median ratio is: " << ratioMedian;

    auto keysRatio = inputs.faves.keys();
    auto keysMedian = inputs.faves.keys();
    std::sort(keysMedian.begin(), keysMedian.end(),[&](const int& i1, const int& i2){
        return allAuthors[i1].matches < allAuthors[i2].matches;
    });

    std::sort(filteredAuthors.begin(), filteredAuthors.end(),[&](const int& i1, const int& i2){
        return allAuthors[i1].ratio < allAuthors[i2].ratio;
    });

    auto ratioMedianIt = std::lower_bound(filteredAuthors.begin(), filteredAuthors.end(), ratioMedian, [&](const int& i1, const int& ){
        return allAuthors[i1].ratio < ratioMedian;
    });
    auto dist = ratioMedianIt - filteredAuthors.begin();
    qDebug() << "distance to median is: " << dist;
    qDebug() << "vector size is: " << filteredAuthors.size();

    qDebug() << "sigma: " << quad;
    qDebug() << "2 sigma: " << quad * 2;

    auto ratioSigma2 = std::lower_bound(filteredAuthors.begin(), filteredAuthors.end(), ratioMedian, [&](const int& i1, const int& ){
        return allAuthors[i1].ratio < (ratioMedian - quad*2);
    });
    sigma2Dist = ratioSigma2 - filteredAuthors.begin();
    qDebug() << "distance to sigma15 is: " << sigma2Dist;

    QLOG_INFO() << "outputs from weighting:";
    QLOG_INFO() << "quad:" << quad;
    QLOG_INFO() << "sigma2Dist:" << sigma2Dist;
    QLOG_INFO() << "ratioMedian:" << ratioMedian;
}

AuthorWeightingResult RecCalculatorImplWeighted::CalcWeightingForAuthor(AuthorResult& author, int authorSize, int maximumMatches){
    AuthorWeightingResult result;
    result.isValid = true;
    bool gtSigma = (ratioMedian - quad) >= author.ratio;
    bool gtSigma2 = (ratioMedian - 2 * quad) >= author.ratio;
    bool gtSigma17 = (ratioMedian - 1.7 * quad) >= author.ratio;

    if(gtSigma2)
    {

        result.authorType = AuthorWeightingResult::EAuthorType::unique;
        counter2Sigma++;
        //result.value = quadratic_coef(author.ratio,ratioMedian, quad, authorSize/10, authorSize/20, authorSize);
        result.value = quadratic_coef(author.ratio,ratioMedian, quad, authorSize, maximumMatches, ECalcType::close);
    }
    else if(gtSigma17)
    {
        result.authorType = AuthorWeightingResult::EAuthorType::rare;
        counter17Sigma++;
        //result.value  = quadratic_coef(author.ratio,ratioMedian,  quad,authorSize/20, authorSize/40, authorSize);
        result.value  = quadratic_coef(author.ratio,ratioMedian,  quad,  authorSize, maximumMatches, ECalcType::near);
    }
    else if(gtSigma)
    {
        result.authorType = AuthorWeightingResult::EAuthorType::uncommon;
        result.value  = quadratic_coef(author.ratio, ratioMedian, quad,  authorSize, maximumMatches, ECalcType::uncommon);
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
    for(auto author : filteredAuthors)
        ratioSum+=allAuthors[author].ratio;
}

}
