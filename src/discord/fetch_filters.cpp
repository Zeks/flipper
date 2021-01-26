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
    FillFicData();
    FillUserPart();
    fics->clear();
    fics->reserve(size);
    source->FetchData(filter, fics);

    for(auto& fic : *fics)
        fic.score = sourceficsData->ficToMetascore[fic.identity.id];
}

int FicFetcherBase::FetchPageCount(core::StoryFilter partialfilter)
{
    this->filter = partialfilter;
    if(!this->filter.partiallyFilled)
        this->filter = CreateFilter();
    FillFicData();
    FillUserPart();

    auto count = source->GetFicCount(filter);
    return count/size;
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

core::StoryFilter FicFetcherPage::CreateFilter()
{
    core::StoryFilter filter;
    filter.recordPage = user->CurrentRecommendationsPage();
    filter.recordLimit = size;
    auto userWordcountFilter = user->GetWordcountFilter();
    filter.minWords = userWordcountFilter.firstLimit;
    filter.maxWords = userWordcountFilter.secondLimit;
    filter.deadFicDaysRange = user->GetDeadFicDaysRange();

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
    auto userFics = user->FicList();
    for(int i = 0; i < userFics->fics.size(); i++)
    {
        if(userFics->sourceFics.contains(userFics->fics.at(i)))
            continue;
        if(!user->GetStrictFreshSort()
                || (user->GetStrictFreshSort() && userFics->metascores.at(i) > 1))
        filter.recommendationScoresSearchToken.ficToScore[userFics->fics[i]] = userFics->metascores.at(i);
    }
    if(filter.sortMode == core::StoryFilter::sm_gems)
        filter.recommendationScoresSearchToken.ficToPureVotes = userFics->ficToVotes;
    userFics->ficToMetascore = filter.recommendationScoresSearchToken.ficToScore;
}

core::StoryFilter FicFetcherRNG::CreateFilter()
{
    core::StoryFilter filter;
    filter.recordPage = user->CurrentRecommendationsPage();
    filter.recordLimit = size;
    filter.randomizeResults = true;
    filter.maxFics = size;
    filter.minRecommendations = qualityCutoff;
    filter.listOpenMode = true;
    filter.rngDisambiguator += user->UserID();
    filter.wipeRngSequence = user->GetRngBustScheduled();
    auto userWordcountFilter = user->GetWordcountFilter();
    filter.minWords = userWordcountFilter.firstLimit;
    filter.maxWords = userWordcountFilter.secondLimit;
    filter.deadFicDaysRange = user->GetDeadFicDaysRange();

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
    auto userFics = user->FicList();

    for(int i = 0; i < userFics->fics.size(); i++)
    {
        if(userFics->sourceFics.contains(userFics->fics.at(i)))
            continue;
        if(userFics->metascores.at(i) >=qualityCutoff)
            filter.recommendationScoresSearchToken.ficToScore[userFics->fics[i]] = userFics->metascores.at(i);
    }
    userFics->ficToMetascore = filter.recommendationScoresSearchToken.ficToScore;
}

}
