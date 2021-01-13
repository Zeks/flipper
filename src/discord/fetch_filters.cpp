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

void FetchFicsForDisplayPageCommand(QSharedPointer<FicSourceGRPC> source,
                      QSharedPointer<discord::User> user,
                      int size,
                      QVector<core::Fanfic>* fics)
{
    core::StoryFilter filter;
    filter.recordPage = user->CurrentRecommendationsPage();
    filter.ignoreAlreadyTagged = false;
    filter.crossoversOnly = false;
    filter.showOriginsInLists = false;
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

    filter.reviewBias = core::StoryFilter::bias_none;
    filter.mode = core::StoryFilter::filtering_in_fics;
    //filter.mode = core::StoryFilter::filtering_in_recommendations;
    filter.slashFilter.excludeSlash = true;
    filter.slashFilter.includeSlash = false;
    filter.slashFilter.slashFilterLevel = 1;
    filter.slashFilter.slashFilterEnabled = true;
    auto userFics = user->FicList();
    for(int i = 0; i < userFics->fics.size(); i++)
    {
        if(userFics->sourceFics.contains(userFics->fics.at(i)))
            continue;
        if(!user->GetStrictFreshSort()
                || (user->GetStrictFreshSort() && userFics->metascores.at(i) > 1))
        filter.recommendationScoresSearchToken.ficToScore[userFics->fics[i]] = userFics->metascores.at(i);
    }
    if(user->GetSortGemsFirst())
        filter.recommendationScoresSearchToken.ficToPureVotes = userFics->ficToVotes;
    userFics->ficToMetascore = filter.recommendationScoresSearchToken.ficToScore;
//    userFics->ficToVotes = filter.recommendationScoresSearchToken.ficToPureVotes;

    auto fandomFilter = user->GetCurrentFandomFilter();
    if(fandomFilter.tokens.size() > 0){
        filter.fandom = fandomFilter.tokens.at(0).id;
    }
    if(fandomFilter.tokens.size() > 1){
        filter.secondFandom = fandomFilter.tokens.at(1).id;
    }
    if(user->GetCurrentIgnoredFandoms().fandoms.size() > 0)
        filter.ignoreFandoms = true;
    UserData userData;

    auto ignoredFandoms =  user->GetCurrentIgnoredFandoms();
    for(auto& token: ignoredFandoms.tokens)
    {
        if(!fandomFilter.fandoms.contains(token.id))
            userData.ignoredFandoms[token.id] = token.includeCrossovers;
    }
    userData.allTaggedFics = user->GetIgnoredFics();
    if(user->GetUseLikedAuthorsOnly()){
        filter.tagsAreUsedForAuthors = true;
        userData.ficIDsForActivetags = user->FicList()->sourceFics;
    }
    if(user->GetShowCompleteOnly())
        filter.ensureCompleted = true;
    if(user->GetHideDead())
        filter.ensureActive = true;
    //QLOG_INFO() << "ignored fics: " << user->GetIgnoredFics();
    if(!user->GetPublishedFilter().isEmpty())
        filter.ficDateFilter = CreateDateFilterFromYear(user->GetPublishedFilter(), filters::dft_published);
    else if(!user->GetFinishedFilter().isEmpty())
        filter.ficDateFilter = CreateDateFilterFromYear(user->GetFinishedFilter(), filters::dft_finished);


    fics->clear();
    fics->reserve(size);
    source->userData = userData;
    source->FetchData(filter, fics);

}

