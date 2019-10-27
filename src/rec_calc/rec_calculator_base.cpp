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
#include "include/rec_calc/rec_calculator_base.h"
#include "timeutils.h"

namespace core{
void RecCalculatorImplBase::Calc(){
    auto filters = GetFilterList();
    auto actions = GetActionList();

    TimedAction relations("Fetching relations",[&](){
        FetchAuthorRelations();
    });
    relations.run();
    TimedAction filtering("Filtering data",[&](){
        Filter(filters, actions);
    });
    filtering.run();

    TimedAction resultAdjustment("adjusting results",[&](){
        AutoAdjustRecommendationParamsAndFilter();
        AdjustRatioForAutomaticParams();
    });
    resultAdjustment.run();

    TimedAction weighting("weighting",[&](){
        CalcWeightingParams();
    });
    weighting.run();

    // not very satisfied with the results
    //    TimedAction authorMatchQuality("Fetching match qualities for authors",[&](){
    //        CollectFicMatchQuality();
    //    });
    //    authorMatchQuality.run();


    TimedAction collecting("collecting votes ",[&](){
        CollectVotes();
    });
    collecting.run();




    TimedAction report("writing match report",[&](){
        for(auto& author: filteredAuthors)
            result.matchReport[allAuthors[author].matches]++;
    });

    report.run();
}

double GetCoeffForTouchyDiff(double diff, bool useScaleDown = true)
{

    if(diff <= 0.1)
        return 1.2;
    if(diff <= 0.18)
        return 1.1;
    if(diff <= 0.35)
        return 1;
    if(useScaleDown)
    {
        if(diff <= 0.45)
            return 0.8;
        return 0.5;
    }
    else {
        return 1;
    }
}



void RecCalculatorImplBase::CollectVotes()
{
    auto weightingFunc = GetWeightingFunc();
    auto authorSize = filteredAuthors.size();
    qDebug() << "Max Matches:" <<  prevMaximumMatches;
    std::for_each(filteredAuthors.begin(), filteredAuthors.end(), [this](int author){
        for(auto fic: inputs.faves[author])
        {
            result.recommendations[fic]+= 1;
        }
    });
    int maxValue = 0;
    int maxId = -1;

    auto it = result.recommendations.begin();
    while(it != result.recommendations.end())
    {
        if(it.value() > maxValue )
        {
            maxValue = it.value();
            maxId = it.key();
        }
        result.pureMatches[it.key()] = it.value();
        it++;
    }
    qDebug() << "Max pure votes: " << maxValue;
    qDebug() << "Max id: " << maxId;
    result.recommendations.clear();

    std::for_each(filteredAuthors.begin(), filteredAuthors.end(), [maxValue,weightingFunc, authorSize, this](int author){
        for(auto fic: inputs.faves[author])
        {
            auto weighting = weightingFunc(allAuthors[author],authorSize, maxValue );
            double matchCountSimilarityCoef = weighting.GetCoefficient();

            double vote = votesBase;

            //std::optional<double> neutralMoodSimilarity = GetNeutralDiffForLists(author);

            std::optional<double> touchyMoodSimilarity = GetTouchyDiffForLists(author);

            if(touchyMoodSimilarity.has_value())
            {
                double coef  = 1;
                if(weighting.authorType == core::AuthorWeightingResult::EAuthorType::rare ||
                        weighting.authorType == core::AuthorWeightingResult::EAuthorType::unique)
                    coef = GetCoeffForTouchyDiff(touchyMoodSimilarity.value(), false);
                else
                    coef = GetCoeffForTouchyDiff(touchyMoodSimilarity.value());

                if(coef > 0.99)
                    result.decentMatches[fic] = 1;
                //                if(author == 77257)
                //                    qDebug() << "similarity coef for author: " << 77257 << "is: " << coef;
                vote = votesBase*coef;
            }
            vote = vote * matchCountSimilarityCoef;
            result.recommendations[fic]+= vote;
            result.AddToBreakdown(fic, weighting.authorType, weighting.GetCoefficient());
        }
    });
}

void RecCalculatorImplBase::AutoAdjustRecommendationParamsAndFilter()
{
    // if the options were supplied manually, we just return what we got
    if(!params->isAutomatic)
        return;
    // else we try to adjust minimum mathes so that a set of conditionsa re met
    // 1) for the average of 3 numbers we take a differnece between it and the middle one
    //    divide by the middle one and check to see if it's >0.05
    // 2) only apply this if average of 3 is > 70


    // first we need to prepare the dataset
    QMap<int, QList<int>> authorsByMatches;
    for(auto author : filteredAuthors)
        authorsByMatches[allAuthors[author].matches].push_back(allAuthors[author].id);
    QLOG_INFO() << "authorsByMatches: " << authorsByMatches;
    // then we go through matches calculating averages and checking that conditions are satisfied

    // this will tell us where cutoff happened
    // if it's 2, then we need to take a look if we need to _relax_ settings, not tighten them
    int stoppingIndex = -1;
    QList<int> matches = authorsByMatches.keys();
    QLOG_INFO() << "Keys used: " << matches;
    QLOG_INFO() << "matches count is: " << matches.count();
    if(matches.size() > 3)
    {
        for(int i = 0; i < matches.count() - 2; i ++){
            QLOG_INFO() << "starting processing of: " << i;
            int firstCount = authorsByMatches[matches[i]].count();
            int secondCount = authorsByMatches[matches[i+1]].count();
            int thirdCount = authorsByMatches[matches[i+2]].count();
            QLOG_INFO() << firstCount << secondCount << thirdCount;
            double average = static_cast<double>(firstCount + secondCount + thirdCount)/3.;
            //QLOG_INFO() << "average is: " << average;
            if(i == 0 && average < 70)
            {
                stoppingIndex = 0;
                break;
            }
            double relative = std::abs((static_cast<double>(secondCount) - average)/static_cast<double>(secondCount));
            QLOG_INFO() << "relative is: " << relative;
            //QLOG_INFO() << " first condition: " << (thirdCount - firstCount)/2. << " second condition: " <<  secondCount/5.;
            if(relative > 0.05 && (std::abs(firstCount - thirdCount)) < 30 && stoppingIndex < 0)
            {
                QLOG_INFO() << "relative bigger than threshhold, stopping: " << relative;
                stoppingIndex = i;

            }
        }
    }
    stoppingIndex = stoppingIndex < 0 ? 0 : stoppingIndex;
    QLOG_INFO() << "Adjustment stopped at: " << stoppingIndex;
    //QHash<int, AuthorResult> result;
    QList<int> result;
    params->minimumMatch = matches[stoppingIndex];
    for(auto matchCount : matches){
        if(matchCount < params->minimumMatch)
        {
            //QLOG_INFO() << "discarding match count: " << matchCount;
            continue;
        }
        //QLOG_INFO() << "adding authors for match count: " << matchCount << " amount:" << authorsByMatches[matchCount].size();
        result += authorsByMatches[matchCount];

    }
    filteredAuthors = result;
}

void RecCalculatorImplBase::AdjustRatioForAutomaticParams()
{
    // intentionally does nothing
}

Roaring RecCalculatorImplBase::BuildIgnoreList()
{
    Roaring ignores;
    //QLOG_INFO() << "fandom ignore list is of size: " << params->ignoredFandoms.size();
    QLOG_INFO() << "Building ignore list";
    TimedAction ignoresCreation("Building ignores",[&](){
        for(auto fic: inputs.fics)
        {
            if(!fic)
                continue;

            int count = 0;
            bool inIgnored = false;
            for(auto fandom: fic->fandoms)
            {
                if(fandom != -1)
                    count++;
                if(params->ignoredFandoms.contains(fandom) && fandom > 1)
                    inIgnored = true;
            }
            if(/*count == 1 && */inIgnored)
                ignores.add(fic->id);

        }
    });
    ignoresCreation.run();
    QLOG_INFO() << "fanfic ignore list is of size: " << ignores.cardinality();
    return ignores;
}

void RecCalculatorImplBase::FetchAuthorRelations()
{
    qDebug() << "faves is of size: " << inputs.faves.size();
    allAuthors.reserve(inputs.faves.size());
    Roaring ignores;
    //QLOG_INFO() << "fandom ignore list is of size: " << params->ignoredFandoms.size();
    TimedAction ignoresCreation("Building ignores",[&](){
        for(auto fic: inputs.fics)
        {
            int count = 0;
            bool inIgnored = false;
            for(auto fandom: fic->fandoms)
            {
                if(fandom != -1)
                    count++;
                if(params->ignoredFandoms.contains(fandom) && fandom > 1)
                    inIgnored = true;
            }
            if(/*count == 1 && */inIgnored)
                ignores.add(fic->id);

        }
    });
    ignoresCreation.run();
    QLOG_INFO() << "fanfic ignore list is of size: " << ignores.cardinality();


    auto sourceFics = QSet<uint32_t>::fromList(fetchedFics.keys());

    for(auto bit : sourceFics)
        ownFavourites.add(bit);
    qDebug() << "finished creating roaring";
    int minMatches;
    minMatches =  params->minimumMatch;
    maximumMatches = minMatches;
    TimedAction action("Relations Creation",[&](){
        auto it = inputs.faves.begin();
        while (it != inputs.faves.end())
        {
            if(params->userFFNId == it.key())
            {
                QLOG_INFO() << "Skipping user's own list: " << params->userFFNId ;
                it++;
                continue;
            }

            auto& author = allAuthors[it.key()];
            author.id = it.key();
            author.matches = 0;
            //QLOG_INFO
            //these are the fics from the current fav list that are ignored

            //bool hasIgnoredMatches = false;
            Roaring ignoredTemp = it.value();
            ignoredTemp = ignoredTemp & ignores;

            if(ignoredTemp.cardinality() > 0)
            {
                //hasIgnoredMatches = true;
                //                QLOG_INFO() << "ficl list size is: " << it.value().cardinality();
                //                QLOG_INFO() << "of those ignored are: " << ignoredTemp.cardinality();
            }
            author.size = it.value().cardinality();
            Roaring temp = ownFavourites;
            // first we need to remove ignored fics
            auto unignoredSize = it.value().xor_cardinality(ignoredTemp);
            //            if(hasIgnoredMatches)
            //                QLOG_INFO() << "this leaves unignored: " << unignoredSize;
            temp = temp & it.value();
            author.matches = temp.cardinality();
            author.sizeAfterIgnore = unignoredSize;
            if(ignores.cardinality() == 0)
                author.sizeAfterIgnore = author.size;
            if(maximumMatches < author.matches)
            {
                prevMaximumMatches = maximumMatches;
                maximumMatches = author.matches;
            }
            matchSum+=author.matches;
            it++;
        }
    });
    action.run();
}

void RecCalculatorImplBase::CollectFicMatchQuality()
{
    QVector<int> tempAuthors;

    for(auto authorId : filteredAuthors)
    {
        tempAuthors.push_back(authorId);
        auto& author = allAuthors[authorId];
        auto& authorFaves = inputs.faves[authorId];
        Roaring temp = ownFavourites;
        auto matches = temp & authorFaves;
        for(auto value : matches)
        {
            auto itFic = inputs.fics.find(value);
            if(itFic == inputs.fics.end())
                continue;

            // < 500 1 votes
            // < 300 4 votes
            // < 100 8 votes
            // < 50 15 votes

            author.breakdown.total++;
            if(itFic->get()->favCount <= 50)
            {
                author.breakdown.below50++;
                author.breakdown.votes+=15;
                if(itFic->get()->authorId != -1)
                    author.breakdown.authors.insert(itFic->get()->authorId);

            }
            else if(itFic->get()->favCount <= 100)
            {
                author.breakdown.below100++;
                author.breakdown.votes+=8;
                if(itFic->get()->authorId != -1)
                    author.breakdown.authors.insert(itFic->get()->authorId);
            }
            else if(itFic->get()->favCount <= 300)
            {
                author.breakdown.below300++;
                author.breakdown.votes+=4;
                if(itFic->get()->authorId != -1)
                    author.breakdown.authors.insert(itFic->get()->authorId);
            }
            else if(itFic->get()->favCount <= 500)
            {
                author.breakdown.below500++;
                author.breakdown.votes+=1;
            }
        }
        author.breakdown.priority1 = (author.breakdown.below50 + author.breakdown.below100) >= 3;
        author.breakdown.priority1 = author.breakdown.priority1 && author.breakdown.authors.size() > 1;

        author.breakdown.priority2 = (author.breakdown.below50 + author.breakdown.below100 + author.breakdown.below300) >= 5;
        author.breakdown.priority2 = author.breakdown.priority1 || (author.breakdown.priority2 && author.breakdown.authors.size() > 1);

        author.breakdown.priority3 = (author.breakdown.below50 + author.breakdown.below100 + author.breakdown.below300 + author.breakdown.below500) >= 8;
        author.breakdown.priority3 = (author.breakdown.priority1 || author.breakdown.priority2) || (author.breakdown.priority3 && author.breakdown.authors.size() > 1);
    }
    std::sort(tempAuthors.begin(), tempAuthors.end(), [&](int id1, int id2){

        bool largerScore = allAuthors[id1].breakdown.votes > allAuthors[id2].breakdown.votes;
        bool doubleBelow100First =  (allAuthors[id1].breakdown.below50 + allAuthors[id1].breakdown.below100) >= 2;
        bool doubleBelow100Second =  (allAuthors[id2].breakdown.below50 + allAuthors[id2].breakdown.below100) >= 2;
        if(doubleBelow100First && !doubleBelow100Second)
            return true;
        if(!doubleBelow100First && doubleBelow100Second)
            return false;
        return largerScore;
    });

    QLOG_INFO() << "Displaying 100 most closesly matched authors";
    auto justified = [](int value, int padding) {
        return QString::number(value).leftJustified(padding, ' ');
    };
    QLOG_INFO() << "Displaying priority lists: ";
    QLOG_INFO() << "//////////////////////";
    QLOG_INFO() << "P1: ";
    QLOG_INFO() << "//////////////////////";
    for(int i = 0; i < 100; i++)
    {
        if(allAuthors[tempAuthors[i]].breakdown.priority1)
            QLOG_INFO() << "Author id: " << justified(tempAuthors[i],9) << " votes: " <<  justified(allAuthors[tempAuthors[i]].breakdown.votes,5) <<
                                                                                                                                                     "Ratio: " << justified(allAuthors[tempAuthors[i]].ratio, 3) <<
                                                                                                                                                                                                                    "below 50: " << justified(allAuthors[tempAuthors[i]].breakdown.below50, 5) <<
                                                                                                                                                                                                                                                                                                  "below 100: " << justified(allAuthors[tempAuthors[i]].breakdown.below100, 5) <<
                                                                                                                                                                                                                                                                                                                                                                                  "below 300: " << justified(allAuthors[tempAuthors[i]].breakdown.below300, 5) <<
                                                                                                                                                                                                                                                                                                                                                                                                                                                                  "below 500: " << justified(allAuthors[tempAuthors[i]].breakdown.below500, 5);
    }
    QLOG_INFO() << "//////////////////////";
    QLOG_INFO() << "P2: ";
    QLOG_INFO() << "//////////////////////";
    for(int i = 0; i < 100; i++)
    {
        if(allAuthors[tempAuthors[i]].breakdown.priority2)
            QLOG_INFO() << "Author id: " << justified(tempAuthors[i],9) << " votes: " <<  justified(allAuthors[tempAuthors[i]].breakdown.votes,5) <<
                                                                                                                                                     "Ratio: " << justified(allAuthors[tempAuthors[i]].ratio, 3) <<
                                                                                                                                                                                                                    "below 50: " << justified(allAuthors[tempAuthors[i]].breakdown.below50, 5) <<
                                                                                                                                                                                                                                                                                                  "below 100: " << justified(allAuthors[tempAuthors[i]].breakdown.below100, 5) <<
                                                                                                                                                                                                                                                                                                                                                                                  "below 300: " << justified(allAuthors[tempAuthors[i]].breakdown.below300, 5) <<
                                                                                                                                                                                                                                                                                                                                                                                                                                                                  "below 500: " << justified(allAuthors[tempAuthors[i]].breakdown.below500, 5);
    }
    QLOG_INFO() << "//////////////////////";
    QLOG_INFO() << "P3: ";
    QLOG_INFO() << "//////////////////////";
    for(int i = 0; i < 100; i++)
    {
        if(allAuthors[tempAuthors[i]].breakdown.priority3)
            QLOG_INFO() << "Author id: " << justified(tempAuthors[i],9) << " votes: " <<  justified(allAuthors[tempAuthors[i]].breakdown.votes,5) <<
                                                                                                                                                     "Ratio: " << justified(allAuthors[tempAuthors[i]].ratio, 3) <<
                                                                                                                                                                                                                    "below 50: " << justified(allAuthors[tempAuthors[i]].breakdown.below50, 5) <<
                                                                                                                                                                                                                                                                                                  "below 100: " << justified(allAuthors[tempAuthors[i]].breakdown.below100, 5) <<
                                                                                                                                                                                                                                                                                                                                                                                  "below 300: " << justified(allAuthors[tempAuthors[i]].breakdown.below300, 5) <<
                                                                                                                                                                                                                                                                                                                                                                                                                                                                  "below 500: " << justified(allAuthors[tempAuthors[i]].breakdown.below500, 5);
    }

}

void RecCalculatorImplBase::Filter(QList<std::function<bool (AuthorResult &, QSharedPointer<RecommendationList>)> > filters,
                                   QList<std::function<void (RecCalculatorImplBase *, AuthorResult &)> > actions)
{
    auto params = this->params;
    auto thisPtr = this;

    std::for_each(allAuthors.begin(), allAuthors.end(), [this, filters, actions, params,thisPtr](AuthorResult& author){
        author.ratio = author.matches != 0 ? static_cast<double>(author.sizeAfterIgnore)/static_cast<double>(author.matches) : 999999;
        author.listDiff.touchyDifference = GetTouchyDiffForLists(author.id);
        bool fail = std::any_of(filters.begin(), filters.end(), [&](decltype(filters)::value_type filter){
                return filter(author, params) == 0;
    });
        if(fail)
            return;
        std::for_each(actions.begin(), actions.end(), [thisPtr, &author](decltype(actions)::value_type action){
            action(thisPtr, author);
        });

    });
}




}
