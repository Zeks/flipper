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


QDate GetRealMinDate(QList<QDate> dates)
{
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
        section = GetSection(str, currentPosition);
        if(!section.isValid)
            break;
        currentPosition = section.start;

        section.result->fandom = page.crossover ? page.fandom + " CROSSOVER" : page.fandom;
        section.result->author->website = "ffn";

        GetUrl(section, currentPosition, str);
        section.result->webId = section.result->ffn_id = url_utils::GetWebId(section.result->url("ffn"), "ffn").toInt();
        GetTitle(section, currentPosition, str);
        //GetAuthor(section, currentPosition, str);
        GetAuthor(section, currentPosition, str);
        GetSummary(section, currentPosition, str);

        GetStatSection(section, currentPosition, str);

        QString statText = section.statSection.text;
        GetTaggedSection(statText.replace(",", ""), "Words:\\s(\\d{1,8})", [&section](QString val){ section.result->wordCount = val;});
        GetTaggedSection(statText.replace(",", ""), "Chapters:\\s(\\d{1,5})", [&section](QString val){ section.result->chapters = val;});
        GetTaggedSection(statText.replace(",", ""), "Reviews:\\s(\\d{1,5})", [&section](QString val){ section.result->reviews = val;});
        GetTaggedSection(statText.replace(",", ""), "Favs:\\s(\\d{1,5})", [&section](QString val){ section.result->favourites = val;});
        GetTaggedSection(statText, "Published:\\s<span\\sdata-xutime='(\\d+)'", [&section](QString val){
            if(val != "not found")
                section.result->published.setTime_t(val.toInt()); ;
        });
        GetTaggedSection(statText, "Updated:\\s<span\\sdata-xutime='(\\d+)'", [&section](QString val){
            if(val != "not found")
                section.result->updated.setTime_t(val.toInt());
            else
                section.result->updated.setTime_t(0);
        });
        GetTaggedSection(statText, "Rated:\\s(.{1})", [&section](QString val){ section.result->rated = val;});
        GetTaggedSection(statText, "English\\s-\\s([A-Za-z/\\-]+)\\s-\\sChapters", [this,&section](QString val){
            ProcessGenres(section, val);
        });
        GetTaggedSection(statText, "</span>\\s-\\s([A-Za-z\\.\\s/]+)$", [this,&section](QString val){
            ProcessCharacters(section, val.replace(" - Complete", ""));
        });
        GetTaggedSection(statText, "(Complete)$", [&section](QString val){
            if(val != "not found")
                section.result->complete = 1;
        });

        if(section.result->fandom.contains("CROSSOVER"))
            GetCrossoverFandomList(section, currentPosition, str);
        else
            section.result->fandoms.push_back(section.result->fandom);


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
        nextUrl = GetNext(sections.last(), currentPosition, str);
}

QString FandomParser::GetFandom(QString text)
{
    QRegExp rxStart(QRegExp::escape("xicon-section-arrow'></span>"));
    QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text, 0);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    return text.mid(indexStart + 28,indexEnd - (indexStart + 28));
}

void FandomParser::WriteProcessed()
{
    fanfics->ProcessIntoDataQueues(processedStuff);
    fanfics->FlushDataQueues();
}

void FandomParser::ClearProcessed()
{
    processedStuff = decltype(processedStuff)();
}

//void FandomParser::GetAuthor(core::Section & section, int& startfrom, QString text)
//{
//    QRegExp rxBy("by\\s<");
//    QRegExp rxStart(">");
//    QRegExp rxEnd(QRegExp::escape("</a>"));
//    int indexBy = rxBy.indexIn(text, startfrom);
//    int indexStart = rxStart.indexIn(text, indexBy + 3);
//    int indexEnd = rxEnd.indexIn(text, indexStart);
//    startfrom = indexEnd;
//    section.result->author->name = text.mid(indexStart + 1,indexEnd - (indexStart + 1));

//}

