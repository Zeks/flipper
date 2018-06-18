#pragma once

#include <QHash>
#include <QSet>

struct UserTags{
  QSet<int> allTags;
  QSet<int> activeTags;
};

class InTagAccessor{
public:
    static QHash<QString, UserTags> userTags;
};

