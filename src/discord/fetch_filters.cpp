#include "include/discord/fetch_filters.h"
namespace discord {
void FetchFicsForDisplayPageCommand(QSharedPointer<FicSourceGRPC> source,
                      QSharedPointer<discord::User> user,
                      int size,
                      QVector<core::Fanfic>* fics)
{
    core::StoryFilter filter;
    filter.recordPage = user->CurrentPage();
    filter.ignoreAlreadyTagged = false;
    filter.showOriginsInLists = false;
    filter.recordLimit = size;
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
        if(userFics->sourceFics.contains(userFics->fics[i]))
            continue;
        filter.recsHash[userFics->fics[i]] = userFics->matchCounts[i];
    }
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
        userData.ignoredFandoms[token.id] = token.includeCrossovers;
    userData.allTaggedFics = user->GetIgnoredFics();
    QLOG_INFO() << "ignored fics: " << user->GetIgnoredFics();



    fics->clear();
    fics->reserve(size);
    source->userData = userData;
    source->FetchData(filter, fics);

}

void FetchFicsForDisplayRngCommand(int size, QSharedPointer<FicSourceGRPC> source, QSharedPointer<User> user, QVector<core::Fanfic> *fics, QSet<int> usedRngSequence)
{
    core::StoryFilter filter;
    filter.recordPage = user->CurrentPage();
    filter.ignoreAlreadyTagged = false;
    filter.showOriginsInLists = false;
    filter.recordLimit = size;
    filter.sortMode = core::StoryFilter::sm_metascore;
    filter.reviewBias = core::StoryFilter::bias_none;
    filter.mode = core::StoryFilter::filtering_in_fics;
    filter.randomizeResults = true;
    filter.maxFics = size;
    //filter.mode = core::StoryFilter::filtering_in_recommendations;
    filter.slashFilter.excludeSlash = true;
    filter.slashFilter.includeSlash = false;
    filter.slashFilter.slashFilterLevel = 1;
    filter.slashFilter.slashFilterEnabled = true;
    auto userFics = user->FicList();
    for(int i = 0; i < userFics->fics.size(); i++)
    {
        if(userFics->sourceFics.contains(userFics->fics[i]))
            continue;
        if(!usedRngSequence.isEmpty() && !usedRngSequence.contains(userFics->fics[i]))
            continue;
        filter.recsHash[userFics->fics[i]] = userFics->matchCounts[i];
    }
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
        userData.ignoredFandoms[token.id] = token.includeCrossovers;
    userData.allTaggedFics = user->GetIgnoredFics();
    QLOG_INFO() << "ignored fics: " << user->GetIgnoredFics();



    fics->clear();
    fics->reserve(size);
    source->userData = userData;
    source->FetchData(filter, fics);
}

}