void FetchFicsForDisplayRngCommand(int size, QSharedPointer<FicSourceGRPC> source, QSharedPointer<User> user, QVector<core::Fanfic> *fics, int qualityCutoff)
{
    core::StoryFilter filter;
    filter.recordPage = user->CurrentRecommendationsPage();
    filter.ignoreAlreadyTagged = false;
    filter.showOriginsInLists = false;
    filter.crossoversOnly = false;
    filter.recordLimit = size;
    filter.sortMode = core::StoryFilter::sm_metascore;
    filter.reviewBias = core::StoryFilter::bias_none;
    filter.mode = core::StoryFilter::filtering_in_fics;
    filter.randomizeResults = true;
    filter.maxFics = size;
    filter.minRecommendations = qualityCutoff;
    filter.listOpenMode = true;
    //filter.mode = core::StoryFilter::filtering_in_recommendations;
    filter.slashFilter.excludeSlash = true;
    filter.slashFilter.includeSlash = false;
    filter.slashFilter.slashFilterLevel = 1;
    filter.slashFilter.slashFilterEnabled = true;
    filter.rngDisambiguator += user->UserID();
    filter.wipeRngSequence = user->GetRngBustScheduled();
    auto userWordcountFilter = user->GetWordcountFilter();
    filter.minWords = userWordcountFilter.firstLimit;
    filter.maxWords = userWordcountFilter.secondLimit;
    filter.deadFicDaysRange = user->GetDeadFicDaysRange();

    auto userFics = user->FicList();

    for(int i = 0; i < userFics->fics.size(); i++)
    {
        if(userFics->sourceFics.contains(userFics->fics.at(i)))
            continue;
        if(userFics->metascores.at(i) >=qualityCutoff)
            filter.recommendationScoresSearchToken.ficToScore[userFics->fics[i]] = userFics->metascores.at(i);
    }
    userFics->ficToMetascore = filter.recommendationScoresSearchToken.ficToScore;
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
    UserData userData;

    auto ignoredFandoms =  user->GetCurrentIgnoredFandoms();
    for(auto& token: ignoredFandoms.tokens)
        if(!fandomFilter.fandoms.contains(token.id))
            userData.ignoredFandoms[token.id] = token.includeCrossovers;
    if(user->GetUseLikedAuthorsOnly()){
        filter.tagsAreUsedForAuthors = true;
        userData.ficIDsForActivetags = user->FicList()->sourceFics;
    }
    if(user->GetShowCompleteOnly())
        filter.ensureCompleted = true;
    if(user->GetHideDead())
        filter.ensureActive= true;
    userData.allTaggedFics = user->GetIgnoredFics();
    //QLOG_INFO() << "ignored fics: " << user->GetIgnoredFics();

    if(!user->GetPublishedFilter().isEmpty())
        filter.ficDateFilter = CreateDateFilterFromYear(user->GetPublishedFilter(), filters::dft_published);
    else if(!user->GetFinishedFilter().isEmpty())
        filter.ficDateFilter = CreateDateFilterFromYear(user->GetFinishedFilter(), filters::dft_finished);

    fics->clear();
    fics->reserve(size);
    source->userData = userData;
    source->FetchData(filter, fics);
}

int FetchPageCountForFilterCommand(QSharedPointer<FicSourceGRPC> source, QSharedPointer<User> user, int size)
{
    core::StoryFilter filter;
    filter.recordPage = user->CurrentRecommendationsPage();
    filter.ignoreAlreadyTagged = false;
    filter.crossoversOnly = false;
    filter.showOriginsInLists = false;
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

    filter.reviewBias = core::StoryFilter::bias_none;
    filter.mode = core::StoryFilter::filtering_in_fics;
    //filter.mode = core::StoryFilter::filtering_in_recommendations;
    filter.slashFilter.excludeSlash = true;
    filter.slashFilter.includeSlash = false;
    filter.slashFilter.slashFilterLevel = 1;
    filter.slashFilter.slashFilterEnabled = true;
    auto userFics = user->FicList();
    for(int i = 0; i < userFics->fics.size(); i++)
    {
        if(userFics->sourceFics.contains(userFics->fics.at(i)))
            continue;
        if(!user->GetStrictFreshSort()
                || (user->GetStrictFreshSort() && userFics->metascores.at(i) > 1))
        filter.recommendationScoresSearchToken.ficToScore[userFics->fics[i]] = userFics->metascores.at(i);
    }
    if(user->GetSortGemsFirst())
        filter.recommendationScoresSearchToken.ficToPureVotes = userFics->ficToVotes;
    userFics->ficToMetascore = filter.recommendationScoresSearchToken.ficToScore;
//    userFics->ficToVotes = filter.recommendationScoresSearchToken.ficToPureVotes;
    auto fandomFilter = user->GetCurrentFandomFilter();
    if(fandomFilter.tokens.size() > 0){
        filter.fandom = fandomFilter.tokens.at(0).id;
    }
    if(fandomFilter.tokens.size() > 1){
        filter.secondFandom = fandomFilter.tokens.at(1).id;
    }
    if(user->GetCurrentIgnoredFandoms().fandoms.size() > 0)
        filter.ignoreFandoms = true;
    UserData userData;

    auto ignoredFandoms =  user->GetCurrentIgnoredFandoms();
    for(auto& token: ignoredFandoms.tokens)
    {
        if(!fandomFilter.fandoms.contains(token.id))
            userData.ignoredFandoms[token.id] = token.includeCrossovers;
    }
    userData.allTaggedFics = user->GetIgnoredFics();
    if(user->GetUseLikedAuthorsOnly()){
        filter.tagsAreUsedForAuthors = true;
        userData.ficIDsForActivetags = user->FicList()->sourceFics;
    }
    if(user->GetShowCompleteOnly())
        filter.ensureCompleted = true;
    if(user->GetHideDead())
        filter.ensureActive= true;

    if(!user->GetPublishedFilter().isEmpty())
        filter.ficDateFilter = CreateDateFilterFromYear(user->GetPublishedFilter(), filters::dft_published);
    else if(!user->GetFinishedFilter().isEmpty())
        filter.ficDateFilter = CreateDateFilterFromYear(user->GetFinishedFilter(), filters::dft_finished);

    //QLOG_INFO() << "ignored fics: " << user->GetIgnoredFics();

    source->userData = userData;
    auto count = source->GetFicCount(filter);
    return count/size;
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

}
