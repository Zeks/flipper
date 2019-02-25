#pragma once
#include "rec_calc/rec_calculator_weighted.h"
#include "data_code/data_holders.h"
#include "data_code/rec_calc_data.h"

namespace core {

class RecCalculatorImplMoodAdjusted: public RecCalculatorImplWeighted{
public:
    RecCalculatorImplMoodAdjusted(RecInputVectors input): RecCalculatorImplWeighted(input){}
};


}
