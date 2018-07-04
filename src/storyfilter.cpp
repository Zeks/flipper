#include "include/storyfilter.h"
#include "logger/QsLog.h"


namespace core{
void StoryFilter::Log()
{
    QLOG_INFO() << "////////////////////STORY FILTER DATA START/////////////////////";
    QLOG_INFO() << " Website : " << website;
    QLOG_INFO() << " Fandom : " << fandom;

    QLOG_INFO() << " Fetch size : " << recordLimit;
    QLOG_INFO() << " Fetched page : " << recordPage;

    QLOG_INFO() << " Include crossovers : " << includeCrossovers;
    QLOG_INFO() << " Crossovers only : " << crossoversOnly;

    QLOG_INFO() << " Ignore fandoms : " << ignoreFandoms;
    QLOG_INFO() << " Randomize results : " << randomizeResults;
    QLOG_INFO() << " Ensure completed : " << ensureCompleted;
    QLOG_INFO() << " Ensure active : " << ensureActive;
    QLOG_INFO() << " Allow unfinihsed : " << allowUnfinished;
    QLOG_INFO() << " Allow no genre : " << allowNoGenre;
    QLOG_INFO() << " Show origins in lists : " << showOriginsInLists;
    QLOG_INFO() << " Other fandoms mode : " << otherFandomsMode;


    QLOG_INFO() << " Sort mode : " << sortMode;

    QLOG_INFO() << " Min words : " << minWords;
    QLOG_INFO() << " Max words : " << maxWords;
    QLOG_INFO() << " Max fics : " << maxFics;
    QLOG_INFO() << " Min favourites : " << minFavourites;

    QLOG_INFO() << " Exclusive recommender: " << useThisRecommenderOnly;
    QLOG_INFO() << " Min recommendations : " << minRecommendations;
    QLOG_INFO() << " List for recommendations : " << listForRecommendations;
    QLOG_INFO() << " List open mode : " << listOpenMode;

    QLOG_INFO() << " Review Bias : " << reviewBias;
    QLOG_INFO() << " Review bias ratio : " << reviewBiasRatio;
    QLOG_INFO() << " Bias operator : " << biasOperator;

    QLOG_INFO() << " Filter mode: " << mode;

    QLOG_INFO() << " Genre exclusion : " << genreExclusion;

    QLOG_INFO() << " Genre inclusion : " << genreInclusion;
    QLOG_INFO() << " Word exclusions : " << wordExclusion;
    QLOG_INFO() << " Word inclusion : " << wordInclusion;


    QLOG_INFO() << " Recent cutoff : " << recentCutoff;
    QLOG_INFO() << " Recent&Popular ratio : " << recentAndPopularFavRatio;

    //QLOG_INFO() << " Genre sort field: " << genreSortField;

    QLOG_INFO() << " Active tags : " << activeTags;
    QLOG_INFO() << " Ignore already tagged: " <<ignoreAlreadyTagged;
    QLOG_INFO() << " Tagged ids : " << taggedIDs;

    slashFilter.Log();
    QLOG_INFO() << "////////////////////STORY FILTER DATA END/////////////////////";
}
}

void SlashFilterState::Log()
{
    QLOG_INFO() << " Slash filter enabled : " << slashFilterEnabled;
    QLOG_INFO() << " Slash filter level : " << slashFilterLevel;
    QLOG_INFO() << " Exclude slash : " << excludeSlash;
    QLOG_INFO() << " Include Slash: " << includeSlash;
    QLOG_INFO() << " Fandom exceptions enabled: " << enableFandomExceptions;
    QLOG_INFO() << " Fandom exceptions: " << fandomExceptions;
    QLOG_INFO() << " Local slash filter enabled: " << applyLocalEnabled;
    QLOG_INFO() << " Exclude slash local: " << excludeSlashLocal;
    QLOG_INFO() << " Include slash local: " << includeSlashLocal;
}
