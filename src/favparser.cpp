/* FFSSE program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see  <http://www.gnu.org/licenses/>*/


#include "include/favparser.h"
#include "include/section.h"
#include "include/pure_sql.h"
#include <QDebug>
#include <QSqlDatabase>
#include <chrono>

FavouriteStoryParser::FavouriteStoryParser(QSharedPointer<interfaces::Fanfics> fanfics)
                    : FFNParserBase(fanfics)
{

}

QList<QSharedPointer<core::Fic> > FavouriteStoryParser::ProcessPage(QString url, QString& str)
{
    core::Section section;
    int currentPosition = 0;
    int counter = 0;
    QList<QSharedPointer<core::Fic>> sections;
    recommender.author->name = authorName;
    recommender.author->SetUrl("ffn", url);
    recommender.author->website = "ffn";
    while(true)
    {

        counter++;
        section = GetSection(str, currentPosition);
        if(!section.isValid)
            break;
        currentPosition = section.start;


        //section.fandom = ui->rbNormal->isChecked() ? currentFandom: currentFandom + " CROSSOVER";
        GetTitle(section, currentPosition, str);
        GetUrl(section, currentPosition, str);
        GetAuthorUrl(section, currentPosition, str);
        GetAuthor(section, currentPosition, str);
        GetSummary(section, currentPosition, str);

        GetStatSection(section, currentPosition, str);
        auto statText = section.statSection.text;
        GetTaggedSection(statText.replace(",", ""), "(.*)(?=\\s-\\sRated:)", [&section](QString val){ section.result->fandom = val;});
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
        GetTaggedSection(statText, "English\\s-\\s([A-Za-z/\\-]+)\\s-\\sChapters", [&section](QString val){ section.result->SetGenres(val, "ffn");});
        GetTaggedSection(statText, "</span>\\s-\\s([A-Za-z\\.\\s/]+)$", [&section](QString val){
            section.result->charactersFull = val.replace(" - Complete", "");
        });
        GetTaggedSection(statText, "(Complete)$", [&section](QString val){
            if(val != "not found")
                section.result->complete = 1;
        });
        if(statText.contains("CROSSOVER", Qt::CaseInsensitive))
            GetCrossoverFandomList(section, currentPosition, str);
        section.result->origin = url;
        if(section.isValid)
        {
            sections.append(section.result);
        }

    }

    if(sections.size() == 0)
    {
        diagnostics.push_back("<span> No data found on the page.<br></span>");
        diagnostics.push_back("<span> \nFinished loading data <br></span>");
    }

    qDebug() << "Processed fic, count:  " << sections.count();
    processedStuff+=sections;
    currentPosition = 999;
    return sections;
}

QString FavouriteStoryParser::GetFandom(QString text)
{
    QRegExp rxStart(QRegExp::escape("xicon-section-arrow'></span>"));
    QRegExp rxEnd(QRegExp::escape("<div"));
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
    fanfics->ProcessIntoDataQueues(processedStuff);
    QList<core::FicRecommendation> recommendations;
    recommendations.reserve(processedStuff.size());
    for(auto& section : processedStuff)
        recommendations.push_back({section, recommender.author});
}



void FavouriteStoryParser::WriteRecommenderInfo()
{
    //database::WriteRecommendersMetainfo(recommender);
}

void FavouriteStoryParser::SetCurrentTag(QString value)
{
    currentTagMode = value;
}

void FavouriteStoryParser::GetAuthorUrl(core::Section & section, int &startfrom, QString text)
{
    // looking for first href
    //QString currentSection = text.mid(startfrom);
    QRegExp rxStart(QRegExp::escape("href=\""));
    QRegExp rxEnd(QRegExp::escape("\">"));
    int indexStart = rxStart.indexIn(text,startfrom);
//    if(indexStart == -1)
//        qDebug() << currentSection;
    int indexEnd = rxEnd.indexIn(text, indexStart);
    section.result->author->SetUrl("ffn","https://www.fanfiction.net" + text.mid(indexStart + 6,indexEnd - (indexStart + 6)));
    startfrom = indexEnd+2;
}

void FavouriteStoryParser::GetAuthor(core::Section & section, int& startfrom, QString text)
{
    //QString currentStart = text.mid(startfrom);
    //QRegExp rxBy("\">");
    //QRegExp rxStart("</a>");
    QRegExp rxEnd(QRegExp::escape("</a>"));
    //int indexBy = rxBy.indexIn(text, startfrom);
    int indexStart = startfrom;
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.result->author->name = text.mid(indexStart,indexEnd - (indexStart));

}



void FavouriteStoryParser::GetTitle(core::Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape("data-title=\""));
    QRegExp rxEnd(QRegExp::escape("\""));
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart+13);
    startfrom = indexEnd;
    section.result->title = text.mid(indexStart + 12,indexEnd - (indexStart + 12));
    section.result->title=section.result->title.replace("\\'","'");
    section.result->title=section.result->title.replace("\'","'");
}

void FavouriteStoryParser::GetStatSection(core::Section &section, int &startfrom, QString text)
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

void FavouriteStoryParser::GetSummary(core::Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape("padtop'>"));
    QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);

    section.result->summary = text.mid(indexStart + 8,indexEnd - (indexStart + 8));
    section.summaryEnd = indexEnd;
    startfrom = indexEnd;
}

void FavouriteStoryParser::GetCrossoverFandomList(core::Section & section, int &startfrom, QString text)
{
    QRegExp rxStart("Crossover\\s-\\s");
    QRegExp rxEnd("\\s-\\sRated:");


    int indexStart = rxStart.indexIn(text, startfrom);
    if(indexStart != -1 )
    {
        section.result->fandom.replace("Crossover - ", "");
        section.result->fandom += " CROSSOVER";
    }

    int indexEnd = rxEnd.indexIn(text, indexStart + 1);

    section.result->fandom = text.mid(indexStart + (rxStart.pattern().length() -2), indexEnd - (indexStart + rxStart.pattern().length() - 2)).trimmed() + QString(" CROSSOVER");
    startfrom = indexEnd;
}

void FavouriteStoryParser::GetUrl(core::Section & section, int& startfrom, QString text)
{
    // looking for first href
    QRegExp rxStart(QRegExp::escape("href=\""));
    QRegExp rxEnd(QRegExp::escape("\">"));
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
        section.result->SetUrl("ffn", section.result->url("ffn").left(3 + captured.length()));
    }
    startfrom = indexEnd+2;
}



void FavouriteStoryParser::GetTaggedSection(QString text, QString rxString ,std::function<void (QString)> functor)
{
    QRegExp rx(rxString);
    int indexStart = rx.indexIn(text);
    //qDebug() << rx.capturedTexts();
    if(indexStart != 1 && !rx.cap(1).trimmed().replace("-","").isEmpty())
        functor(rx.cap(1));
    else
        functor("not found");
}


core::Section FavouriteStoryParser::GetSection(QString text, int start)
{
    core::Section section;
    QRegExp rxStart("<div\\sclass=\'z-list\\sfavstories\'");

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
