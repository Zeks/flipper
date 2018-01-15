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


#include "include/parsers/ffn/favparser.h"
#include "include/core/section.h"
#include "include/pure_sql.h"
#include "include/url_utils.h"
#include <QDebug>
#include <QSqlDatabase>
#include <chrono>
#include <algorithm>

FavouriteStoryParser::FavouriteStoryParser(QSharedPointer<interfaces::Fanfics> fanfics)
    : FFNParserBase(fanfics)
{

}
static QString MicrosecondsToString(int value) {
    QString decimal = QString::number(value/1000000);
    int offset = decimal == "0" ? 0 : decimal.length();
    QString partial = QString::number(value).mid(offset,1);
    return decimal + "." + partial;}

void ReserveSpaceForSections(QList<QSharedPointer<core::Fic>>& sections,  core::Section& section, QString& str)
{
    int parts = str.length()/(section.end-section.start);
    sections.reserve(parts);
}

QList<QSharedPointer<core::Fic> > FavouriteStoryParser::ProcessPage(QString url, QString& str)
{

    QList<QSharedPointer<core::Fic>>  sections;
    core::Section section;
    int currentPosition = 0;
    int counter = 0;

    core::AuthorPtr author(new core::Author);
    section.result->author = author;
    recommender.author = author;

    recommender.author->name = authorName;
    recommender.author->SetWebID("ffn", url_utils::GetWebId(url, "ffn").toInt());
    auto point = std::chrono::high_resolution_clock::now();
    //qDebug() << "part started at: " << point.time_since_epoch()/std::chrono::seconds(1);;

    while(true)
    {
        counter++;
        section = GetSection(str, "<div\\sclass=\'z-list\\sfavstories\'", currentPosition);
        if(!section.isValid)
            break;

        if(sections.size() == 0)
            ReserveSpaceForSections(sections, section, str);

        currentPosition = section.start;
        ProcessSection(section, currentPosition, str);

        // can be set invalid during the parse
        if(section.isValid)
            sections.append(section.result);
    }

    //qDebug() << "parser iterations: " << counter;
    auto elapsed = std::chrono::high_resolution_clock::now() - point;
    //qDebug() << "Part of size: " << str.length() << "Processed in: " << MicrosecondsToString(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());

    //qDebug() << "part finished at: " << std::chrono::high_resolution_clock::now().time_since_epoch()/ std::chrono::seconds(1);;
    if(sections.size() == 0)
    {
        diagnostics.push_back("<span> No data found on the page.<br></span>");
        diagnostics.push_back("<span> \nFinished loading data <br></span>");
    }

    //qDebug() << "Processed fic, count:  " << sections.count();
    processedStuff+=sections;
    currentPosition = 999;
    return sections;
}

void FavouriteStoryParser::ClearProcessed()
{
    processedStuff.clear();
    recommender = core::FavouritesPage();
}

void FavouriteStoryParser::ClearDoneCache()
{
    alreadyDone.clear();
}

void FavouriteStoryParser::SetCurrentTag(QString value)
{
    currentTagMode = value;
}

void FavouriteStoryParser::SetAuthor(core::AuthorPtr author)
{
    recommender.author = author;
}

QString FavouriteStoryParser::ExtractRecommenderNameFromUrl(QString url)
{
    int pos = url.lastIndexOf("/");
    return url.mid(pos+1);
}

void FavouriteStoryParser::GetFandomFromTaggedSection(core::Section & section, QString text)
{
    thread_local QRegExp rxFandom("(.*)(?=\\s-\\sRated:)");
    GetTaggedSection(text, rxFandom,    [&section](QString val){ section.result->fandom = val;});
}

void FavouriteStoryParser::GetTitle(core::Section & section, int& startfrom, QString text)
{
    thread_local QRegExp rxStart(QRegExp::escape("data-title=\""));
    thread_local QRegExp rxEnd(QRegExp::escape("\""));
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart+13);
    startfrom = indexEnd;
    section.result->title = text.mid(indexStart + 12,indexEnd - (indexStart + 12));
    section.result->title=section.result->title.replace("\\'","'");
    section.result->title=section.result->title.replace("\'","'");
}


void FavouriteStoryParser::GetTitleAndUrl(core::Section & section, int& currentPosition, QString str)
{
    GetTitle(section, currentPosition, str);
    GetUrl(section, currentPosition, str);
}
