#include "discord/rec_message_recreation_token.h"
namespace discord{

inline QTextStream &operator>>(QTextStream &in, RecsMessageCreationMemo &token)
{
    QString temp;

    in >> temp;
    token.list.adjusting = temp.toInt();
    in >> temp;
    token.list.ficFavouritesCutoff = temp.toInt();
    in >> temp;
    token.list.ignoreBreakdowns = temp.toInt();
    in >> temp;
    token.list.isAutomatic = temp.toInt();
    in >> temp;
    token.list.listSizeMultiplier = temp.toInt();
    in >> temp;
    token.list.resultLimit = temp.toInt();
    in >> temp;
    token.list.ratioCutoff = temp.toInt();
    in >> temp;
    token.list.useMoodAdjustment = temp.toInt();
    in >> temp;
    token.list.useWeighting = temp.toInt();
    in >> temp;
    token.list.userFFNId = temp.toLongLong();

    auto& filter = token.filter;
    in >> temp;
    filter.allowUnfinished = temp.toInt();
    in >> temp;
    filter.crossoversOnly = temp.toInt();
    in >> temp;
    filter.deadFicDaysRange = temp.toInt();
    in >> temp;
    filter.ensureActive = temp.toInt();
    in >> temp;
    filter.ensureCompleted = temp.toInt();
    in >> temp;
    filter.fandom = temp.toInt();
    in >> temp;
    filter.ficDateFilter.mode = static_cast<filters::EDateFilterType>(temp.toInt());
    in >> temp;
    filter.ficDateFilter.dateStart = temp.toStdString();
    in >> temp;
    filter.ficDateFilter.dateEnd = temp.toStdString();
    in >> temp;
    filter.includeCrossovers = temp.toInt();
    in >> temp;
    filter.likedAuthorsEnabled = temp.toInt();
    in >> temp;
    filter.listOpenMode = temp.toInt();
    in >> temp;
    filter.maxFics = temp.toInt();
    in >> temp;
    filter.maxWords = temp.toInt();
    in >> temp;
    filter.minWords = temp.toInt();
    in >> temp;
    filter.mode = static_cast<core::StoryFilter::EFilterMode>(temp.toInt());
    in >> temp;
    filter.randomizeResults = temp.toInt();
    in >> temp;
    filter.rating = static_cast<core::StoryFilter::ERatingFilter>(temp.toInt());
    in >> temp;
    filter.recordLimit = temp.toInt();
    in >> temp;
    filter.recordPage = temp.toInt();
    in >> temp;
    filter.rngDisambiguator = temp;
    in >> temp;
    filter.secondFandom = temp.toInt();
    in >> temp;
    filter.sortMode = static_cast<core::StoryFilter::ESortMode>(temp.toInt());
    in >> temp;
    filter.wipeRngSequence = temp.toInt();
    return in;
}

inline QTextStream &operator<<(QTextStream &out, const RecsMessageCreationMemo & token)
{
    //out << token.originalMessage << " ";
    // dumping list creation params
    out << token.list.adjusting << " ";
    out << token.list.ficFavouritesCutoff << " ";
    out << token.list.ignoreBreakdowns << " ";
    out << token.list.isAutomatic << " ";
    out << token.list.listSizeMultiplier << " ";
    out << token.list.resultLimit << " ";
    out << token.list.ratioCutoff << " ";
    out << token.list.useMoodAdjustment << " ";
    out << token.list.useWeighting << " ";
    out << token.list.userFFNId << " ";
    // dumping filter
    auto& filter = token.filter;
    out << filter.allowUnfinished << " ";
    out << filter.crossoversOnly  << " ";
    out << filter.deadFicDaysRange  << " ";
    out << filter.ensureActive << " ";
    out << filter.ensureCompleted << " ";
    out << filter.fandom << " ";
    out << filter.ficDateFilter.mode << " ";
    out << QString::fromStdString(filter.ficDateFilter.dateStart) << " ";
    out << QString::fromStdString(filter.ficDateFilter.dateEnd) << " ";
    out << filter.includeCrossovers << " ";
    out << filter.likedAuthorsEnabled << " ";
    out << filter.listOpenMode << " ";
    out << filter.maxFics << " ";
    out << filter.maxWords << " ";
    out << filter.minWords << " ";
    out << filter.mode << " ";
    out << filter.randomizeResults << " ";
    out << filter.rating << " ";
    out << filter.recordLimit << " ";
    out << filter.recordPage << " ";
    out << filter.rngDisambiguator << " ";
    out << filter.secondFandom << " ";
    out << filter.sortMode << " ";
    out << filter.wipeRngSequence << " ";
    return out;
}
}
