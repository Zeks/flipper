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
#include "rec_calc/rec_calculator_base.h"

namespace core {
enum class ECalcType{
    common,
    uncommon,
    rare,
    unique
};

double quadratic_coef(double ratio,
                      double median,
                      double sigma,
                      int authorSize,
                      int maximumMatches,
                      ECalcType type);


double sqrt_coef(double ratio, double median, double sigma, int base, int scaler);




class RecCalculatorImplWeighted : public RecCalculatorImplBase{
public:
    RecCalculatorImplWeighted(const RecInputVectors& input): RecCalculatorImplBase(input){}
    virtual FilterListType GetFilterList() override;
    virtual ActionListType GetActionList() override;
    virtual std::function<AuthorWeightingResult(AuthorResult&, int, int)> GetWeightingFunc() override;
    void CalcWeightingParams() override;
    AuthorWeightingResult CalcWeightingForAuthor(AuthorResult& author, int authorSize, int maximumMatches);

    double ratioSum = 0;
    double ratioMedian = 0;
    double quadraticDeviation = 0;

    double uncommonRange = 0;
    double rareRange = 0;
    double uniqueRange = 0;

    int uniqueAuthors = 0;
    int rareAuthors = 0;
    int uncommonAuthors = 0;

    int endOfUniqueAuthorRange = 0;
    int counter2Sigma = 0;
    int counter17Sigma = 0;

    bool needsRangeAdjustment = false;

    void AdjustRatioForAutomaticParams() override;
    void ResetAccumulatedData() override;
    bool WeightingIsValid() const override;
};



}
