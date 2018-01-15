/*
FFSSE is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
