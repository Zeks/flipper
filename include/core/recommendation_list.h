#pragma once

#include "core/db_entity.h"
#include "reclist_author_result.h"

#include <QSharedPointer>
#include <QString>
#include <QDateTime>
namespace core {
class Fanfic;
class Author;

struct FicRecommendation
{
    QSharedPointer<core::Fanfic> fic;
    QSharedPointer<core::Author> author;
    bool IsValid(){
        if(!fic || !author)
            return false;
        return true;
    }
};

struct FavouritesMatchResult{
    int ratio = 0;
    int ratioWithoutIgnores = 0;
    QList<int> matches;
};

class RecommendationList;
typedef QSharedPointer<RecommendationList> RecPtr;

struct RecommendationListFicData
{
    void Clear();
    int id = -1;
    QSet<int> sourceFics;
    QSet<int> sourceFicsFFN;
    QVector<int> fics;
    QVector<int> purges;
    QVector<int> metascores;
    QVector<double> noTrashScores;
    QVector<int> authorIds;
    QHash<int, int> matchReport;
    std::unordered_map<int, int> ficToMetascore;
    std::unordered_map<int, int> ficToVotes;
    QHash<int, core::MatchBreakdown> breakdowns;

    int perfectRngScoreCutoff = 0;
    int goodRngScoreCutoff = 0;
    int perfectRngFicsSize = 0;
    int goodRngFicsSize = 0;
    QSet<int> perfectRngFics;
    QSet<int> goodRngFics;
};



class RecommendationList : public DBEntity{
public:
    RecommendationList(){
        ficData.reset(new RecommendationListFicData());
    }
    static RecPtr NewRecList() { return QSharedPointer<RecommendationList>(new RecommendationList);}
    void Log();
    void PassSetupParamsInto(RecommendationList& other);
    bool success = false;
    bool isAutomatic = true;
    bool adjusting = false;
    bool useWeighting = false;
    bool useMoodAdjustment = false;
    bool hasAuxDataFilled = false;
    bool useDislikes = false;
    bool useDeadFicIgnore= false;
    bool assignLikedToSources = false;
    bool ignoreBreakdowns = false;

    int id = -1;
    int ficCount =-1;
    int minimumMatch = -1;
    int alwaysPickAt = -2;
    int maxUnmatchedPerMatch = 5000;
    int userFFNId = -1;
    int sigma2Distance = -1;
    int listSizeMultiplier = 200;
    int ficFavouritesCutoff = 0;
    int resultLimit = 0;
    uint16_t ratioCutoff = std::numeric_limits<uint16_t>::max();

    double quadraticDeviation = -1;
    double ratioMedian = -1;

    QString name;
    QString tagToUse;

    // this needs to take into account if the fandom is ignored fully
    // but for now lets leave it as is
    QSet<int> ignoredFandoms;
    QSet<int> ignoredDeadFics;
    QSet<int> likedAuthors;
    QSet<int> minorNegativeVotes;
    QSet<int> majorNegativeVotes;
    QDateTime created;
    QSharedPointer<RecommendationListFicData> ficData;

    QStringList errors;
};



}
