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
#pragma once
#include <QStringList>
#include <QDateTime>
#include <QHash>
#include <QVector>
#include <QSet>
struct SlashFilterState
{
    void Log();
    bool slashFilterEnabled;
    bool applyLocalEnabled;
    bool excludeSlash;
    bool includeSlash;
    bool excludeSlashLocal;
    bool includeSlashLocal;
    bool enableFandomExceptions;
    QList<int> fandomExceptions;
    int slashFilterLevel;
};
namespace core{
struct StoryFilter{
    void Log();

    static QStringList ProcessDelimited(QString str, QString delimiter){
        if(str.contains(delimiter))
            return str.split(delimiter);
        return str.split(" ");
    }
    enum ESortMode
    {
        sm_undefined    = 0,
        sm_wordcount    = 1,
        sm_favourites   = 2,
        sm_favrate      = 3,
        sm_updatedate   = 4,
        sm_publisdate   = 5,
        sm_reccount     = 6,
        sm_wcrcr        = 7,
        sm_revtofav     = 8,
        sm_genrevalues  = 9,

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
    //do I even need that?
    //QString ficCategory;

    bool includeCrossovers = false;
    bool ignoreAlreadyTagged = false;
    bool crossoversOnly = false;
    bool ignoreFandoms = false;
    bool randomizeResults = false;
    bool ensureCompleted = false;
    bool ensureActive = false;
    bool allowUnfinished = true;
    bool allowNoGenre = true;
    bool showOriginsInLists = false;
    bool otherFandomsMode = false;
    bool listOpenMode= false;
    SlashFilterState slashFilter;

    int useThisRecommenderOnly = -1;
    int recordLimit = -1;
    int recordPage = -1;
//    int physicalRecordLimit = -1;
//    int lastFetchedRecordID = -1;
    int minWords = 0;
    int maxWords = 0;
    int maxFics = 0;
    int minFavourites = 0;
    int minRecommendations = 0;
    int listForRecommendations;
    int recentAndPopularFavRatio;

    ESortMode sortMode;
    EReviewBiasMode reviewBias;
    EBiasOperator biasOperator;
    EFilterMode mode;

    int fandom = -1;
    QString website;

    QStringList genreExclusion;
    QStringList genreInclusion;
    QStringList wordExclusion;
    QStringList wordInclusion;
    QStringList activeTags;
    QDateTime recentCutoff;
    QString genreSortField;

    double reviewBiasRatio = 0;

    QList<int> taggedIDs;
    QVector<int> recFics;
    QHash<int, int> recsHash; // for use on the server
};
}
