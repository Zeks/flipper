/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

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
#include "include/parsers/ffn/ffnparserbase.h"
#include "include/web/url_utils.h"
#include <QDebug>
#include <QSettings>

FFNParserBase::~FFNParserBase()
{

}

void FFNParserBase::ProcessGenres(core::FanficSectionInFFNFavourites &section, QString genreText)
{
    section.result->SetGenres(genreText, QStringLiteral("ffn"));
    //qDebug() << "Genres: " << section.result->genres;
}

void FFNParserBase::ProcessCharacters(core::FanficSectionInFFNFavourites &section, QString characters)
{
    section.result->charactersFull = characters.trimmed();
    //qDebug() << "Characters: " << characters;
}

void FFNParserBase::ProcessStatSection(core::FanficSectionInFFNFavourites &section)
{
    thread_local QRegExp rxWords("Words:\\s(\\d{1,10})");
    thread_local QRegExp rxChapters("Chapters:\\s(\\d{1,6})");
    thread_local QRegExp rxReviews("Reviews:\\s(\\d{1,6})");
    thread_local QRegExp rxFavs("Favs:\\s(\\d{1,6})");
    thread_local QRegExp rxPublished("Published:\\s<span\\sdata-xutime='(\\d+)'");
    thread_local QRegExp rxUpdated("Updated:\\s<span\\sdata-xutime='(\\d+)'");
    thread_local QRegExp rxRated("Rated:\\s(.{1})");
    thread_local QRegExp rxGenres("English\\s-\\s([A-Za-z/\\-]+)\\s-\\sChapters");
    thread_local QRegExp rxComplete("(Complete)$");

    section.result->statSection = section.statSection.text;
    auto statText = section.statSection.text;

    statText = statText.replace(",", "");

    //qDebug() << statText;
    GetFandomFromTaggedSection(section, statText);
    GetTaggedSection(statText, rxWords,      [&section](QString val){ section.result->wordCount = val;});
    GetTaggedSection(statText, rxChapters,   [&section](QString val){ section.result->chapters = val;});
    GetTaggedSection(statText, rxReviews,    [&section](QString val){ section.result->reviews = val;});
    GetTaggedSection(statText, rxFavs,       [&section](QString val){ section.result->favourites = val;});
    GetTaggedSection(statText, rxPublished, [&section](QString val){
        if(val != QStringLiteral("not found"))
            section.result->published.setTime_t(val.toInt()); ;
        //qDebug() << "Published: " <<  section.result->published;
    });
    GetTaggedSection(statText, rxUpdated,  [&section](QString val){
        if(val != QStringLiteral("not found"))
            section.result->updated.setTime_t(val.toInt());
        else
            section.result->updated.setTime_t(0);
        //qDebug() << "Updated: " <<  section.result->updated;
    });
    GetTaggedSection(statText, rxRated, [&section](QString val){ section.result->rated = val;});
    GetTaggedSection(statText, rxGenres, [&section](QString val){ section.result->SetGenres(val, QStringLiteral("ffn"));});
    GetTaggedSection(statText, rxComplete, [&section](QString val){
        if(val != QStringLiteral("not found"))
            section.result->complete = 1;
    });
    GetCharacters(section.statSection.text,  [&section](QString val){
        section.result->charactersFull = val;
    });
    if(statText.contains(QStringLiteral("CROSSOVER"), Qt::CaseInsensitive))
        GetCrossoverFandomList(section, statText);
    else
        section.result->fandoms.push_back(section.result->fandom);
}

void FFNParserBase::GetTaggedSection(QString text, QRegExp &rx, std::function<void (QString)> functor)
{
    int indexStart = rx.indexIn(text);
    if(indexStart != 1 && !rx.cap(1).trimmed().replace("-","").isEmpty())
        functor(rx.cap(1));
    else
        functor(QStringLiteral("not found"));
}

void FFNParserBase::GetFandomFromTaggedSection(core::FanficSectionInFFNFavourites &, QString )
{
    //intentionally empty
}

