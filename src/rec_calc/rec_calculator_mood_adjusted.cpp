#include "include/rec_calc/rec_calculator_mood_adjusted.h"
#include "include/Interfaces/genres.h"
#include <QStringList>

namespace core {


static auto ratioFilterMoodAdjusted = [](AuthorResult& author, QSharedPointer<RecommendationList> params)
{
    bool firstPass =  author.ratio <= params->pickRatio && author.matches > 0;

    if(!firstPass)
        return false;

    auto cleanRatio = author.matches != 0 ? static_cast<double>(author.size)/static_cast<double>(author.matches) : 999999;
    if(author.listDiff.touchyDifference.has_value())
    {
        auto authorcoef = author.listDiff.touchyDifference.value();
        if((cleanRatio > params->pickRatio) && authorcoef  >= 0.4)
        {
            qDebug() << "skipping author: " << author.id << "with coef: "  << authorcoef  << " and ratio: " <<  cleanRatio;
            author.ratio = 999999;
        }
    }
    bool secondPass = author.ratio <= params->pickRatio && author.matches > 0;;
    return secondPass;
};

RecCalculatorImplWeighted::FilterListType RecCalculatorImplMoodAdjusted::GetFilterList(){
    return {matchesFilter, ratioFilterMoodAdjusted};
}

RecCalculatorImplMoodAdjusted::RecCalculatorImplMoodAdjusted(RecInputVectors input, genre_stats::GenreMoodData moodData):
    RecCalculatorImplWeighted(input), moodData(moodData)
{
    QStringList moodList;
    moodList << "Neutral" << "Funny"  << "Shocky" << "Flirty" << "Dramatic" << "Hurty" << "Bondy";

    for(auto authorKey: input.moods.keys())
    {
        double neutralDifference = 0., touchyDifference = 0.;
        auto authorData = input.moods[authorKey];
        for(int i= 0; i < moodList.size(); i++)
        {
            auto userValue =  interfaces::Genres::ReadMoodValue(moodList[i], moodData.listMoodData);
            auto authorValue =  interfaces::Genres::ReadMoodValue(moodList[i], authorData);
            if(i > 1)
                touchyDifference += std::max(userValue, authorValue) - std::min(userValue, authorValue);
            neutralDifference += std::max(userValue, authorValue) - std::min(userValue, authorValue);
        }
        if(authorKey == 77257)
        {
            qDebug() << "Logging user mood list";
            moodData.listMoodData.Log();
            qDebug() << "Logging author mood list";
            authorData.Log();
            qDebug() << "between author: " << authorKey << " and user, neutral: " << neutralDifference;
            qDebug() << "between author: " << authorKey << " and user, touchy: " << touchyDifference;
        }
        moodDiffs[authorKey].neutralDifference =  neutralDifference;
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

//    if(author == 77257)
//        qDebug() << "reading touchy coef: " << it.value().touchyDifference;
    return it.value().touchyDifference;
}

}
