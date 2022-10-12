#pragma once
#include <QHash>
#include <QSet>
#include <QVector>
#include <optional>
//#include "include/rec_calc/rec_calculator_base.h"


namespace core{


struct ListMoodDifference{
    std::optional<double> neutralDifference = 0.;
    std::optional<double> touchyDifference = 0.;
};
struct AuthorMatchBreakdown{
    int below50 = 0;
    int below100 = 0;
    int below300 = 0;
    int below500 = 0;
    int total = 0;
    int votes = 0;
    QSet<int> authors;
    bool aboveThreshold = false;
    bool priority1 = false;
    bool priority2 = false;
    bool priority3 = false;
};

struct AuthorWeightingResult
{
    enum class EAuthorType{
        common = 0,
        uncommon = 1,
        rare = 2,
        unique = 3
    };

    AuthorWeightingResult(){}
    AuthorWeightingResult(double value, EAuthorType type): isValid(true), value(value), authorType(type){}
    bool isValid = false;
    double GetCoefficient(){
        if(ownProfile == true)
            return 0;

        if(!isValid)
            return 1;
        return 1 + value;
    }
    bool ownProfile = false;;
    double value = 0;
    EAuthorType authorType;
};

struct AuthorResult{
    uint32_t sizeAfterIgnore = 0;

    uint32_t id;
    uint32_t matches = 0;
    uint32_t negativeMatches = 0;
    uint32_t fullListSize;
    uint32_t usedMinimumMatrch= 0;

    double ratio = 0;
    double similarityPercentage = 0;
    double negativeRatio = 0;
    double negativeToPositiveMatches = 0;
    double distance = 0;
    double usedRatio = 0;

    AuthorWeightingResult::AuthorWeightingResult::EAuthorType authorMatchCloseness = AuthorWeightingResult::EAuthorType::common;
    AuthorMatchBreakdown breakdown;
    ListMoodDifference listDiff;
};


struct DiagnosticsForReclist{
    bool isValid = false;

    int sigma2Dist = 0;
    double ratioMedian = 0;
    double quad = 0;

    QHash<uint32_t, QVector<uint32_t>> authorsForFics;
    QVector<AuthorResult> authorData;
};


inline uint qHash(AuthorWeightingResult::EAuthorType key, uint seed)
 {
     return ::qHash(static_cast<uint>(key), seed);
 }

struct MatchBreakdown{
  uint32_t ficId = -1;
  QHash<AuthorWeightingResult::EAuthorType, int> authorTypes;
  QHash<AuthorWeightingResult::EAuthorType, double> authorTypeVotes;
  void AddAuthorResult(AuthorWeightingResult::EAuthorType type,
                       int counts,
                       double votes){
      authorTypes[type] = counts;
      authorTypeVotes[type] =votes;
  }
  void AddAuthor(AuthorWeightingResult::EAuthorType type, double vote){
    authorTypes[type]++;
    authorTypeVotes[type]+=vote;
  }
  QList<uint32_t> GetPercentages();
};
}
