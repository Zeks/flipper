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


#include "include/favparser.h"
#include "include/section.h"
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
    qDebug() << "part started at: " << point.time_since_epoch()/std::chrono::seconds(1);;
    QRegExp rxFandom("(.*)(?=\\s-\\sRated:)");
    QRegExp rxWords("Words:\\s(\\d{1,8})");
    QRegExp rxChapters("Chapters:\\s(\\d{1,5})");
    QRegExp rxReviews("Reviews:\\s(\\d{1,5})");
    QRegExp rxFavs("Favs:\\s(\\d{1,5})");
    QRegExp rxPublished("Published:\\s<span\\sdata-xutime='(\\d+)'");
    QRegExp rxUpdated("Updated:\\s<span\\sdata-xutime='(\\d+)'");
    QRegExp rxRated("Rated:\\s(.{1})");
    QRegExp rxGenres("English\\s-\\s([A-Za-z/\\-]+)\\s-\\sChapters");
    QRegExp rxComplete("(Complete)$");
    while(true)
    {
        counter++;
        section = GetSection(str, currentPosition);
        if(sections.size() == 0 && section.isValid)
        {
            int parts = str.length()/(section.end-section.start);
            //qDebug() << "reserving parts: " << parts;
            sections.reserve(parts);
        }
        if(!section.isValid)
            break;
        currentPosition = section.start;


        //section.fandom = ui->rbNormal->isChecked() ? currentFandom: currentFandom + " CROSSOVER";
        GetTitle(section, currentPosition, str);
        GetUrl(section, currentPosition, str);
        GetAuthorId(section, currentPosition, str);
        GetAuthor(section, currentPosition, str);
        GetSummary(section, currentPosition, str);

        GetStatSection(section, currentPosition, str);
        auto statText = section.statSection.text.replace(",", "");
        GetTaggedSection(statText, rxFandom,    [&section](QString val){ section.result->fandom = val;});
        GetTaggedSection(statText, rxWords,      [&section](QString val){ section.result->wordCount = val;});
        GetTaggedSection(statText, rxChapters,   [&section](QString val){ section.result->chapters = val;});
        GetTaggedSection(statText, rxReviews,    [&section](QString val){ section.result->reviews = val;});
        GetTaggedSection(statText, rxFavs,       [&section](QString val){ section.result->favourites = val;});
        GetTaggedSection(statText, rxPublished, [&section](QString val){
            if(val != "not found")
                section.result->published.setTime_t(val.toInt()); ;
        });
        GetTaggedSection(statText, rxUpdated,  [&section](QString val){
            if(val != "not found")
                section.result->updated.setTime_t(val.toInt());
            else
                section.result->updated.setTime_t(0);
        });
        GetTaggedSection(statText, rxRated, [&section](QString val){ section.result->rated = val;});
        GetTaggedSection(statText, rxGenres, [&section](QString val){ section.result->SetGenres(val, "ffn");});
        GetCharacters(statText,  [&section](QString val){
            section.result->charactersFull = val;
        });
        GetTaggedSection(statText, rxComplete, [&section](QString val){
            if(val != "not found")
                section.result->complete = 1;
        });
        if(statText.contains("CROSSOVER", Qt::CaseInsensitive))
            GetCrossoverFandomList(section, currentPosition, str);
        else
            section.result->fandoms.push_back(section.result->fandom);

        if(section.isValid)
            sections.append(section.result);
    }


    //qDebug() << "parser iterations: " << counter;
    auto elapsed = std::chrono::high_resolution_clock::now() - point;
    qDebug() << "Part of size: " << str.length() << "Processed in: " << MicrosecondsToString(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());

    qDebug() << "part finished at: " << std::chrono::high_resolution_clock::now().time_since_epoch()/ std::chrono::seconds(1);;
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

QString FavouriteStoryParser::GetFandom(QString text)
{
    thread_local QRegExp rxStart(QRegExp::escape("xicon-section-arrow'></span>"));
    thread_local QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text, 0);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    return text.mid(indexStart + 28,indexEnd - (indexStart + 28));
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

void FavouriteStoryParser::WriteProcessed()
{
    //requires
    //database::WriteFandomsForStory(section, knownFandoms);
//    fanfics->ProcessIntoDataQueues(processedStuff);
//    fandoms = fandomsInterface->EnsureFandoms(parser.processedStuff);
//    QList<core::FicRecommendation> recommendations;
//    recommendations.reserve(processedStuff.size());
//    for(auto& section : processedStuff)
//        recommendations.push_back({section, recommender.author});
}



void FavouriteStoryParser::WriteRecommenderInfo()
{
    //database::WriteRecommendersMetainfo(recommender);
}

void FavouriteStoryParser::SetCurrentTag(QString value)
{
    currentTagMode = value;
}

void FavouriteStoryParser::SetAuthor(core::AuthorPtr author)
{
    recommender.author = author;
}

void FavouriteStoryParser::GetAuthorId(core::Section & section, int &startfrom, QString text)
{
    // looking for first href
    //QString currentSection = text.mid(startfrom);
//    QRegExp rxStart(QRegExp::escape("href=\""));
//    QRegExp rxEnd(QRegExp::escape("\">"));
//    int indexStart = rxStart.indexIn(text,startfrom);
////    if(indexStart == -1)
////        qDebug() << currentSection;
//    int indexEnd = rxEnd.indexIn(text, indexStart);

    thread_local QRegExp rx("(/u/(\\d+)/)(.*)(?=\">)");
    rx.setMinimal(true);
    auto index = rx.indexIn(text, startfrom);
    if(index == -1)
        return;
    bool ok = true;
    section.result->author->SetWebID("ffn", rx.cap(2).toInt(&ok)); // todo, needs checking
    auto capture = rx.cap();
    startfrom = index+rx.cap().size()+2;
}

