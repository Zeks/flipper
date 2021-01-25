#pragma once
#include "core/recommendation_list.h"
#include "include/storyfilter.h"
#include <QTextStream>
namespace discord{

struct RecsMessageCreationMemo{
    int64_t originalMessage;
    core::RecommendationList list;
    core::StoryFilter filter;

    friend  QTextStream &operator<<(QTextStream &out, const RecsMessageCreationMemo &p);
    friend  QTextStream &operator>>(QTextStream &in, RecsMessageCreationMemo &p);

};


inline QTextStream &operator>>(QTextStream &in, RecsMessageCreationMemo &p);
inline QTextStream &operator<<(QTextStream &out, const RecsMessageCreationMemo &p);

}
