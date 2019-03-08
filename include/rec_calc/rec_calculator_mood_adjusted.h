#pragma once
#include "rec_calc/rec_calculator_weighted.h"
#include "data_code/data_holders.h"
#include "data_code/rec_calc_data.h"
#include <array>

namespace core {

class RecCalculatorImplMoodAdjusted: public RecCalculatorImplWeighted{
public:
    RecCalculatorImplMoodAdjusted(RecInputVectors input, genre_stats::GenreMoodData moodData);
    std::optional<double> GetNeutralDiffForLists(uint32_t);
    std::optional<double> GetTouchyDiffForLists(uint32_t);
    virtual FilterListType GetFilterList();

    QHash<uint32_t, ListMoodDifference> moodDiffs;
    genre_stats::GenreMoodData moodData;

};


}