void FandomParser::GetAuthor(core::Section & section, int &startfrom,  QString text)
{
    text = text.mid(startfrom);
    auto full = GetDoubleNarrow(text,"/u/\\d+/", "</a>", true,
                                "",  "\">", false,
                                2);

    QRegExp rxEnd("(/u/(\\d+)/)(.*)(?='>)");
    rxEnd.setMinimal(true);
    auto index = rxEnd.indexIn(text);
    if(index == -1)
        return;

    QSharedPointer<core::Author> author(new core::Author);
    section.result->author = author;
    section.result->author->SetUrl("ffn",rxEnd.cap(1));
    section.result->author->webId = rxEnd.cap(2).toInt();
    section.result->author->name = full;

}


void FandomParser::GetTitle(core::Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape(">"));
    QRegExp rxEnd(QRegExp::escape("</a>"));
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.result->title = text.mid(indexStart + 1,indexEnd - (indexStart + 1));
    qDebug() << section.result->title;
}

void FandomParser::GetStatSection(core::Section &section, int &startfrom, QString text)
{
    QRegExp rxStart("padtop2\\sxgray");
    QRegExp rxEnd("</div></div></div>");
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    section.statSection.text = text.mid(indexStart + 15,indexEnd - (indexStart + 15));
    section.statSectionStart = indexStart + 15;
    section.statSectionEnd = indexEnd;
    //qDebug() << section.statSection;
}

void FandomParser::GetSummary(core::Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape("padtop'>"));
    QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);

    section.result->summary = text.mid(indexStart + 8,indexEnd - (indexStart + 8));
    section.summaryEnd = indexEnd;
    startfrom = indexEnd;
}

void FandomParser::GetCrossoverFandomList(core::Section & section, int &startfrom, QString text)
{
    QRegExp rxStart("Crossover\\s-\\s");
    QRegExp rxEnd("\\s-\\sRated:");

    int indexStart = rxStart.indexIn(text, startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart + 1);

    QString tmp = text.mid(indexStart + (rxStart.pattern().length() -2), indexEnd - (indexStart + rxStart.pattern().length() - 2)).trimmed();
    section.result->fandom = tmp + QString(" CROSSOVER");
    section.result->fandoms = tmp.split("&");
    startfrom = indexEnd;
}

void FandomParser::GetUrl(core::Section & section, int& startfrom, QString text)
{
    // looking for first href
    QRegExp rxStart(QRegExp::escape("href=\""));
    QRegExp rxEnd(QRegExp::escape("\"><img"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    section.result->SetUrl("ffn",text.mid(indexStart + 6,indexEnd - (indexStart + 6)));
    startfrom = indexEnd+2;
}

QString FandomParser::GetNext(core::Section & section, int &startfrom, QString text)
{
    QString nextUrl;
    QRegExp rxEnd(QRegExp::escape("Next &#187"));
    int indexEnd = rxEnd.indexIn(text, startfrom);
    int posHref = indexEnd - 400 + text.midRef(indexEnd - 400,400).lastIndexOf("href='");
    //int indexStart = rxStart.indexIn(text,section.start);
    //indexStart = rxStart.indexIn(text,indexStart+1);
    //indexStart = rxStart.indexIn(text,indexStart+1);
    nextUrl = CreateURL(text.mid(posHref+6, indexEnd - (posHref+6)));
    if(!nextUrl.contains("&p="))
        nextUrl = "";
    indexEnd = rxEnd.indexIn(text, startfrom);
    return nextUrl;
}

void FandomParser::GetTaggedSection(QString text, QString rxString ,std::function<void (QString)> functor)
{
    QRegExp rx(rxString);
    int indexStart = rx.indexIn(text);
    //qDebug() << rx.capturedTexts();
    if(indexStart != 1 && !rx.cap(1).trimmed().replace("-","").isEmpty())
        functor(rx.cap(1));
    else
        functor("not found");
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


core::Section FandomParser::GetSection(QString text, int start)
{
    //QSharedPointer<core::Section> section (new core::Section);
    core::Section section ;
    QRegExp rxStart("<div\\sclass=\'z-list\\szhover\\szpointer\\s\'");
    int index = rxStart.indexIn(text, start);
    if(index != -1)
    {
        section.isValid = true;
        section.start = index;
        int end = rxStart.indexIn(text, index+1);
        if(end == -1)
            end = index + 2000;
        section.end = end;
    }
    return section;
}
QString FandomParser::CreateURL(QString str)
{
    return "https://www.fanfiction.net/" + str;
}
