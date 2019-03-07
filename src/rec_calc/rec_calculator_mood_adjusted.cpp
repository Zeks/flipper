#include "include/rec_calc/rec_calculator_mood_adjusted.h"
#include "include/Interfaces/genres.h"
#include <QStringList>

namespace core {

RecCalculatorImplMoodAdjusted::RecCalculatorImplMoodAdjusted(RecInputVectors input, genre_stats::GenreMoodData moodData):
    RecCalculatorImplWeighted(input), moodData(moodData)
{
    QStringList moodList;
    moodList << "Neutral" << "Funny"  << "Shocky" << "Flirty" << "Dramatic" << "Hurty" << "Bondy";
    double neutralDiffernce = 0., touchyDifference = 0.;
    for(auto authorKey: input.moods.keys())
    {
        auto authorData = input.moods[authorKey];
        for(int i= 0; i < moodList.size(); i++)
        {
            auto userValue =  interfaces::Genres::ReadMoodValue(moodList[i], moodData.listMoodData);
            auto authorValue =  interfaces::Genres::ReadMoodValue(moodList[i], authorData);
            if(i > 1)
                touchyDifference += std::max(userValue, authorValue) - std::min(userValue, authorValue);
            neutralDiffernce += std::max(userValue, authorValue) - std::min(userValue, authorValue);
        }
        moodDiffs[authorKey].neutralDifference =  neutralDiffernce;
        moodDiffs[authorKey].touchyDifference =  touchyDifference;
    }
    votesBase = 100;
}

std::optional<double> RecCalculatorImplMoodAdjusted::GetNeutralDiffForLists(uint32_t author)
{
    auto it = moodDiffs.find(author);
    if(it == moodDiffs.end())
        return {};

    return it.value().neutralDifference;
}

std::optional<double> RecCalculatorImplMoodAdjusted::GetTouchyDiffForLists(uint32_t author)
{
    auto it = moodDiffs.find(author);
    if(it == moodDiffs.end())
        return {};

    return it.value().touchyDifference;
}

}