#include "include/regex_utils.h"
void FFNParserBase::GetCharacters(QString text,
                                         std::function<void (QString)> functor)
{
    //auto full = GetSingleNarrow(text,"</span>\\s-\\s", "</div></div></div>", true);
    auto full = GetDoubleNarrow(text,QStringLiteral("^"),QStringLiteral("$"), true,
                                QStringLiteral(""),  QStringLiteral("</span>\\s-\\s"), true,
                                10);


    // qDebug() << text;
    full = full.replace(QStringLiteral(" - Complete"), QStringLiteral(""));
    if(full.contains(QStringLiteral("Published")))
        full = QStringLiteral("");

    if(full == QStringLiteral("Complete"))
        full = QString();
    functor(full);
}
void FFNParserBase::GetCrossoverFandomList(core::FanficSectionInFFNFavourites & section,  QString text)
{
    thread_local QRegExp rxStart("Crossover\\s-\\s");
    thread_local QRegExp rxEnd("\\s-\\sRated:");

    int indexStart = rxStart.indexIn(text);
    if(indexStart != -1 )
    {
        section.result->fandom.replace(QStringLiteral("Crossover - "), QStringLiteral(""));
        section.result->fandom += QStringLiteral(" CROSSOVER");
    }

    int indexEnd = rxEnd.indexIn(text, indexStart + 1);

    QString tmp = text.mid(indexStart + (rxStart.pattern().length() -2), indexEnd - (indexStart + rxStart.pattern().length() - 2)).trimmed();
    tmp.replace("\\'", "'");
    section.result->fandom = tmp + QStringLiteral(" CROSSOVER");
    section.result->fandoms = tmp.split(QStringLiteral(" & "), Qt::SkipEmptyParts);
    section.result->isCrossover = true;
}

void FFNParserBase::GetUrl(core::FanficSectionInFFNFavourites & section, int& startfrom, QString text)
{
    // looking for first href
    thread_local QRegExp rxStart(QRegExp::escape("href=\'"));
    thread_local QRegExp rxEnd(QRegExp::escape("\'><img"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    section.result->SetUrl(QStringLiteral("ffn"),text.mid(indexStart + 6,indexEnd - (indexStart + 6)));
    section.result->identity.web.ffn = url_utils::GetWebId(section.result->url(QStringLiteral("ffn")), QStringLiteral("ffn")).toInt();
    startfrom = indexEnd+2;
}



void FFNParserBase::GetAuthor(core::FanficSectionInFFNFavourites & section, int &startfrom,  QString text)
{
    text = text.mid(startfrom);
    auto full = GetDoubleNarrow(text,QStringLiteral("/u/\\d+/"), QStringLiteral("</a>"), true,
                                QStringLiteral(""),  QStringLiteral("'>"), false,
                                2);

    thread_local QRegExp rxEnd("(/u/(\\d+)/)(.*)(?='>)");
    rxEnd.setMinimal(true);
    auto index = rxEnd.indexIn(text);
    if(index == -1)
        return;

    QSharedPointer<core::Author> author(new core::Author);
    section.result->author = author;
    section.result->author->SetWebID(QStringLiteral("ffn"), rxEnd.cap(2).toInt());
    section.result->author->name = full;

}

void FFNParserBase::GetSummary(core::FanficSectionInFFNFavourites & section, int& startfrom, QString text)
{
    thread_local QRegExp rxStart(QRegExp::escape("padtop'>"));
    thread_local QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);

    section.result->summary = text.mid(indexStart + 8,indexEnd - (indexStart + 8));
    section.summaryEnd = indexEnd;
    startfrom = indexEnd;
}
void FFNParserBase::GetStatSection(core::FanficSectionInFFNFavourites &section, int &startfrom, QString text)
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

core::FanficSectionInFFNFavourites FFNParserBase::GetSection(QString text, QString sectionSeparator, int start)
{
    core::FanficSectionInFFNFavourites section ;
    thread_local QRegExp rxStart(sectionSeparator);
    rxStart.setPattern(sectionSeparator);
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
std::once_flag settingsFlag;
void FFNParserBase::ProcessSection(core::FanficSectionInFFNFavourites &section, int &currentPosition, QString str)
{
    section.result->isValid = true;
    //qDebug() << str;
    GetTitleAndUrl(section, currentPosition, str);
    if(section.result->ficSource != core::Fanfic::efs_own_works)
        GetAuthor(section, currentPosition, str);
    GetSummary(section, currentPosition, str);

    GetStatSection(section, currentPosition, str);
    ProcessStatSection(section);
    static bool performLogging = false;
    std::call_once(settingsFlag, [&](){
        QSettings settings(QStringLiteral("settings/settings.ini"), QSettings::IniFormat);
        performLogging = settings.value(QStringLiteral("Settings/logParsedFics")).toBool();
    });
    if(performLogging)
        section.result->Log();
}
