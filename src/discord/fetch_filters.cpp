/*Flipper is a recommendation and search engine for fanfiction.net
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
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#include "include/discord/fetch_filters.h"
namespace discord {

core::FicDateFilter CreateDateFilterFromYear(QString year, filters::EDateFilterType type){
    core::FicDateFilter result;
    QDate date = QDate::fromString(year, "yyyy");
    auto nextYear = date.addYears(1);
    result.dateStart = QString(year + "-00-00").toStdString();
    result.dateEnd = QString(nextYear.toString("yyyy")+ "-00-00").toStdString();
    result.mode = type;
    return result;
}

void FetchFicsForShowIdCommand(QSharedPointer<FicSourceGRPC> source, QList<int> ficIds, QVector<core::Fanfic> *fics)
{
    fics->clear();
    fics->reserve(ficIds.size());

    QList<core::StoryFilter::FicId> ficsTask;
    for(auto ficId : ficIds)
    {
        core::StoryFilter::FicId fic;
        fic.id = ficId;
        fic.idType = core::StoryFilter::EUseThisFicType::utf_ffn_id;
        ficsTask.push_back(fic);
    }
    source->FetchFics(ficsTask, fics);
}

void FicFetcherBase::Fetch(core::StoryFilter partialfilter, QVector<core::Fanfic> *fics)
{
    this->filter = partialfilter;
    if(!this->filter.partiallyFilled)
        this->filter = CreateFilter();

    filter.recordLimit = this->recordLimit;
    filter.recordPage= this->pageToUse;
    if(displayingSimilarityList)
        ResetFilterForSimilarityList(this->filter);

    FillFicData();
    FillUserPart();

    fics->clear();
    fics->reserve(this->recordLimit);
    source->FetchData(filter, fics);

    for(auto& fic : *fics)
        fic.score = sourceficsData->ficToMetascore[fic.identity.id];
}

int FicFetcherBase::FetchPageCount(core::StoryFilter partialfilter)
{
    this->filter = partialfilter;
    if(!this->filter.partiallyFilled)
        this->filter = CreateFilter();

    if(displayingSimilarityList)
        ResetFilterForSimilarityList(this->filter);

    FillFicData();
    FillUserPart();


    auto count = source->GetFicCount(filter);
    return count/recordLimit;
}

void FicFetcherBase::FillUserPart()
{
    UserData userData;
    auto ignoredFandoms =  user->GetCurrentIgnoredFandoms();
    std::set<int> filteredFandomSet{filter.fandom,filter.secondFandom};
    for(auto& token: ignoredFandoms.tokens)
    {
        bool inExplicitlyShown = filteredFandomSet.find(token.id) != std::end(filteredFandomSet);
        if(!inExplicitlyShown){
            userData.ignoredFandoms[token.id] = token.includeCrossovers;
        }
    }
    if(filter.tagsAreUsedForAuthors)
        userData.ficIDsForActivetags = sourceficsData->sourceFics;

    userData.allTaggedFics = user->GetIgnoredFics();
    source->userData = userData;
}

void FicFetcherBase::ResetFilterForSimilarityList(core::StoryFilter & filter)
{
    filter.minWords = 0;
    filter.maxWords = 0;
    filter.strictFreshSort = false;
    filter.listOpenMode = false;
    filter.sortMode = core::StoryFilter::sm_metascore;
    filter.fandom = -1;
    filter.secondFandom = -1;
    filter.ignoreFandoms = false;
    filter.tagsAreUsedForAuthors = false;
    filter.ensureCompleted = false;
    filter.ensureActive = false;
    filter.ficDateFilter = {};
}

core::StoryFilter FicFetcherPage::CreateFilter()
{
    core::StoryFilter filter;
    filter.recordPage = pageToUse;
    auto userWordcountFilter = user->GetWordcountFilter();
    filter.minWords = userWordcountFilter.firstLimit;
    filter.maxWords = userWordcountFilter.secondLimit;
    filter.deadFicDaysRange = user->GetDeadFicDaysRange();
    filter.strictFreshSort = user->GetStrictFreshSort();

    if(user->GetSortFreshFirst()){
        filter.listOpenMode = true;
        filter.sortMode = core::StoryFilter::sm_publisdate;
    }else if(user->GetSortGemsFirst()){
        filter.listOpenMode = true;
        filter.sortMode = core::StoryFilter::sm_gems;
    }
    else
        filter.sortMode = core::StoryFilter::sm_metascore;

    auto fandomFilter = user->GetCurrentFandomFilter();
    if(fandomFilter.tokens.size() > 0){
        filter.fandom = fandomFilter.tokens.at(0).id;
    }
    if(fandomFilter.tokens.size() > 1){
        filter.secondFandom = fandomFilter.tokens.at(1).id;
    }
    if(user->GetCurrentIgnoredFandoms().fandoms.size() > 0)
        filter.ignoreFandoms = true;

    if(user->GetUseLikedAuthorsOnly())
        filter.tagsAreUsedForAuthors = true;
    if(user->GetShowCompleteOnly())
        filter.ensureCompleted = true;
    if(user->GetHideDead())
        filter.ensureActive = true;
    if(!user->GetPublishedFilter().isEmpty())
        filter.ficDateFilter = CreateDateFilterFromYear(user->GetPublishedFilter(), filters::dft_published);
    else if(!user->GetFinishedFilter().isEmpty())
        filter.ficDateFilter = CreateDateFilterFromYear(user->GetFinishedFilter(), filters::dft_finished);
    return filter;
}

void FicFetcherPage::FillFicData()
{
    //auto userFics = user->FicList();

    for(int i = 0; i < sourceficsData->fics.size(); i++)
    {
        if(sourceficsData->sourceFics.contains(sourceficsData->fics.at(i)))
            continue;
        if(!filter.strictFreshSort
                || (filter.strictFreshSort && sourceficsData->metascores.at(i) > 1))
        filter.recommendationScoresSearchToken.ficToScore[sourceficsData->fics[i]] = sourceficsData->metascores.at(i);
    }
    if(filter.sortMode == core::StoryFilter::sm_gems)
        filter.recommendationScoresSearchToken.ficToPureVotes = sourceficsData->ficToVotes;
    sourceficsData->ficToMetascore = filter.recommendationScoresSearchToken.ficToScore;
}

core::StoryFilter FicFetcherRNG::CreateFilter()
{
    core::StoryFilter filter;
    filter.recordPage = pageToUse;
    filter.recordLimit = recordLimit;
    filter.randomizeResults = true;
    filter.maxFics = recordLimit;
    filter.minRecommendations = qualityCutoff;
    filter.listOpenMode = true;
    filter.rngDisambiguator += user->UserID();
    filter.wipeRngSequence = user->GetRngBustScheduled();
    auto userWordcountFilter = user->GetWordcountFilter();
    filter.minWords = userWordcountFilter.firstLimit;
    filter.maxWords = userWordcountFilter.secondLimit;
    filter.deadFicDaysRange = user->GetDeadFicDaysRange();
    filter.strictFreshSort = user->GetStrictFreshSort();

    auto fandomFilter = user->GetCurrentFandomFilter();
    if(fandomFilter.tokens.size() > 0){
        filter.fandom = fandomFilter.tokens.at(0).id;
        filter.includeCrossovers = fandomFilter.tokens.at(0).includeCrossovers;
    }
    if(fandomFilter.tokens.size() > 1){
        filter.secondFandom = fandomFilter.tokens.at(1).id;
        filter.includeCrossovers = true;
    }
    if(user->GetCurrentIgnoredFandoms().fandoms.size() > 0)
        filter.ignoreFandoms = true;
    if(user->GetUseLikedAuthorsOnly())
        filter.tagsAreUsedForAuthors = true;
    if(user->GetShowCompleteOnly())
        filter.ensureCompleted = true;
    if(user->GetHideDead())
        filter.ensureActive= true;
    if(!user->GetPublishedFilter().isEmpty())
        filter.ficDateFilter = CreateDateFilterFromYear(user->GetPublishedFilter(), filters::dft_published);
    else if(!user->GetFinishedFilter().isEmpty())
        filter.ficDateFilter = CreateDateFilterFromYear(user->GetFinishedFilter(), filters::dft_finished);
    return filter;
}

void FicFetcherRNG::FillFicData()
{
    //auto userFics = user->FicList();

    for(int i = 0; i < sourceficsData->fics.size(); i++)
    {
        if(sourceficsData->sourceFics.contains(sourceficsData->fics.at(i)))
            continue;
        if(sourceficsData->metascores.at(i) >=qualityCutoff)
            filter.recommendationScoresSearchToken.ficToScore[sourceficsData->fics[i]] = sourceficsData->metascores.at(i);
    }
    sourceficsData->ficToMetascore = filter.recommendationScoresSearchToken.ficToScore;
}

}
