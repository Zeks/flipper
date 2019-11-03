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



void core::Author::Log()
{

    qDebug() << "id: " << id;
    LogWebIds();
    qDebug() << "name: " << name;
    qDebug() << "valid: " << isValid;
    qDebug() << "idStatus: " << static_cast<int>(idStatus);
    qDebug() << "ficCount: " << ficCount;
    qDebug() << "favCount: " << favCount;
    qDebug() << "lastUpdated: " << lastUpdated;
    qDebug() << "firstPublishedFic: " << firstPublishedFic;
}

void core::Author::LogWebIds()
{
    qDebug() << "Author WebIds:" ;
    for(auto key : webIds.keys())
    {
        if(!key.trimmed().isEmpty())
            qDebug() << key << " " << webIds[key];
    }
}

QString core::Author::CreateAuthorUrl(QString urlType, int webId) const
{
    if(urlType == "ffn")
        return "https://www.fanfiction.net/u/" + QString::number(webId);
    return QString();
}

QStringList core::Author::GetWebsites() const
{
    return webIds.keys();
}

void core::Author::Serialize(QDataStream &out)
{
    out << hasChanges;
    out << id;
    out << static_cast<int>(idStatus);
    out << name;
    out << firstPublishedFic;
    out << lastUpdated;
    out << ficCount;
    out << recCount;
    out << favCount;
    out << isValid;
    out << webIds;
    out << static_cast<int>(updateMode);

    stats.Serialize(out);
}

void core::Author::Deserialize(QDataStream &in)
{
    in >> hasChanges;
    in >> id;
    int temp;
    in >> temp;
    idStatus = static_cast<AuthorIdStatus>(temp);

    in >> name;
    in >> firstPublishedFic;
    in >> lastUpdated;
    in >> ficCount;
    in >> recCount;
    in >> favCount;
    in >> isValid;
    in >> webIds;
    in >> temp;
    updateMode = static_cast<UpdateMode>(temp);

    stats.Deserialize(in);
}



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
    qDebug() << "webId: " << webId;
    if(ffn_id != -1)
        qDebug() << "FFN: " << ffn_id;
    if(ao3_id != -1)
        qDebug() << "Ao3: " << ao3_id;
    if(sb_id != -1)
        qDebug() << "SB: " << sb_id;
    if(sv_id != -1)
        qDebug() << "SV: " << sv_id;
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
    qDebug() << "AtChapter:" << atChapter;
    qDebug() << "Is Crossover:" << isCrossover;
    qDebug() << "Summary:" << summary;
    qDebug() << "StatSection:" << statSection;
    qDebug() << "Tags:" << tags;
    qDebug() << "Language:" << language;
    qDebug() << "Summary:" << summary;
    LogWebIds();
    calcStats.Log();
    qDebug() << "//////////////////////////////////////";
}

void core::Fic::FicCalcStats::Log()
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
    qDebug() << "automaic: " << isAutomatic;
    qDebug() << "ficCount: " << ficCount ;
    qDebug() << "tagToUse: " << tagToUse ;
    qDebug() << "minimumMatch: " << minimumMatch ;
    qDebug() << "maximumNegativeMatches: " << maximumNegativeMatches;
    qDebug() << "alwaysPickAt: " << alwaysPickAt ;
    qDebug() << "pickRatio: " << maxUnmatchedPerMatch ;
    qDebug() << "created: " << created ;
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

void core::FicSectionStats::Serialize(QDataStream &out)
{
    out << favourites;
    out << ficWordCount;
    out << averageWordsPerChapter;
    out << averageLength;
    out << fandomsDiversity;
    out << explorerFactor;
    out << megaExplorerFactor;
    out << crossoverFactor;
    out << unfinishedFactor;
    out << esrbUniformityFactor;
    out << esrbKiddy;
    out << esrbMature;
    out << genreDiversityFactor;
    out << moodUniformity;
    out << moodNeutral;
    out << moodSad;
    out << moodHappy;
    out << crackRatio;
    out << slashRatio;
    out << notSlashRatio;
    out << smutRatio;
    out << static_cast<int>(esrbType);
    out << static_cast<int>(prevalentMood);
    out << static_cast<int>(mostFavouritedSize);
    out << static_cast<int>(sectionRelativeSize);

    out << prevalentGenre;
    out << sizeFactors;
    out << fandoms;
    out << fandomFactors;
    out << fandomsConverted;
    out << fandomFactorsConverted;
    out << genreFactors;

    out << firstPublished;
    out << lastPublished;

}

void core::FicSectionStats::Deserialize(QDataStream &in)
{
    in >>  favourites;
    in >>  ficWordCount;
    in >>  averageWordsPerChapter;
    in >>  averageLength;
    in >>  fandomsDiversity;
    in >>  explorerFactor;
    in >>  megaExplorerFactor;
    in >>  crossoverFactor;
    in >>  unfinishedFactor;
    in >>  esrbUniformityFactor;
    in >>  esrbKiddy;
    in >>  esrbMature;
    in >>  genreDiversityFactor;
    in >>  moodUniformity;
    in >>  moodNeutral;
    in >>  moodSad;
    in >>  moodHappy;
    in >>  crackRatio;
    in >>  slashRatio;
    in >>  notSlashRatio;
    in >>  smutRatio;
    int temp;
    in >>  temp;
    esrbType = static_cast<ESRBType>(temp);
    in >>  temp;
    prevalentMood = static_cast<MoodType>(temp);
    in >>  temp;
    mostFavouritedSize = static_cast<EntitySizeType>(temp);
    in >>  temp;
    sectionRelativeSize = static_cast<EntitySizeType>(temp);

    in >>  prevalentGenre;
    in >>  sizeFactors;
    in >>  fandoms;
    in >>  fandomFactors;
    in >>  fandomsConverted;
    in >>  fandomFactorsConverted;
    in >>  genreFactors;

    in >>  firstPublished;
    in >>  lastPublished;
}

