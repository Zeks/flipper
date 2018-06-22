#pragma once

#include <QHash>
#include <QSet>

struct UserData{
  QSet<int> allTags;
  QSet<int> activeTags;
  QHash<int, bool> ignoredFandoms;
};

class UserInfoAccessor{
public:
    static QHash<QString, UserData> userData;
};


struct RecommendationsData{
  QSet<int> sourceFics;
  QSet<int> matchedAuthors;
  QHash<int, int> listData;
  QHash<int,int> recommendationList;
};
class RecommendationsInfoAccessor{
public:
    static QHash<QString, RecommendationsData> recommendatonsData;
};
