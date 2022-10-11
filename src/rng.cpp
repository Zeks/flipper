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
#include "include/rng.h"
#include "include/Interfaces/db_interface.h"
#include <functional>
#include <random>

namespace core{
QStringList DefaultRNGgenerator::Get(QSharedPointer<Query> query, QString userToken, sql::Database, StoryFilter &filter)
{
    auto where = userToken.toStdString() + query->str;
    QStringList result;
    bool listIsOutdated = false, containsList = false;
    for(const auto& bind: std::as_const(query->bindings))
        where += bind.key + bind.value.toString().substr(0,30);
    where += "Minrecs: " + QString::number(filter.minRecommendations).toStdString();
    where += "Rated: " + QString::number(filter.rating).toStdString();
    where += "Complete: " + QString::number(filter.ensureCompleted).toStdString();
    where += "Liked: " + QString::number(filter.likedAuthorsEnabled).toStdString();
    where += "Dead: " + QString::number(filter.allowUnfinished).toStdString();
    where += "Active tags: " + filter.activeTags.join(",").toStdString();
    where += "Displaying purged:" + QString::number(filter.displayPurgedFics).toStdString();
    where += "Disambiguator: " + filter.rngDisambiguator.toStdString();
    {
        // locking to make sure it's not modified when we search
        QReadLocker locker(&rngData->lock);
        containsList = rngData->randomIdLists.find(where) != rngData->randomIdLists.end();
        if(containsList)
            listIsOutdated = rngData->randomIdLists[where]->generationTimestamp < QDateTime::currentDateTimeUtc().addDays(-1);
    }

    QLOG_INFO() << "RANDOM USING WHERE:" << QString::fromStdString(where);
    if(!containsList || listIsOutdated || filter.wipeRngSequence)
    {
        QWriteLocker locker(&rngData->lock);
        RemoveOutdatedRngSequences();
        RemoveOlderRngSequencesPastTheLimit(200);

        QLOG_INFO() << "GENERATING RANDOM SEQUENCE";
        auto idList = portableDBInterface->GetIdListForQuery(query);
        if(idList.size() == 0)
            idList.push_back("-1");
        RNGData::ListPtr usedList;
        if(!containsList || listIsOutdated)
        {
            RNGData::ListPtr newList(new RNGData::ListPtr::Type);
            rngData->randomIdLists[where] = newList;
            usedList = newList;
            usedList->sequenceIdentifier = where;
        }
        else
            usedList = rngData->randomIdLists[where];
        usedList->ids = idList;
        usedList->lastAccessTimestamp = QDateTime::currentDateTimeUtc();
        usedList->generationTimestamp = QDateTime::currentDateTimeUtc();
        if(!containsList)
            rngData->queue.push(usedList);
    }
    else
        QLOG_INFO() << "USING CACHED RANDOM SEQUENCE";

    std::random_device rd; // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    auto& currentList = rngData->randomIdLists[where]->ids;
    rngData->randomIdLists[where]->lastAccessTimestamp = QDateTime::currentDateTimeUtc();
    std::uniform_int_distribution<> distr(0, currentList.size()-1); // define the range
    result.reserve(filter.maxFics);
    for(auto i = 0; i < filter.maxFics; i++)
    {
        auto value = distr(eng);
        result.push_back(currentList[value]);
    }
    return result;
}

void DefaultRNGgenerator::RemoveOutdatedRngSequences()
{
    forever{
        if(!rngData->queue.size())
            break;
        auto item = rngData->queue.top();
        if(item->generationTimestamp < QDateTime::currentDateTimeUtc().addDays(-1))
        {
            rngData->randomIdLists.erase(item->sequenceIdentifier);
            rngData->queue.pop();
        }
        else
            break;
    }
}

void DefaultRNGgenerator::RemoveOlderRngSequencesPastTheLimit(size_t limit)
{
    if(rngData->randomIdLists.size() < limit)
        return;
    //rngData->Log("pre");
    // HACK this resorts the queue
    RNGData::ListPtr newList(new RNGData::ListPtr::Type);
    newList->lastAccessTimestamp = QDateTime::currentDateTime().addYears(-1);
    rngData->queue.push(newList);
    auto item = rngData->queue.top();
    rngData->queue.pop();
    // HACK END
    while(rngData->queue.size() >= limit){
        auto item = rngData->queue.top();
        rngData->randomIdLists.erase(item->sequenceIdentifier);
        rngData->queue.pop();
    }
    //rngData->Log("post");
}

RNGData::RNGData(): queue([](ListPtr i1,ListPtr i2)->bool{
    return i1->lastAccessTimestamp > i2->lastAccessTimestamp;
})
{

}

void RNGData::Log(QString prefix)
{
    QLOG_INFO() << "RNG SEQUENCE LOG:" << prefix;
    std::vector<ListPtr> values;
    values.reserve(randomIdLists.size());
    for(auto& [key,value] : randomIdLists)
        values.push_back(value);
    std::sort(values.begin(), values.end(), [](auto i1, auto i2){ return i1->lastAccessTimestamp < i2->lastAccessTimestamp;});
    for(auto& value: values){
        QLOG_INFO() << "ITEM:" << value->lastAccessTimestamp;
    }
}

}
