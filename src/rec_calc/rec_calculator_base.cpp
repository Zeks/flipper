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
#include "third_party/nanobench/nanobench.h"
#include <QFuture>
#include <QtConcurrent>
#include <execution>

namespace core{
void RecCalculatorImplBase::ResetAccumulatedData()
{
    matchSum = 0;
    filteredAuthors.clear();
}

auto threadedIntListProcessor = [](QString taskName, int threadsToUse, QList<int> list, auto worker, auto resultingDataProcessor){
    QVector<std::pair<QList<int>::const_iterator,QList<int>::const_iterator>> iterators;
    int chunkSize = list.size()/threadsToUse;
    int listSize = list.size();
    int  i = 0;
    iterators.reserve(threadsToUse);
    while(i*chunkSize < listSize)
    {
        QList<int>::const_iterator begin = list.cbegin();
        QList<int>::const_iterator end = list.cbegin();
        std::advance(begin, i*chunkSize);
        int rightBorder = i*chunkSize + chunkSize;
        if(rightBorder < list.size())
            std::advance(end, rightBorder);
        else
            end = list.cend();
        iterators.push_back({begin, end});
        i++;
    }
    typedef std::pair<QList<int>::const_iterator,QList<int>::const_iterator> PairType;
    TimedAction task(taskName,[&](){
        QVector<QFuture<decltype(worker(std::declval<PairType>()))>> futures;
        for(int i = 0; i < iterators.size(); i++)
            futures.push_back(QtConcurrent::run(std::bind(worker,iterators.at(i))));
        for(auto future: futures)
            future.waitForFinished();
        for(const auto& future: futures)
            resultingDataProcessor(future.result());
    });
    task.run();
};

auto threadedIntListTupleProcessor = [](QString taskName, int threadsToUse, QList<int> list, auto worker, auto resultingDataProcessor){
    QVector<std::tuple<QList<int>::const_iterator,QList<int>::const_iterator,QList<int>::const_iterator>> iterators;
    int chunkSize = list.size()/threadsToUse;
    int listSize = list.size();
    int  i = 0;
    iterators.reserve(threadsToUse);
    while(i*chunkSize < listSize)
    {
        QList<int>::const_iterator begin = list.cbegin();
        QList<int>::const_iterator end = list.cbegin();
        std::advance(begin, i*chunkSize);
        int rightBorder = i*chunkSize + chunkSize;
        if(rightBorder < list.size())
            std::advance(end, rightBorder);
        else
            end = list.cend();
        iterators.push_back({list.begin(),begin, end});
        i++;
    }
    typedef std::tuple<QList<int>::const_iterator,QList<int>::const_iterator,QList<int>::const_iterator> TupleType;
    TimedAction task(taskName,[&](){
        QVector<QFuture<decltype(worker(std::declval<TupleType>()))>> futures;
        for(int i = 0; i < iterators.size(); i++)
            futures.push_back(QtConcurrent::run(std::bind(worker,iterators.at(i))));
        for(auto future: futures)
            future.waitForFinished();
        for(const auto& future: futures)
            resultingDataProcessor(future.result());
    });
    task.run();
};

bool RecCalculatorImplBase::Calc(){
    auto filters = GetFilterList();
    auto actions = GetActionList();
    //TimedAction relations("Fetching relations",[&](){
        //ankerl::nanobench::Bench().minEpochIterations(2).run(
                    //[&](){
        FetchAuthorRelations();
        //});
    //});
    //relations.run();
    params->ratioCutoff = ratioCutoff;
    RunMatchingAndWeighting(params, filters, actions);
    QLOG_INFO() << "filtered authors after default pass:" << filteredAuthors.size();

    CalculateNegativeToPositiveRatio();
    bool succesfullyGotVotes = false;
    TimedAction collecting("collecting votes ",[&](){
        succesfullyGotVotes = CollectVotes();
    });
    collecting.run();
    if(!succesfullyGotVotes)
        return false;

    TimedAction report("writing match report",[&](){
        for(auto& author: std::as_const(filteredAuthors))
            if(author != ownProfileId)
                result.matchReport[allAuthors[author].matches]++;
    });

    report.run();
    //ReportNegativeResults();
    if(needsDiagnosticData)
        FillFilteredAuthorsForFics();
    return true;
}

void RecCalculatorImplBase::RunMatchingAndWeighting(QSharedPointer<RecommendationList> params, const FilterListType& filters, const ActionListType& actions)
{
    int  i = 0;
    AutoAdjustmentAndFilteringResult firstAdjustmentResult;
    do
    {
        ResetAccumulatedData();
        TimedAction filtering("Filtering data",[&](){
            Filter(params, filters, actions);
        });
        filtering.run();

        TimedAction weighting("weighting",[&](){
            CalcWeightingParams();
        });
        weighting.run();
        if(!params->isAutomatic && !params->adjusting)
            break;
        i++;
    }while(params->adjusting);
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



bool RecCalculatorImplBase::CollectVotes()
{
    auto weightingFunc = GetWeightingFunc();
    auto authorSize = filteredAuthors.size();
    if(filteredAuthors.size() == 0)
        return false;
    qDebug() << "Max Matches:" <<  prevMaximumMatches;
    std::for_each(filteredAuthors.cbegin(), filteredAuthors.cend(), [this](int author){
        for(auto fic: inputs.faves[author])
        {
            result.recommendations[fic]+= 1;
        }
    });
    int maxValue = 0;
    int maxId = -1;

    uint32_t negativeSum = 0;
    for(auto author: std::as_const(filteredAuthors))
        negativeSum+=allAuthors[author].negativeMatches;
    negativeAverage = negativeSum/filteredAuthors.size();

    auto it = result.recommendations.cbegin();
    while(it != result.recommendations.cend())
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
    uint32_t negativeMatchCutoff = negativeAverage/3;

    std::for_each(filteredAuthors.cbegin(), filteredAuthors.cend(), [maxValue,weightingFunc, authorSize, this, negativeMatchCutoff](int author){
        for(auto fic: inputs.faves[author])
        {
            result.sumNegativeMatchesForFic[fic] += allAuthors[author].negativeMatches;
            auto weighting = weightingFunc(allAuthors[author],authorSize, maxValue );
            double matchCountSimilarityCoef = weighting.GetCoefficient();
            if(allAuthors[author].negativeMatches <= negativeMatchCutoff)
                result.sumNegativeVotesForFic[fic]++;


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
            if(doTrashCounting &&  ownMajorNegatives.cardinality() > startOfTrashCounting){
                if(allAuthors[author].negativeToPositiveMatches > (averageNegativeToPositiveMatches*2))
                {
                    vote = vote / (1 + (allAuthors[author].negativeToPositiveMatches - averageNegativeToPositiveMatches));
                    //if(allAuthors[author].negativeToPositiveMatches > averageNegativeToPositiveMatches*2)
                        //QLOG_INFO() << "reducing vote for fic: " << fic << "from: " << originalVote << " to: " << vote;
                }
                else if(allAuthors[author].negativeToPositiveMatches < (averageNegativeToPositiveMatches - averageNegativeToPositiveMatches/2.)){
                    vote = vote * (1 + (averageNegativeToPositiveMatches - allAuthors[author].negativeToPositiveMatches)*3);
                    //QLOG_INFO() << "increasing vote for fic: " << fic << "from: " << originalVote << " to: " << vote;
                }
                else if(allAuthors[author].negativeToPositiveMatches < (averageNegativeToPositiveMatches - averageNegativeToPositiveMatches/3.))
                    vote = vote * (1 + ((averageNegativeToPositiveMatches - averageNegativeToPositiveMatches/3.) - allAuthors[author].negativeToPositiveMatches));

//                else if(allAuthors[author].negativeToPositiveMatches < (averageNegativeToPositiveMatches))
//                    vote = vote * (1 + (averageNegativeToPositiveMatches - allAuthors[author].negativeToPositiveMatches));
            }

            result.recommendations[fic]+= vote;
            result.AddToBreakdown(fic, weighting.authorType, weighting.GetCoefficient());
        }
    });
    return true;
    //for(auto& breakdownKey : result.pureMatches.keys())
    //{
//        auto normalizer = 1 + 0.1*std::max(4-result.pureMatches[breakdownKey], 0);
//        result.noTrashScore[breakdownKey] = 100*normalizer*(static_cast<double>(result.sumNegativeMatches[breakdownKey])/static_cast<double>(result.pureMatches[breakdownKey]));
    //}
}

AutoAdjustmentAndFilteringResult RecCalculatorImplBase::AutoAdjustRecommendationParamsAndFilter(QSharedPointer<RecommendationList> params)
{
    AutoAdjustmentAndFilteringResult result;
    // if the options were supplied manually, we just return what we got
    if(!params->isAutomatic && !params->adjusting)
        return result;
    // else we try to adjust minimum mathes so that a set of conditionsa re met
    // 1) for the average of 3 numbers we take a differnece between it and the middle one
    //    divide by the middle one and check to see if it's >0.05
    // 2) only apply this if average of 3 is > 70


    // first we need to prepare the dataset
    QMap<int, QList<int>> authorsByMatches;
    int totalMatches = 0;
    for(auto author : std::as_const(filteredAuthors))
    {
        authorsByMatches[allAuthors[author].matches].push_back(allAuthors[author].id);
        totalMatches++;
    }
    //QLOG_INFO() << "authorsByMatches: " << authorsByMatches;
    // then we go through matches calculating averages and checking that conditions are satisfied

    // this will tell us where cutoff happened
    // if it's 2, then we need to take a look if we need to _relax_ settings, not tighten them
    int stoppingIndex = -1;
    QList<int> matches = authorsByMatches.keys();
    QLOG_INFO() << "Keys used: " << matches;
    QLOG_INFO() << "matches count is: " << matches.count();
    //QLOG_INFO() << "total matches: " << totalMatches;
    auto matchCountPreIndex = [&](int externalIndex){
        int sum = 0;
        for(int index = 0; index <= externalIndex && index < matches.size(); index++){
            sum+=authorsByMatches[matches[index]].count();
        }
        //QLOG_INFO() << "Matches left: " << totalMatches - sum;
        return totalMatches - sum;

    };

    if(matches.size() && matches.last() >= params->maxUnmatchedPerMatch/2)
    {
        for(int i = 0; i < matches.count() - 2; i ++){
            if(i > 0)
                result.adjustmentStoppedAtFirstIteration = false;

            QLOG_INFO() << "starting processing of: " << i;
            int firstCount = authorsByMatches[matches[i]].count();
            int secondCount = authorsByMatches[matches[i+1]].count();
            int thirdCount = authorsByMatches[matches[i+2]].count();
            QLOG_INFO() << firstCount << secondCount << thirdCount;
            result.sizes[0] = firstCount;
            result.sizes[1] = secondCount;
            result.sizes[2] = thirdCount;
            double average = static_cast<double>(firstCount + secondCount + thirdCount)/3.;
            //QLOG_INFO() << "average is: " << average;

            if(i == 0 && average < 70)
            {
                stoppingIndex = 0;
                break;
            }
            if(static_cast<float>(matchCountPreIndex(i+2))/static_cast<float>(totalMatches) < 0.1f)
            {
                stoppingIndex = i;
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
    result.performedFiltering = true;
    params->minimumMatch = matches[stoppingIndex];

    if(params->minimumMatch > 20)
        params->minimumMatch = 20;
    for(auto matchCount : std::as_const(matches)){
        if(matchCount < params->minimumMatch)
        {
            //QLOG_INFO() << "discarding match count: " << matchCount;
            continue;
        }
        //QLOG_INFO() << "adding authors for match count: " << matchCount << " amount:" << authorsByMatches[matchCount].size();
        result.authors += QSet<int>(authorsByMatches[matchCount].cbegin(), authorsByMatches[matchCount].cend());
    }
    return result;
}

void RecCalculatorImplBase::AdjustRatioForAutomaticParams()
{
    // intentionally does nothing
}

bool RecCalculatorImplBase::AdjustParamsToHaveExceptionalLists(QSharedPointer<RecommendationList> params, const AutoAdjustmentAndFilteringResult& adjustmentResult)
{
    int dropMinimum = 1;
    int dropRatio = 1;
    auto stopAdjusting = [&](){
        params->isAutomatic = false;
        params->adjusting = false;
        return false;
    };
    if(adjustmentResult.adjustmentStoppedAtFirstIteration && adjustmentResult.sizes[2]*6 > params->maxUnmatchedPerMatch)
        return stopAdjusting();
    if(params->minimumMatch-dropMinimum < 10 && params->maxUnmatchedPerMatch - dropRatio <= 10)
        return stopAdjusting();
    if(params->minimumMatch-dropMinimum < 10 && params->maxUnmatchedPerMatch-dropRatio <= 10)
    {
        // we've failed to find exceptional lists, better to just fall back to higher ratio
        //params->maxUnmatchedPerMatch*=2;
        return stopAdjusting();
    }

    if(params->maxUnmatchedPerMatch-dropRatio < 10 && params->minimumMatch == 20)
    {
        // we've failed to find exceptional lists, better to just fall back to higher ratio
        //params->maxUnmatchedPerMatch*=2;
        return stopAdjusting();
    }

    bool canDropRatio = params->maxUnmatchedPerMatch-dropRatio > 10;
    bool canDropMin = params->minimumMatch-dropMinimum > 10;
    if(canDropRatio  || canDropMin ){
        if(canDropRatio)
            params->maxUnmatchedPerMatch-=dropRatio;
        if(canDropMin )
            params->minimumMatch-=dropMinimum;
        params->isAutomatic = false;
        params->adjusting = true;
        QLOG_INFO() << "params after adjustment: " <<  params->minimumMatch << " " << params->maxUnmatchedPerMatch;
        return true;
    }

    return stopAdjusting();
}


Roaring RecCalculatorImplBase::BuildIgnoreList()
{
    QLOG_INFO() << "Building ignore list";
    QLOG_INFO() << "Ignored fics size:" << params->ignoredDeadFics.size();
    Roaring fullIgnores;
    auto ficKeys = inputs.fics.keys();

    auto worker = [&](const std::pair<QList<int>::const_iterator,QList<int>::const_iterator>& beginEnd){
            auto itCurrent = beginEnd.first;
            auto itEnd= beginEnd.second;
            Roaring ignores;
            while(itCurrent < itEnd)
            {
                auto& fic = inputs.fics[*itCurrent];
                itCurrent++;

                if(!fic)
                    continue;

                bool inIgnored = false;
                // we don't ignore fics that are soruces for the recommednation list
                if(params->ficData->sourceFics.contains(fic->id))
                    continue;

                // we don't ignore fics that user pressed negative tag on for weighting
                if(params->majorNegativeVotes.contains(fic->id))
                    continue;

                // the fics that were maked as "Limbo"
                if(params->ignoredDeadFics.contains(fic->id))
                    inIgnored = true;

                for(auto fandom: std::as_const(fic->fandoms))
                {
                    if(params->ignoredFandoms.contains(fandom) && fandom >= 1)
                        inIgnored = true;
                }
                if(inIgnored)
                    ignores.add(fic->id);
            }
            QLOG_INFO() << "returning ignored fics of size: " << ignores.cardinality();
            return ignores;
        };

    threadedIntListProcessor("Creation of ignore list", QThread::idealThreadCount() - 3,ficKeys,  worker, [&fullIgnores](const Roaring& data){
        fullIgnores= fullIgnores | data;
    });
    QLOG_INFO() << "fanfic ignore list is of size: " << fullIgnores.cardinality();
    return fullIgnores;


}

//struct RatioHash{
//    void AddToken(int token){
//        QWriteLocker locker(&lock);
//        ratioSumInfo[token].ratio = token;
//    };
//    RatioSumInfo& GetToken(int token){
//        QReadLocker locker(&lock);
//        return ratioSumInfo[token];
//    };
//    QMap<uint32_t, RatioSumInfo> ratioSumInfo;
//    QReadWriteLock lock;
//};

struct AuthorRelationsResult{
    uint maximumMatches = 0;
    uint matchSum = 0;
    QMap<uint32_t, RatioInfo> ratioInfo;
    QMap<uint32_t, RatioSumInfo> ratioSumInfo;
};
template <typename T>
void Save( const QMap<uint32_t, T>& data )
{
    QFile file("statistics/_data.txt");
    if (file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        QTextStream stream(&file);
       for (auto key: data.keys())
            stream << QString::number(data[key].ratio).leftJustified(6, ' ')
                    << " authors:    "<< QString::number(data[key].authors).leftJustified(5, ' ')
                    << " avg size:   " << QString::number(data[key].averageListSize).leftJustified(5, ' ')
                    << " fics added: " << QString::number(data[key].lastFicsAdded).leftJustified(5, ' ')
                    << " min size:   " << QString::number(data[key].minListSize).leftJustified(5, ' ')
                    << " max size:   " << QString::number(data[key].maxListSize).leftJustified(5, ' ')
                    << " total size: " << QString::number(data[key].fics.cardinality()).leftJustified(5, ' ')
                   << Qt::endl;
    }
}


void RecCalculatorImplBase::FetchAuthorRelations()
{
    qDebug() << "faves is of size: " << inputs.faves.size();
    allAuthors.clear();
    std::vector<AuthorResult> tempAuthors;
    tempAuthors.resize(inputs.faves.size());
    ownFavourites = {};
    maximumMatches = 0;
    matchSum = 0;
    Roaring ignores = BuildIgnoreList();

    for(auto i = fetchedFics.cbegin(); i != fetchedFics.cend(); i++)
        ownFavourites.add(i.key());

    qDebug() << "finished creating roaring";
    QLOG_INFO() << "user's FFN id: " << params->userFFNId;

    ownProfileId = params->userFFNId;
    maximumMatches = params->minimumMatch;
    AuthorRelationsResult funcResult;
    //RatioHash ratioHash;
    TimedAction action("Relations Creation",[&](){
        auto worker = [&](const std::tuple<QList<int>::const_iterator,QList<int>::const_iterator,QList<int>::const_iterator>& iterators){
            AuthorRelationsResult tempResult;
            tempResult.maximumMatches = params->minimumMatch;
            auto itCurrent = std::get<1>(iterators);
            auto itEnd= std::get<2>(iterators);
            auto rangeBegin = std::get<0>(iterators);
            while(itCurrent < itEnd)
            {
                auto& author = tempAuthors[itCurrent-rangeBegin];
                author.id = *itCurrent;
                if(ownProfileId == author.id)
                {
                    itCurrent++;
                    continue;
                }

                const auto& tempAuthorRoaring = std::as_const(inputs.faves)[author.id];
                author.fullListSize = tempAuthorRoaring.cardinality();
                const uint ignoredFics = tempAuthorRoaring.and_cardinality(ignores);
                const auto unignoredSize = tempAuthorRoaring.cardinality() - ignoredFics;

                // first we need to remove ignored fics
                //auto unignoredSize = inputs.faves[author.id].xor_cardinality(ignoredTemp);
                //Roaring temp = tempAuthorRoaring.operator&(ownFavourites);
                author.matches = tempAuthorRoaring.and_cardinality(ownFavourites);
                author.negativeMatches = tempAuthorRoaring.and_cardinality(ownMajorNegatives);
                author.sizeAfterIgnore = unignoredSize;

                // not interested with lists that don't add anything new
                // also not very interested with listsizes of less than 10 because their ratio will be too skewed
                if(author.matches > 0){
                    const auto ratio = author.sizeAfterIgnore/author.matches;
                    if(ratio <= 1 || author.sizeAfterIgnore < 10){
                        itCurrent++;
                        continue;
                    }
                    author.ratio = ratio;
                    //ratioHash.AddToken(ratio);
                    auto& ratioObject = tempResult.ratioInfo[ratio];
                    ratioObject.ratio = ratio;
                    ratioObject.authors++;
                    if(ratioObject.minMatches > author.matches)
                        ratioObject.minMatches = author.matches;
                    //ratioObject.fics|=tempAuthorRoaring;
                    ratioObject.ficsAfterIgnore|=tempAuthorRoaring.operator-(ignores);

                    if(ratioObject.minListSize > author.sizeAfterIgnore)
                        ratioObject.minListSize = author.sizeAfterIgnore;
                    if(ratioObject.maxListSize < author.sizeAfterIgnore)
                        ratioObject.maxListSize = author.sizeAfterIgnore;
                    if(tempResult.maximumMatches < author.matches)
                    {
                        prevMaximumMatches = tempResult.maximumMatches;
                        tempResult.maximumMatches = author.matches;
                    }
                    tempResult.matchSum+=author.matches;
                }
                itCurrent++;
            }
            return tempResult;
        };


        threadedIntListTupleProcessor("Creation of author relations", QThread::idealThreadCount() - 3,inputs.faves.keys(),  worker, [&funcResult](AuthorRelationsResult&& data){
            if(funcResult.maximumMatches < data.maximumMatches)
                funcResult.maximumMatches = data.maximumMatches;
            funcResult.matchSum+=data.matchSum;

            for(auto i = data.ratioInfo.cbegin(); i != data.ratioInfo.cend(); i++){
                funcResult.ratioInfo[i.key()]+=i.value();
            }
        });
    });
    action.run();
    for(auto&& author: tempAuthors){
        auto id = author.id;
        allAuthors.emplace(std::move(id),std::move(author));
    }
    matchSum = funcResult.matchSum;
    maximumMatches = funcResult.maximumMatches;
    RatioSumInfo tempSummary;

    for(auto i = funcResult.ratioInfo.cbegin(); i != funcResult.ratioInfo.cend(); i++) {
        int tempCardinality = tempSummary.fics.cardinality();
        const auto& item = i.value();
        tempSummary.authors += item.authors;
        tempSummary.ficsAfterIgnore |= item.ficsAfterIgnore;

        auto& sumratio = funcResult.ratioSumInfo[i.key()];
        sumratio.ratio = i.key();
        sumratio.lastFicsAdded = tempSummary.fics.cardinality() - tempCardinality;
        sumratio.fics = tempSummary.fics;
        sumratio.ficsAfterIgnore = tempSummary.ficsAfterIgnore;
        sumratio.authors = tempSummary.authors;
        sumratio.minListSize = item.minListSize;
        sumratio.maxListSize= item.maxListSize;
        //QLOG_INFO() << "ratio: " <<  i.key() << " own size: " << ownFavourites.cardinality() << " projected cardinality: " << ownFavourites.cardinality() * 200  << " sum cardinality: " << sumratio.ficsAfterIgnore.cardinality();
        if(ratioCutoff == std::numeric_limits<uint16_t>::max() && (sumratio.ficsAfterIgnore.cardinality() > (ownFavourites.cardinality() * params->listSizeMultiplier)))
        {
            QLOG_INFO() << "Picking ratio: " << i.key();
            ratioCutoff = i.key();
        }
    }

    QLOG_INFO() << "At the end of author processing maximumMatches: " << maximumMatches << " matchsum: " << matchSum;
}

void RecCalculatorImplBase::CollectFicMatchQuality()
{
//    QVector<int> tempAuthors;

//    for(auto authorId : filteredAuthors)
//    {
//        tempAuthors.push_back(authorId);
//        auto& author = allAuthors[authorId];
//        auto& authorFaves = inputs.faves[authorId];
//        Roaring temp = ownFavourites;
//        auto matches = temp & authorFaves;
//        for(auto value : matches)
//        {
//            auto itFic = inputs.fics.find(value);
//            if(itFic == inputs.fics.end())
//                continue;

//            // < 500 1 votes
//            // < 300 4 votes
//            // < 100 8 votes
//            // < 50 15 votes

//            author.breakdown.total++;
//            if(itFic->get()->favCount <= 50)
//            {
//                author.breakdown.below50++;
//                author.breakdown.votes+=15;
//                if(itFic->get()->authorId != -1)
//                    author.breakdown.authors.insert(itFic->get()->authorId);

//            }
//            else if(itFic->get()->favCount <= 100)
//            {
//                author.breakdown.below100++;
//                author.breakdown.votes+=8;
//                if(itFic->get()->authorId != -1)
//                    author.breakdown.authors.insert(itFic->get()->authorId);
//            }
//            else if(itFic->get()->favCount <= 300)
//            {
//                author.breakdown.below300++;
//                author.breakdown.votes+=4;
//                if(itFic->get()->authorId != -1)
//                    author.breakdown.authors.insert(itFic->get()->authorId);
//            }
//            else if(itFic->get()->favCount <= 500)
//            {
//                author.breakdown.below500++;
//                author.breakdown.votes+=1;
//            }
//        }
//        author.breakdown.priority1 = (author.breakdown.below50 + author.breakdown.below100) >= 3;
//        author.breakdown.priority1 = author.breakdown.priority1 && author.breakdown.authors.size() > 1;

//        author.breakdown.priority2 = (author.breakdown.below50 + author.breakdown.below100 + author.breakdown.below300) >= 5;
//        author.breakdown.priority2 = author.breakdown.priority1 || (author.breakdown.priority2 && author.breakdown.authors.size() > 1);

//        author.breakdown.priority3 = (author.breakdown.below50 + author.breakdown.below100 + author.breakdown.below300 + author.breakdown.below500) >= 8;
//        author.breakdown.priority3 = (author.breakdown.priority1 || author.breakdown.priority2) || (author.breakdown.priority3 && author.breakdown.authors.size() > 1);
//    }
//    std::sort(tempAuthors.begin(), tempAuthors.end(), [&](int id1, int id2){

//        bool largerScore = allAuthors[id1].breakdown.votes > allAuthors[id2].breakdown.votes;
//        bool doubleBelow100First =  (allAuthors[id1].breakdown.below50 + allAuthors[id1].breakdown.below100) >= 2;
//        bool doubleBelow100Second =  (allAuthors[id2].breakdown.below50 + allAuthors[id2].breakdown.below100) >= 2;
//        if(doubleBelow100First && !doubleBelow100Second)
//            return true;
//        if(!doubleBelow100First && doubleBelow100Second)
//            return false;
//        return largerScore;
//    });

//    QLOG_INFO() << "Displaying 100 most closesly matched authors";
//    auto justified = [](int value, int padding) {
//        return QString::number(value).leftJustified(padding, ' ');
//    };
//    QLOG_INFO() << "Displaying priority lists: ";
//    QLOG_INFO() << "//////////////////////";
//    QLOG_INFO() << "P1: ";
//    QLOG_INFO() << "//////////////////////";
//    for(int i = 0; i < 100; i++)
//    {
//        if(allAuthors[tempAuthors[i]].breakdown.priority1)
//            QLOG_INFO() << "Author id: " << justified(tempAuthors[i],9) << " votes: " <<  justified(allAuthors[tempAuthors[i]].breakdown.votes,5) <<
//                                                                                                                                                     "Ratio: " << justified(allAuthors[tempAuthors[i]].ratio, 3) <<
//                                                                                                                                                                                                                    "below 50: " << justified(allAuthors[tempAuthors[i]].breakdown.below50, 5) <<
//                                                                                                                                                                                                                                                                                                  "below 100: " << justified(allAuthors[tempAuthors[i]].breakdown.below100, 5) <<
//                                                                                                                                                                                                                                                                                                                                                                                  "below 300: " << justified(allAuthors[tempAuthors[i]].breakdown.below300, 5) <<
//                                                                                                                                                                                                                                                                                                                                                                                                                                                                  "below 500: " << justified(allAuthors[tempAuthors[i]].breakdown.below500, 5);
//    }
//    QLOG_INFO() << "//////////////////////";
//    QLOG_INFO() << "P2: ";
//    QLOG_INFO() << "//////////////////////";
//    for(int i = 0; i < 100; i++)
//    {
//        if(allAuthors[tempAuthors[i]].breakdown.priority2)
//            QLOG_INFO() << "Author id: " << justified(tempAuthors[i],9) << " votes: " <<  justified(allAuthors[tempAuthors[i]].breakdown.votes,5) <<
//                                                                                                                                                     "Ratio: " << justified(allAuthors[tempAuthors[i]].ratio, 3) <<
//                                                                                                                                                                                                                    "below 50: " << justified(allAuthors[tempAuthors[i]].breakdown.below50, 5) <<
//                                                                                                                                                                                                                                                                                                  "below 100: " << justified(allAuthors[tempAuthors[i]].breakdown.below100, 5) <<
//                                                                                                                                                                                                                                                                                                                                                                                  "below 300: " << justified(allAuthors[tempAuthors[i]].breakdown.below300, 5) <<
//                                                                                                                                                                                                                                                                                                                                                                                                                                                                  "below 500: " << justified(allAuthors[tempAuthors[i]].breakdown.below500, 5);
//    }
//    QLOG_INFO() << "//////////////////////";
//    QLOG_INFO() << "P3: ";
//    QLOG_INFO() << "//////////////////////";
//    for(int i = 0; i < 100; i++)
//    {
//        if(allAuthors[tempAuthors[i]].breakdown.priority3)
//            QLOG_INFO() << "Author id: " << justified(tempAuthors[i],9) << " votes: " <<  justified(allAuthors[tempAuthors[i]].breakdown.votes,5) <<
//                                                                                                                                                     "Ratio: " << justified(allAuthors[tempAuthors[i]].ratio, 3) <<
//                                                                                                                                                                                                                    "below 50: " << justified(allAuthors[tempAuthors[i]].breakdown.below50, 5) <<
//                                                                                                                                                                                                                                                                                                  "below 100: " << justified(allAuthors[tempAuthors[i]].breakdown.below100, 5) <<
//                                                                                                                                                                                                                                                                                                                                                                                  "below 300: " << justified(allAuthors[tempAuthors[i]].breakdown.below300, 5) <<
//                                                                                                                                                                                                                                                                                                                                                                                                                                                                  "below 500: " << justified(allAuthors[tempAuthors[i]].breakdown.below500, 5);
//    }

}

void RecCalculatorImplBase::Filter(QSharedPointer<RecommendationList> params,
                                   const QList<std::function<bool (AuthorResult &, QSharedPointer<RecommendationList>)> >& filters,
                                   const QList<std::function<void (RecCalculatorImplBase *, AuthorResult &)> >& actions)
{
    auto thisPtr = this;
    using FilterType = std::decay<decltype(filters)>::type::value_type;
    using ActionType = std::decay<decltype(actions)>::type::value_type;

    for(auto & [ key, author]: allAuthors){
        auto setInvalid = [](auto& author){
            author.ratio = 99999;
            author.similarityPercentage = 0;
        };
        if(author.matches == 0 || author.sizeAfterIgnore < 10){
            setInvalid(author);
        }
        else{
            author.similarityPercentage = author.matches/(static_cast<double>(author.sizeAfterIgnore)/100.);
            author.ratio = static_cast<double>(author.sizeAfterIgnore)/static_cast<double>(author.matches);
            author.negativeRatio = author.negativeMatches != 0  ? static_cast<double>(author.negativeMatches)/static_cast<double>(author.fullListSize) : std::numeric_limits<double>::max();
            author.listDiff.touchyDifference = GetTouchyDiffForLists(author.id);
            author.listDiff.neutralDifference = GetNeutralDiffForLists(author.id);
            bool fail = std::any_of(std::execution::par_unseq,filters.cbegin(), filters.cend(), [author = std::ref(author),&params](const FilterType& filter){
                return filter(author, params) == 0;

            });
            if(fail || author.ratio == 1){
                setInvalid(author);
                continue;
            }
            std::for_each(actions.cbegin(), actions.cend(), [thisPtr, author = std::ref(author)](const ActionType& action){
                action(thisPtr, author);
            });

        }
    };
}

void RecCalculatorImplBase::CalculateNegativeToPositiveRatio()
{
    for(auto author : std::as_const(filteredAuthors)){
        double ratioForAuthor = static_cast<double>(allAuthors[author].negativeMatches)/static_cast<double>(allAuthors[author].matches);
        //QLOG_INFO() << "Negative matches: " << static_cast<double>(allAuthors[author].negativeMatches) << "Positive matches: " << static_cast<double>(allAuthors[author].matches);
        averageNegativeToPositiveMatches += ratioForAuthor;
        allAuthors[author].negativeToPositiveMatches = ratioForAuthor;
    }
    averageNegativeToPositiveMatches /=filteredAuthors.size();
    QLOG_INFO() << " average ratio division result: " << averageNegativeToPositiveMatches;
}

//void RecCalculatorImplBase::ReportNegativeResults()
//{

//    auto authorList = filteredAuthors.values();

//    std::sort(authorList.begin(), authorList.end(), [&](int id1, int id2){
//        return allAuthors[id1].ratio < allAuthors[id2].ratio;
//    });

//    int limit = authorList.size() > 50 ? 50 : authorList.size();
//    QLOG_INFO() << "RATIO REPORT";
//    int i = 0;
////    for(auto& author : authorList){
////        if(i > limit)
////            break;
////        QLOG_INFO() << Qt::endl << " author: " << author << " size: " <<  allAuthors[author].sizeAfterIgnore << Qt::endl
////                       << " negative matches: " <<  allAuthors[author].negativeMatches
////                       << " negative ratio: " <<  allAuthors[author].negativeRatio
////        << Qt::endl
////                << " positive matches: " <<  allAuthors[author].matches
////                << " positive ratio: " <<  allAuthors[author].ratio;
////        i++;
////    }

////    QLOG_INFO() << "MATCH REPORT";
////    std::sort(authorList.begin(), authorList.end(), [&](int id1, int id2){
////        return allAuthors[id1].matches > allAuthors[id2].matches;
////    });
////    limit = authorList.size() > 50 ? 50 : authorList.size();
////    i = 0;
////    for(auto& author : authorList){
////        if(i > limit)
////            break;
////        QLOG_INFO() << Qt::endl << " author: " << author << " size: " <<  allAuthors[author].sizeAfterIgnore << Qt::endl
////                       << " negative matches: " <<  allAuthors[author].negativeMatches
////                       << " negative ratio: " <<  allAuthors[author].negativeRatio
////        << Qt::endl
////                << " positive matches: " <<  allAuthors[author].matches
////                << " positive ratio: " <<  allAuthors[author].ratio;
////        i++;
////    }

//    QLOG_INFO() << "NEGATIVE REPORT";
//    QList<int> zeroAuthors;
//    for(auto author : authorList)
//    {
//        if(allAuthors[author].negativeMatches == 0)
//            zeroAuthors.push_back(author);
//    }

//    std::sort(authorList.begin(), authorList.end(), [&](int id1, int id2){
//        return allAuthors[id1].negativeMatches < allAuthors[id2].negativeMatches ;
//    });
//    limit = authorList.size() > 50 ? 50 : authorList.size();
//    i = 0;
////    for(auto& author : authorList){
////        if(i > limit)
////            break;
////        QLOG_INFO() << Qt::endl << " author: " << author << " size: " <<  allAuthors[author].sizeAfterIgnore << Qt::endl
////                       << " negative matches: " <<  allAuthors[author].negativeMatches
////                       << " negative ratio: " <<  allAuthors[author].negativeRatio
////        << Qt::endl
////                << " positive matches: " <<  allAuthors[author].matches
////                << " positive ratio: " <<  allAuthors[author].ratio;
////        i++;

////    }
//}

void RecCalculatorImplBase::FillFilteredAuthorsForFics()
{
    QLOG_INFO() << "Filling authors for fics: " << result.recommendations.size();
    int counter = 0;
    // probably need to prefill roaring per fic so that I don't search
    QHash<uint32_t, Roaring> ficsRoarings;
    QLOG_INFO() << "Filling fic roarings";
    for(auto author : std::as_const(filteredAuthors))
    {
        if(counter%10000)
            QLOG_INFO() << counter;
        for(auto fic: inputs.faves[author])
            ficsRoarings[fic].add(author);
        if(counter%10000)
            QLOG_INFO() << "roarings size is: " << ficsRoarings.size();
        counter++;
    }
    counter = 0;

    Roaring filterAuthorsRoar;
    for(auto author : std::as_const(filteredAuthors))
        filterAuthorsRoar.add(author);

    QLOG_INFO()  << "Authors roar size is: " << filterAuthorsRoar.cardinality();

    QLOG_INFO() << "Filling actual author data";

    for(auto i = result.recommendations.cbegin(); i != result.recommendations.cend(); i++){
        if(counter%10000 == 0)
            QLOG_INFO() << "At counter:" << counter;

        auto& ficRoaring = ficsRoarings[i.key()];
        //QLOG_INFO() << "Fic roaring size is:" << ficRoaring.cardinality();

        Roaring temp = filterAuthorsRoar;
        //QLOG_INFO() << "Author roaring size is:" << ficRoaring.cardinality();

        temp = temp & ficRoaring;
        //QLOG_INFO() << "Result size is:" << temp.cardinality();
        if(temp.cardinality() == 0)
            continue;

        if(!authorsForFics.contains(i.key()))
            authorsForFics[i.key()].reserve(1000);
        for(auto author : temp){
            authorsForFics[i.key()].push_back(author);
        }
        counter++;
//        if(counter > 10)
//            break;
    }
    //QLOG_INFO() << "data at exit is: " << authorsForFics;
    QLOG_INFO() << "Finished filling authors for fics";
}

bool RecCalculatorImplDefault::WeightingIsValid() const
{
    return true;
}




}



