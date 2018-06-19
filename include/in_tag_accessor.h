#pragma once

#include <QHash>
#include <QSet>

//struct IgnoredFandom{
//    int fandomId;
//    bool ignoredCrosses;
//};

struct UserData{
  QSet<int> allTags;
  QSet<int> activeTags;
  QHash<int, bool> ignoredFandoms;
};

class UserInfoAccessor{
public:
    static QHash<QString, UserData> userData;
};