void FavouriteStoryParser::GetAuthor(core::Section & section, int& startfrom, QString text)
{
    //QString currentStart = text.mid(startfrom);
    //QRegExp rxBy("\">");
    //QRegExp rxStart("</a>");
    thread_local QRegExp rxEnd(QRegExp::escape("</a>"));
    //int indexBy = rxBy.indexIn(text, startfrom);
    //QString search = text.mid(startfrom);
    int indexStart = startfrom;
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    QString name = text.mid(indexStart,indexEnd - (indexStart));
    section.result->author->name = name;

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

void FavouriteStoryParser::GetStatSection(core::Section &section, int &startfrom, QString text)
{
    thread_local QRegExp rxStart("padtop2\\sxgray");
    thread_local QRegExp rxEnd("</div></div></div>");
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    section.statSection.text = text.mid(indexStart + 15,indexEnd - (indexStart + 15));
    section.statSectionStart = indexStart + 15;
    section.statSectionEnd = indexEnd;
    //qDebug() << section.statSection;
}

void FavouriteStoryParser::GetSummary(core::Section & section, int& startfrom, QString text)
{
    thread_local QRegExp rxStart(QRegExp::escape("padtop'>"));
    thread_local QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);

    section.result->summary = text.mid(indexStart + 8,indexEnd - (indexStart + 8));
    section.summaryEnd = indexEnd;
    startfrom = indexEnd;
}

void FavouriteStoryParser::GetCrossoverFandomList(core::Section & section, int &startfrom, QString text)
{
    thread_local QRegExp rxStart("Crossover\\s-\\s");
    thread_local QRegExp rxEnd("\\s-\\sRated:");


    int indexStart = rxStart.indexIn(text, startfrom);
    if(indexStart != -1 )
    {
        section.result->fandom.replace("Crossover - ", "");
        section.result->fandom += " CROSSOVER";
    }

    int indexEnd = rxEnd.indexIn(text, indexStart + 1);

    QString tmp = text.mid(indexStart + (rxStart.pattern().length() -2), indexEnd - (indexStart + rxStart.pattern().length() - 2)).trimmed();
    section.result->fandom = tmp + QString(" CROSSOVER");
    section.result->fandoms = tmp.split(" & ", QString::SkipEmptyParts);
    startfrom = indexEnd;
}

void FavouriteStoryParser::GetUrl(core::Section & section, int& startfrom, QString text)
{
    // looking for first href
    thread_local QRegExp rxStart(QRegExp::escape("href=\""));
    thread_local QRegExp rxEnd(QRegExp::escape("\">"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int tempStart = indexStart;
    int indexEnd = rxEnd.indexIn(text, indexStart);
    int tempEnd = indexEnd;
    //QString tempUrl = text.mid(indexStart + 6,indexEnd - (indexStart + 6));
    indexStart = rxStart.indexIn(text,indexStart+1);
    indexEnd = rxEnd.indexIn(text, indexStart);
    QString secondUrl = text.mid(indexStart + 6,indexEnd - (indexStart + 6));
    if(!secondUrl.contains("/s/"))
    {
        indexStart = tempStart;
        indexEnd  = tempEnd;
    }
    QString tempUrl = text.mid(indexStart + 6,indexEnd - (indexStart + 6));
    if(tempUrl.length() > 200)
    {
        QString currentSection = text.mid(startfrom);
        //qDebug() << currentSection;
        //qDebug() << currentSection;
    }
    section.result->SetUrl("ffn",text.mid(indexStart + 6,indexEnd - (indexStart + 6)));
    QRegExp rxWebId("/s/(\\d+)");
    auto indexWeb = rxWebId.indexIn(section.result->url("ffn"));
    if(indexWeb != -1)
    {
        QString captured = rxWebId.cap(1);
        section.result->webId = captured.toInt();
        //qDebug() << "webId: " << section.result->webId;
        section.result->SetUrl("ffn", section.result->url("ffn").left(3 + captured.length()));
    }
    startfrom = indexEnd+2;
}



void FavouriteStoryParser::GetTaggedSection(QString text, QRegExp& rx, std::function<void (QString)> functor)
{
//    /QRegExp rx(rxString);
    int indexStart = rx.indexIn(text);
    if(indexStart != 1 && !rx.cap(1).trimmed().replace("-","").isEmpty())
        functor(rx.cap(1));
    else
        functor("not found");
}
#include "include/regex_utils.h"
void FavouriteStoryParser::GetCharacters(QString text,
                                         std::function<void (QString)> functor)
{
    //auto full = GetSingleNarrow(text,"</span>\\s-\\s", "</div></div></div>", true);
    auto full = GetDoubleNarrow(text,"^", "$", true,
                                "",  "</span>\\s-\\s", true,
                                10);


    //qDebug() << "Just characters:" << full;
    full = full.replace(" - Complete", "");
//    if(full.trimmed().isEmpty() && (text.midRef(40).contains("Naruto") || text.midRef(40).contains("Harry") ))
//    {
//        qDebug() << webId << " Full section: " << text;
//        functor(full);
//    }
    functor(full);
}


core::Section FavouriteStoryParser::GetSection(QString text, int start)
{
    core::Section section;
    thread_local QRegExp rxStart("<div\\sclass=\'z-list\\sfavstories\'");

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

QString FavouriteStoryParser::ExtractRecommenderNameFromUrl(QString url)
{
    int pos = url.lastIndexOf("/");
    return url.mid(pos+1);
}

