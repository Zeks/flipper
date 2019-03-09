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
#include "rec_calc/rec_calculator_base.h"

namespace core {
enum class ECalcType{
    common,
    uncommon,
    near,
    close
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
    RecCalculatorImplWeighted(RecInputVectors input): RecCalculatorImplBase(input){}
    virtual FilterListType GetFilterList();
    virtual ActionListType GetActionList();
    virtual std::function<AuthorWeightingResult(AuthorResult&, int, int)> GetWeightingFunc();
    void CalcWeightingParams();
    AuthorWeightingResult CalcWeightingForAuthor(AuthorResult& author, int authorSize, int maximumMatches);

    double ratioSum = 0;
    double ratioMedian = 0;
    double quad = 0;
    int sigma2Dist = 0;
    int counter2Sigma = 0;
    int counter17Sigma = 0;


};



}