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
        reccount=    4,
        wcrcr =      5
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
    bool ensureCompleted = false;
    bool ensureActive = false;
    bool allowUnfinished = true;
    bool allowNoGenre = true;
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

