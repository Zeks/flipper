/*
FFSSE is a replacement search engine for fanfiction.net search results
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

namespace core{
struct StoryFilter{
    static QStringList ProcessDelimited(QString str, QString delimiter){
        if(str.contains(delimiter))
            return str.split(delimiter);
        return str.split(" ");
    }
    enum ESortMode
    {
        wordcount =  0,
        favourites = 1,
        favrate =    2,
        updatedate = 3,
        publisdate = 4,
        reccount=    5,
        wcrcr =      6
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
        filtering_in_fics = 0,
        filtering_in_recommendations = 1
    };
    //do I even need that?
    //QString ficCategory;
    QString fandom;
    QString website;
    int useThisRecommenderOnly = -1;
    bool includeCrossovers = false;
    bool ignoreAlreadyTagged = false;
    int minWords = 0;
    int maxWords = 0;
    int maxFics = 0;
    bool randomizeResults = false;
    int minFavourites = 0;
    int minRecommendations = 0;
    bool ensureCompleted = false;
    bool ensureActive = false;
    bool allowUnfinished = true;
    bool allowNoGenre = true;
    bool showOriginsInLists = false;
    ESortMode sortMode;
    int listForRecommendations;
    EReviewBiasMode reviewBias;
    EBiasOperator biasOperator;
    double reviewBiasRatio = 0;

    QDateTime recentCutoff;
    int recentAndPopularFavRatio;
    QStringList genreExclusion;
    QStringList genreInclusion;
    QStringList wordExclusion;
    QStringList wordInclusion;
    QStringList titleInclusion;
    QStringList activeTags;
    EFilterMode mode;
};
}

