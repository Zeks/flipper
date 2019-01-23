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
