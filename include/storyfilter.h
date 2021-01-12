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
#pragma once
#include <QStringList>
#include <QDateTime>
#include <QHash>
#include <QVector>
#include <QSet>
#include "core/fandom_list.h"
#include "filters/date_filter.h"

struct SlashFilterState
{
    void Log();
    bool slashFilterEnabled = false;
    bool applyLocalEnabled = false;
    bool excludeSlash = true;
    bool includeSlash = false;
    bool excludeSlashLocal = false;
    bool includeSlashLocal = false;
    bool enableFandomExceptions = false;
    QList<int> fandomExceptions;
    int slashFilterLevel = 0;
    bool onlyExactLevel = false;
    bool onlyMatureForSlash = false;
};


namespace core{

struct RecommendationListFicSearchToken
{
    std::unordered_map<int, int> ficToScore;
    std::unordered_map<int, int> ficToPureVotes;
};

struct FicDateFilter{
    std::string dateStart;
    std::string dateEnd;
    filters::EDateFilterType mode = filters::dft_none;
};

struct StoryFilter{
    void Log();

    static QStringList ProcessDelimited(QString str, QString delimiter){
        if(str.contains(delimiter))
            return str.split(delimiter);
        return str.split(QStringLiteral(" "));
    }
    enum EScoreType
    {
        st_points = 0,
        st_minimal_dislikes = 1,
    };
    enum ESortMode
    {
        sm_undefined         = 0,
        sm_wordcount         = 1,
        sm_favourites        = 2,
        sm_trending          = 3,
        sm_updatedate        = 4,
        sm_publisdate        = 5,
        sm_metascore         = 6,
        sm_wcrcr             = 7,
        sm_revtofav          = 8,
        sm_genrevalues       = 9,
        sm_userscores        = 10,
        sm_minimize_dislikes = 11,
        sm_gems              = 12,

    };
    enum EReviewBiasMode{
        bias_none    = 0,
        bias_favor   = 1,
        bias_exclude = 2
    };
    enum EBiasOperator{
        bias_more = 0,
        bias_less = 1
    };
    enum EFilterMode{
        filtering_undefined = 0,
        filtering_in_fics = 1,
        filtering_in_recommendations = 2,
        filtering_whole_list = 3
    };
    enum ERatingFilter{
        rt_t_m = 0,
        rt_t = 1,
        rt_m = 2,
    };
    enum EGenrePresence{
        gp_considerable = 0,
        gp_medium = 1,
        gp_minimal = 2,
        gp_none = 3
    };
    enum ESourceListLimiter{
        sll_all = 0,
        sll_above_average = 1,
        sll_very_close = 2,
        sll_exceptional = 3
    };
    enum EUseThisFicType{
      utf_ffn_id = 0,
      utf_db_id = 1
    };
    enum EShowSourcesMode{
        ssm_filter_as_usual = 0,
        ssm_show = 1,
        ssm_hide = 2,
    };

    bool isValid = true;
    bool includeCrossovers = true;
    bool crossoversOnly = false;
    bool ignoreAlreadyTagged = false;
    bool ignoreFandoms = false;
    bool randomizeResults = false;
    bool ensureCompleted = false;
    bool ensureActive = false;
    bool allowUnfinished = true;
    bool allowNoGenre = true;
    bool showOriginsInLists = false;
    bool otherFandomsMode = false;
    bool listOpenMode= false;
    bool tagsAreUsedForAuthors = false;
    bool likedAuthorsEnabled = false;
    bool tagsAreANDed = false;
    bool useRealGenres = false;
    bool descendingDirection = true;
    bool displayPurgedFics = false;
    bool displaySnoozedFics = false;
    bool wipeRngSequence = false;;

    SlashFilterState slashFilter;

    int useThisRecommenderOnly = -1;
    int recordLimit = -1;
    int recordPage = -1;
    int minWords = 0;
    int maxWords = 0;
    int maxFics = 0;
    int minFavourites = 0;
    int minRecommendations = 0;
    int listForRecommendations = -1;

    int recentAndPopularFavRatio = -1;
    int ignoredFandomCount = 0;
    int recommendationsCount = 0;
    int activeTagsCount = 0;
    int allTagsCount = 0;
    int allSnoozeCount = 0;
    int useThisAuthor = -1;
    int useThisFic = -1;
    int protocolMajorVersion = 2;
    int protocolMinorVersion = 0;
    int deadFicDaysRange = 365;

    QList<int> usedRecommenders;
    ESortMode sortMode;
    EReviewBiasMode reviewBias;
    EBiasOperator biasOperator;
    EFilterMode mode;
    ERatingFilter rating;
    EGenrePresence genrePresenceForInclude;
    EGenrePresence genrePresenceForExclude;
    EShowSourcesMode showRecSources = ssm_filter_as_usual;
    ESourceListLimiter sourcesLimiter = ESourceListLimiter::sll_all;
    EUseThisFicType useThisFicType = EUseThisFicType::utf_ffn_id;

    int fandom = -1;
    int secondFandom = -1;


    QString website;

    QStringList genreExclusion;
    QStringList genreInclusion;
    QStringList wordExclusion;
    QStringList wordInclusion;
    QStringList activeTags;
    QDateTime recentCutoff;
    QString genreSortField;

    double reviewBiasRatio = 0;

    QList<int> recFics;
    RecommendationListFicSearchToken recommendationScoresSearchToken;
    //QHash<int, int> recsHash; // for use on the server
    QHash<int, int> scoresHash; // for use on the server
    std::unordered_map<int,core::fandom_lists::FandomSearchStateToken> fandomStates;
    QString userToken;
    QString rngDisambiguator;
    FicDateFilter ficDateFilter;
};

struct ReclistFilter{
    int mainListId = -1;
    int secondListId = -1;
    int minMatchCount = 0;
    core::StoryFilter::ESourceListLimiter limiter = core::StoryFilter::sll_all;
    core::StoryFilter::EScoreType scoreType = core::StoryFilter::st_points;
    bool displayPurged = false;
};

}

