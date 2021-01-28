#pragma once
#include "core/recommendation_list.h"
#include "include/storyfilter.h"
#include "discord/fetch_filters.h"
#include <QTextStream>
namespace discord{

struct RecsMessageCreationMemo{
    //int64_t originalMessage;
    //int rngQualityCutoff = 0;
    int ficFavouritesCutoff = 0;
    std::atomic<int> page = 0;
    bool strictFreshSort = 0;
    QString userFFNId = 0;
    core::StoryFilter filter;
    StoryFilterDisplayToken storyFilterDisplayToken;
    QSet<int> sourceFics;
    friend  QTextStream &operator<<(QTextStream &out, const RecsMessageCreationMemo &p);
    friend  QTextStream &operator>>(QTextStream &in, RecsMessageCreationMemo &p);

};


inline QTextStream &operator>>(QTextStream &in, RecsMessageCreationMemo &p);
inline QTextStream &operator<<(QTextStream &out, const RecsMessageCreationMemo &p);

}
