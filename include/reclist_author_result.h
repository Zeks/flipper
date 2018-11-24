#pragma once
#include <QHash>

namespace core{
struct AuthorResult{
    int id;
    int matches;
    double ratio;
    int size;
    double distance = 0;
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
        if(!isValid)
            return 1;
        return 1 + value;
    }
    double value = 0;
    EAuthorType authorType;
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
