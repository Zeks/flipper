/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
#include "include/favholder.h"
#include "include/Interfaces/authors.h"
#include "logger/QsLog.h"
#include "include/timeutils.h"
#include "core/fic_genre_data.h"
#include "threaded_data/threaded_save.h"
#include "threaded_data/threaded_load.h"
#include "rec_calc/rec_calculator_weighted.h"
#include "rec_calc/rec_calculator_mood_adjusted.h"

#include <QSettings>
#include <QDir>
#include <algorithm>
//#include <execution>
#include <cmath>
namespace core{

void RecCalculator::LoadFavourites(QSharedPointer<interfaces::Authors> authorInterface)
{
    CreateTempDataDir();
    QSettings settings("settings/settings_server.ini", QSettings::IniFormat);
    if(settings.value("Settings/usestoreddata", false).toBool() && QFile::exists("ServerData/roafav_0.txt"))
    {
        holder.LoadData<core::rdt_favourites>("ServerData");
    }
    else
    {
        LoadFavouritesDataFromDatabase(authorInterface);
        SaveFavouritesData();
    }
}

void RecCalculator::CreateTempDataDir()
{
    QDir dir(QDir::currentPath());
    dir.mkdir("ServerData");
}

void RecCalculator::LoadFavouritesDataFromDatabase(QSharedPointer<interfaces::Authors> authorInterface)
{
       Q_UNUSED(authorInterface)
    //    auto favourites = authorInterface->LoadFullFavouritesHashset();
    //    for(auto key: favourites.keys())
    //    {
    //        for(auto item : favourites[key])
    //        this->favourites[key].add(item);
    //    }
}

void RecCalculator::LoadStoredFavouritesData()
{
    //thread_boost::LoadFavouritesData("ServerData", favourites);
}

void RecCalculator::SaveFavouritesData()
{
    //thread_boost::SaveFavouritesData("ServerData", favourites);
}




RecommendationListResult RecCalculator::GetMatchedFicsForFavList(QHash<uint32_t, core::FicWeightPtr> fetchedFics,
                                                                 QSharedPointer<RecommendationList> params,
                                                                 genre_stats::GenreMoodData moodData)
{
    QSharedPointer<RecCalculatorImplBase> calculator;
    if(params->useWeighting)
    {
        if(params->useMoodAdjustment)
           calculator.reset(new RecCalculatorImplMoodAdjusted({holder.faves, holder.fics, holder.authorMoodDistributions}, moodData));
        else
           calculator.reset(new RecCalculatorImplWeighted({holder.faves, holder.fics, holder.authorMoodDistributions}));
    }
    else
        calculator.reset(new RecCalculatorImplDefault({holder.faves, holder.fics, holder.authorMoodDistributions}));
    calculator->fetchedFics = fetchedFics;
    calculator->doTrashCounting = params->useDislikes;
    calculator->params = params;
    for(auto fic : params->majorNegativeVotes)
        calculator->ownMajorNegatives.add(static_cast<uint32_t>(fic));
    QLOG_INFO() << "Received negative votes: " << params->majorNegativeVotes.size();
    TimedAction action("Reclist Creation",[&](){
        calculator->Calc();
    });
    action.run();
    calculator->result.authors = calculator->filteredAuthors;

    return calculator->result;
}

DiagnosticRecommendationListResult RecCalculator::GetDiagnosticRecommendationList(QHash<uint32_t, FicWeightPtr> fetchedFics, QSharedPointer<RecommendationList> params, genre_stats::GenreMoodData moodData)
{
    DiagnosticRecommendationListResult result;

    QSharedPointer<RecCalculatorImplWeighted> actualCalculator(new RecCalculatorImplMoodAdjusted({holder.faves, holder.fics, holder.authorMoodDistributions}, moodData));
    actualCalculator->fetchedFics = fetchedFics;
    actualCalculator->params = params;
    actualCalculator->needsDiagnosticData = true;

    for(auto fic : params->majorNegativeVotes)
        actualCalculator->ownMajorNegatives.add(static_cast<uint32_t>(fic));

    TimedAction action("Reclist Creation",[&](){
        actualCalculator->Calc();
        QLOG_INFO() << "Param calc finished";
    });
    action.run();
    actualCalculator->result.authors = actualCalculator->filteredAuthors;
    result.recs = actualCalculator->result;
    result.quad = actualCalculator->quad;
    result.ratioMedian = actualCalculator->ratioMedian;
    result.sigma2Dist = actualCalculator->sigma2Dist;
    //result.authorData = actualCalculator->
    for(auto author : actualCalculator->filteredAuthors){
        result.authorData.push_back(actualCalculator->allAuthors[author]);
    }
    result.authorsForFics = actualCalculator->authorsForFics;

    return result;
}

MatchedFics RecCalculator::GetMatchedFics(UserMatchesInput input, int user2)
{
    QLOG_INFO() << "Creating calculator";
    QSharedPointer<RecCalculatorImplWeighted> calculator;
    calculator.reset(new RecCalculatorImplWeighted({holder.faves, holder.fics, holder.authorMoodDistributions}));
    //calculator->fetchedFics = fetchedFics;
    QSharedPointer<RecommendationList> params(new RecommendationList);
    for(auto ignore: input.userIgnoredFandoms)
        params->ignoredFandoms.insert(ignore);
    calculator->params = params;

    auto ignores = calculator->BuildIgnoreList();
    QLOG_INFO() << "Making & list";
    Roaring ignoredTemp = holder.faves[user2];
    ignoredTemp = ignoredTemp & ignores;
    QLOG_INFO() << "Checking cardinality";
    auto unignoredSize = holder.faves[user2].xor_cardinality(ignoredTemp);


    MatchedFics result;
    QLOG_INFO() << "Blargh";
    Roaring temp = input.userFavourites;
    temp = temp & holder.faves[user2];
    for(auto fic : temp)
        result.matches.push_back(fic);
    result.ratioWithoutIgnores = static_cast<float>(holder.faves[user2].cardinality())/static_cast<float>(temp.cardinality());
    result.ratio = static_cast<float>(unignoredSize)/static_cast<float>(temp.cardinality());
    return result;
}

}
