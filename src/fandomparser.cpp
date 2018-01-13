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
#include "fandomparser.h"
#include "Interfaces/fanfics.h"
#include "url_utils.h"
#include "regex_utils.h"


static QDate GetRealMinDate(QList<QDate> dates)
{
    if(dates.size() == 0)
        return QDate();
    std::sort(std::begin(dates), std::end(dates));
    QDate medianDate = dates[dates.size()/2];
    QHash<QDate, int> distances;
    int counter = 0;
    int totalDistance = 0;
    for(auto date : dates)
    {
        int distance = std::abs(date.daysTo(medianDate));
        totalDistance+=distance;
        counter++;
    }
    if(counter == 0)
        return QDateTime::currentDateTimeUtc().date();

    double average = static_cast<double>(totalDistance)/static_cast<double>(counter);

    QDate minDate = medianDate;
    for(auto date : dates)
    {
        if(date < minDate && std::abs(date.daysTo(medianDate)) <= average)
            minDate = date;
    }
    return minDate;
}


FandomParser::FandomParser(QSharedPointer<interfaces::Fanfics> fanfics)
    : FFNParserBase(fanfics)
{

}

void FandomParser::ProcessPage(WebPage page)
{
    processedStuff.clear();
    minSectionUpdateDate = QDateTime::currentDateTimeUtc().date();
    QString& str = page.content;
    core::Section section;
    int currentPosition = 0;
    int counter = 0;
    QList<core::Section> sections;
    QList<QDate> updateDates;

    while(true)
    {
        counter++;
        section = GetSection(str, "<div\\sclass=\'z-list\\szhover\\szpointer\\s\'" ,currentPosition);
        if(!section.isValid)
            break;
        currentPosition = section.start;

        section.result->fandom = page.crossover ? page.fandom + " CROSSOVER" : page.fandom;

        ProcessSection(section, currentPosition, str);

        if(section.isValid)
        {
            auto updateDate = section.result->updated.date();
            if(updateDate.year() > 1990)
                updateDates.push_back(updateDate);
            processedStuff.append(section.result);
            sections.push_back(section);
        }

    }
    minSectionUpdateDate = GetRealMinDate(updateDates);

    if(sections.size() > 0)
        nextUrl = GetNext(currentPosition, str);
}

void FandomParser::ClearProcessed()
{
    processedStuff = decltype(processedStuff)();
}

QString FandomParser::GetNext( int &startfrom, QString text)
{
    QString nextUrl;
    QRegExp rxEnd(QRegExp::escape("Next &#187"));
    int indexEnd = rxEnd.indexIn(text, startfrom);
    int posHref = indexEnd - 400 + text.midRef(indexEnd - 400,400).lastIndexOf("href='");
    nextUrl = CreateURL(text.mid(posHref+6, indexEnd - (posHref+6)));
    if(!nextUrl.contains("&p="))
        nextUrl = "";
    indexEnd = rxEnd.indexIn(text, startfrom);
    return nextUrl;
}

QString FandomParser::GetLast(QString pageContent)
{
    QString lastUrl;
    QRegExp rxEnd(QRegExp::escape("Last</a"));
    int indexEnd = rxEnd.indexIn(pageContent);
    indexEnd-=2;
    int posHref = indexEnd - 400 + pageContent.midRef(indexEnd - 400,400).lastIndexOf("href='");
    lastUrl = CreateURL(pageContent.mid(posHref+6, indexEnd - (posHref+6)));
    if(!lastUrl.contains("&p="))
        lastUrl = "";
    indexEnd = rxEnd.indexIn(pageContent);
    return lastUrl;
}

QString FandomParser::CreateURL(QString str)
{
    return "https://www.fanfiction.net/" + str;
}
void FandomParser::GetTitle(core::Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape(">"));
    QRegExp rxEnd(QRegExp::escape("</a>"));
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.result->title = text.mid(indexStart + 1,indexEnd - (indexStart + 1));
//    /qDebug() << section.result->title;
}

void FandomParser::GetTitleAndUrl(core::Section & section, int& currentPosition, QString str)
{
    GetUrl(section, currentPosition, str);
    GetTitle(section, currentPosition, str);
}
