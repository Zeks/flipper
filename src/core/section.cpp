/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#include "include/core/section.h"
#include <QDebug>

//core::FicSectionStats core::Author::MergeStats(QList<core::AuthorPtr> authors)
//{

//    for(auto author : authors)
//    {

//    }
//}




core::Section::Section():result(new core::Fic())
{

}
void core::Fic::LogUrls()
{
    qDebug() << "Urls:" ;
    for(auto key : urls.keys())
    {
        if(!key.trimmed().isEmpty())
            qDebug() << key << " " << urls[key];
    }
}
void core::Fic::LogWebIds()
{
    qDebug() << "WebIds:" ;
    qDebug() << "webId: " << identity.web.ffn;
    if(identity.web.ffn != -1)
        qDebug() << "FFN: " << identity.web.ffn;
    if(identity.web.ao3 != -1)
        qDebug() << "Ao3: " << identity.web.ao3;
    if(identity.web.sb!= -1)
        qDebug() << "SB: " << identity.web.sb;
    if(identity.web.sv != -1)
        qDebug() << "SV: " << identity.web.sv;
}

core::Fic::Statistics core::Fic::getCalcStats() const
{
    return statistics;
}

void core::Fic::setCalcStats(const Statistics &value)
{
    statistics = value;
}

void core::Fic::SetUrl(QString type, QString url)
{
    urls[type] = url;
    urlFFN = url;
}
void core::Fic::Log()
{
    //qDebug() << "Author:"  << author->GetWebIds() << author->name
    qDebug() << "//////////////////////////////////////";
    qDebug() << "Author: ";
    if(author)
        author->Log();
    else
        qDebug() << "Author pointer invalid";

    LogUrls();
    qDebug() << "Title:" << title;
    qDebug() << "Fandoms:" << fandoms.join(" + ");
    qDebug() << "Genre:" << genres.join(",");
    qDebug() << "Published:" << published;
    qDebug() << "Updated:" << updated;
    qDebug() << "Characters:" << charactersFull;

    qDebug() << "WordCount:" << wordCount;
    qDebug() << "Chapters:" << chapters;
    qDebug() << "Reviews:" << reviews;
    qDebug() << "Favourites:" << favourites;
    //qDebug() << "Follows:" << follows; // not yet parsed
    qDebug() << "Rated:" << rated;
    qDebug() << "Complete:" << complete;
    qDebug() << "AtChapter:" << userData.atChapter;
    qDebug() << "Is Crossover:" << isCrossover;
    qDebug() << "Summary:" << summary;
    qDebug() << "StatSection:" << statSection;
    qDebug() << "Tags:" << userData.tags;
    qDebug() << "Language:" << language;

    qDebug() << "Recommendations main:" << recommendationsData.recommendationsMainList;
    qDebug() << "Recommendations second:" << recommendationsData.recommendationsSecondList;
    qDebug() << "Place main:" << recommendationsData.placeInMainList;
    qDebug() << "Place second:" << recommendationsData.placeInSecondList;
    LogWebIds();
    statistics.Log();
    qDebug() << "//////////////////////////////////////";
}

void core::Fic::Statistics::Log()
{
    if(!isValid)
    {
        qDebug() << "Statistics not yet calculated";
        return;
    }
    qDebug() << "Wcr:" << wcr;
    qDebug() << "Wcr adjusted:" << wcr_adjusted;
    qDebug() << "ReviewsTofavourites:" << reviewsTofavourites;
    qDebug() << "Age:" << age;
    qDebug() << "DaysRunning:" << daysRunning;
}

void core::RecommendationList:: Log()
{

    qDebug() << "List id: " << id ;
    qDebug() << "name: " << name ;
    qDebug() << "automatic: " << isAutomatic;
    qDebug() << "useWeighting: " << useWeighting;
    qDebug() << "useDislikes: " << useDislikes;
    qDebug() << "useDeadFicIgnore: " << useDeadFicIgnore;
    qDebug() << "useMoodAdjustment: " << useMoodAdjustment;
    qDebug() << "hasAuxDataFilled: " << hasAuxDataFilled;
    qDebug() << "maxUnmatchedPerMatch: " << maxUnmatchedPerMatch;
    qDebug() << "ficCount: " << ficCount ;
    qDebug() << "tagToUse: " << tagToUse ;
    qDebug() << "minimumMatch: " << minimumMatch ;
    qDebug() << "alwaysPickAt: " << alwaysPickAt ;
    qDebug() << "pickRatio: " << maxUnmatchedPerMatch ;
    qDebug() << "created: " << created ;
    //qDebug() << "source fics: " << ficData.sourceFics;
}

void core::RecommendationList::PassSetupParamsInto(RecommendationList &other)
{
    other.isAutomatic = isAutomatic;
    other.useWeighting = useWeighting;
    other.useMoodAdjustment = useMoodAdjustment;
    other.useDislikes = useDislikes;
    other.useDeadFicIgnore= useDeadFicIgnore;
    other.minimumMatch = minimumMatch;
    other.alwaysPickAt = alwaysPickAt;
    other.maxUnmatchedPerMatch = maxUnmatchedPerMatch;
    other.name = name;
}

void core::AuthorStats::Serialize(QDataStream &out)
{
    out << pageCreated;
    out << bioLastUpdated;
    out << favouritesLastUpdated;
    out << favouritesLastChecked;
    out << bioWordCount;

    favouriteStats.Serialize(out);
    ownFicStats.Serialize(out);
}

void core::AuthorStats::Deserialize(QDataStream &in)
{
    in >> pageCreated;
    in >> bioLastUpdated;
    in >> favouritesLastUpdated;
    in >> favouritesLastChecked;
    in >> bioWordCount;

    favouriteStats.Deserialize(in);
    ownFicStats.Deserialize(in);
}


QString core::Fandom::GetName() const
{
    return this->name;
}
