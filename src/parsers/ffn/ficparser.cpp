/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2018  Marchenko Nikolai

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
#include "include/parsers/ffn/ficparser.h"
#include "include/core/section.h"
#include "include/regex_utils.h"
#include "include/url_utils.h"
#include "include/pure_sql.h"
#include "Interfaces/fanfics.h"
#include "Interfaces/authors.h"
#include "Interfaces/genres.h"
#include <QDebug>
#include <QSqlDatabase>
#include <algorithm>
#include <chrono>

FicParser::FicParser(QSharedPointer<interfaces::Fanfics> fanfics,
                     QSharedPointer<interfaces::Genres> genres)
: FFNParserBase(fanfics), genres(genres){

}

QSharedPointer<core::Fic> FicParser::ProcessPage(QString url, QString& str)
{
    core::Section section;
    int currentPosition = 0;
    if(str.contains("Unable to locate story. Code 1"))
    {
        auto fics = fanfics;
        int ficId = fics->GetIdForUrl(url);
        fics->DeactivateFic(ficId);
        return section.result;
    }

    section = GetSection(str);
    section.result->webId = url_utils::GetWebId(url, "ffn").toInt();
    qDebug() << "Processing fic: " << section.result->webId;
    currentPosition = section.start;

    GetAuthor(section, str);
    GetFandom(section, currentPosition, str);
    GetTitle(section,  str);
    GetSummary(section,  str);
    GetStatSection(section,  str);

    DetermineMarkedSubsectionPresence(section);
    ProcessUnmarkedSections(section);

    auto& stat = section.statSection;

    // reading marked section data
    GetTaggedSection(stat.text, "target='rating'>Fiction\\s\\s([A-Z])", [&section](QString val){ section.result->rated = val;});
    GetTaggedSection(stat.text, "Chapters:\\s(\\d+)", [&section](QString val){ section.result->chapters = val;});
    GetTaggedSection(stat.text, "Words:\\s(\\d+,\\d+)", [&section](QString val){section.result->wordCount = val;});
    GetTaggedSection(stat.text, "(\\d+)</a>", [&section](QString val){ section.result->reviews = val;});
    GetTaggedSection(stat.text, "Favs:\\s(\\d+)", [&section](QString val){ section.result->favourites = val;});
    GetTaggedSection(stat.text, "Follows:\\s(\\d+)", [&section](QString val){ section.result->follows = val;});
    GetTaggedSection(stat.text, "Status:\\s(\\d+)", [&section](QString val){
        if(val != "not found")
            section.result->complete = 1;}
    );
    GetTaggedSection(stat.text, "data-xutime='(\\d+)'", [&section](QString val){
        if(val != "not found")
        {
            section.result->updated.setTime_t(val.toInt());
            qDebug() << val << section.result->updated;
        }
    });
    GetTaggedSection(stat.text, "data-xutime='(\\d+)'", [&section](QString val){
        if(val != "not found")
        {
            section.result->published.setTime_t(val.toInt());
            qDebug() << val << section.result->published;
        }
    }, 1);
    // if something doesn't have update date then we've read "published" incorrectly, fixing
    if(!section.statSection.updated.isValid)
        section.result->published = section.result->updated;
    section.result->isValid = true;
    //auto tempResult = QSharedPointer<core::Fic>(new core::Fic>);
    //*tempResult = section.result;
    //processedStuff = {tempResult};
    return section.result;
}

core::Section::Tag FicParser::GetStatTag(QString text, QString tag)
{
    core::Section::Tag result;
    result.marker = tag;
    QRegExp rx(tag);
    auto index = rx.indexIn(text);
    if(index == -1)
        return result;
    result.isValid = true;
    result.position = index;
    return result;
}

void FicParser::GetTaggedSection(QString text,  QString rxString, std::function<void (QString)> functor, int skipCount)
{
    QRegExp rx(rxString);
    int indexStart = rx.indexIn(text);
    while(skipCount != 0)
    {
        indexStart = rx.indexIn(text, indexStart + 1);
        skipCount--;
    }
    if(indexStart != 1 && !rx.cap(1).trimmed().replace("-","").isEmpty())
        functor(rx.cap(1));
    else
        functor("not found");
}

void FicParser::ProcessUnmarkedSections(core::Section & section)
{
    auto& stat = section.statSection;
    // step 0, process rating to find the end of it
    QRegExp rxRated("'rating'>Fiction\\s\\s(.)");
    auto index = rxRated.indexIn(stat.text);
    int ratedTagLength = 11;
    if(index == -1)
        return;
    stat.rated = {rxRated.cap(1), index + ratedTagLength};
    // step 1, get language marker as it is necessary and always follows rating
    QRegExp rxlanguage("\\s-\\s(.*)(?=\\s-\\s)");
    rxlanguage.setMinimal(true);
    index = rxlanguage.indexIn(stat.text);
    int separatorTagLength = 3;
    if(index == -1)
        return;
    stat.language = core::Section::Tag(rxlanguage.cap(1), index + separatorTagLength + rxlanguage.cap(1).length());

    // step 2 decide if we have one or two sections between language and next found tagged position
    int nextTaggedSectionPosition = -1;
    if(stat.chapters.position != -1)
        nextTaggedSectionPosition = stat.chapters.position;
    else
        nextTaggedSectionPosition = stat.words.position;

    int size = nextTaggedSectionPosition - stat.language.end;
    QString temp = stat.text.mid(stat.language.end, size);
    temp=temp.trimmed();
    temp = temp.replace(QRegExp("(^-)|(-$)"), "");
    auto separatorCount = temp.count(" - ");
    if(separatorCount >= 1)
    {
        // two separators means we are have both genre and characters (hopefully)
        // thus, we are splitting what we have on " - "
        // 0 must be genre, 1 must be characters
        QStringList splittings = temp.split(" - ", QString::SkipEmptyParts);
        if(splittings.size() == 0)
            return;

        QStringList genreCandidates = splittings.at(0).split("/");
        bool isGenre = false;
        if(genres)
            isGenre = genres->IsGenreList(genreCandidates);
        if(isGenre)
        {
            ProcessGenres(section, splittings.at(0));
            splittings.pop_front();
        }

        if(splittings.size() == 0)
            return;

        ProcessCharacters(section, splittings.join(" "));
    }
    else{
        // step 3 decide if it has whitespaces before the next " - "
        //   if it has it's characters section
        auto splittings = temp.split(" - ", QString::SkipEmptyParts)        ;
        for(auto& part: splittings)
            part = part.trimmed();
        if(splittings.size() == 0)
            return;
        if(splittings.at(0).contains(" "))
            ProcessCharacters(section, splittings.at(0));
        else
        {
            //   if it has not, it's genre or a singlewords character like Dobby
            //       attempt to split it on "/" and check every token with genre's list in DB
            //       if no match is found its characters

            QStringList genreCandidates = splittings.at(0).split("/");
            bool isGenre = false;
            if(genres)
                isGenre = genres->IsGenreList(genreCandidates);
            if(isGenre)
                ProcessGenres(section, splittings.at(0));
            else
                ProcessCharacters(section, splittings.at(0));
        }
    }
}

void FicParser::DetermineMarkedSubsectionPresence(core::Section & section)
{
    auto& stat = section.statSection;
    stat.rated = GetStatTag(stat.text, "Rated:");
    stat.chapters = GetStatTag(stat.text, "Chapters:");
    stat.favs = GetStatTag(stat.text, "Favs:");
    stat.follows = GetStatTag(stat.text, "Follows:");
    stat.reviews = GetStatTag(stat.text, "Reviews:");
    stat.published = GetStatTag(stat.text, "Published:");
    stat.updated = GetStatTag(stat.text, "Updated:");
    stat.words= GetStatTag(stat.text, "Words:");
    stat.status = GetStatTag(stat.text, "Status:");
    stat.id = GetStatTag(stat.text, "Id:");
}



void FicParser::ClearProcessed()
{
    processedStuff = decltype(processedStuff)();
}

//void FicParser::WriteProcessed()
//{
//    fanfics->ProcessIntoDataQueues(processedStuff);
//    fanfics->FlushDataQueues();
//}

void FicParser::SetRewriteAuthorNames(bool value)
{
    rewriteAuthorName = value;
}

void FicParser::GetFandom(core::Section & section, int &, QString text)
{
    auto full = GetDoubleNarrow(text,"id=pre_story_links", "</a>\\n", true,
                                "",  "/\">", true,
                                3);
    full.replace(" Crossover", "");
    full.replace("\\'", "'");
    QStringList split = full.split(" + ", QString::SkipEmptyParts);
    section.result->fandom = full;
    section.result->fandoms = split;
    qDebug() << section.result->fandoms;
    qDebug() << section.result->fandom;
}


void FicParser::GetAuthor(core::Section & section,  QString text)
{
    auto full = GetDoubleNarrow(text,"/u/\\d+/", "</a>", true,
                                "",  "'>", false,
                                2);

    QRegExp rxEnd("(/u/\\d+/)(.*)(?='>)");
    rxEnd.setMinimal(true);
    auto index = rxEnd.indexIn(text);
    if(index == -1)
        return;

    QSharedPointer<core::Author> author(new core::Author);
    section.result->author = author;
    QString uPart = rxEnd.cap(2);
    section.result->author->SetWebID("ffn",uPart.mid(3).toInt()); // needs checking
    section.result->author->name = full;
    queuedAuthor = author;
}

void FicParser::GetTitle(core::Section & section,QString text)
{
    QRegExp rx("Follow/Fav</button><b\\sclass='xcontrast[_]txt'>(.*)</");
    rx.setMinimal(true);
    int indexStart = rx.indexIn(text);
    if(indexStart == -1)
    {
        qDebug() << "failed to get title";
        return;
    }
    section.result->title = rx.cap(1);
    qDebug() << section.result->title;
}

void FicParser::GetStatSection(core::Section &section,  QString text)
{
    auto full = GetSingleNarrow(text,"Rated:", "\\sid:", true);
    qDebug() << full;
    section.statSection.text = full;
}

void FicParser::GetSummary(core::Section & section, QString text)
{
    auto summary = GetDoubleNarrow(text,
                    "Private\\sMessage", "</div", true,
                    "", "'>", false,
                    2);

    section.result->summary = summary;
    qDebug() << summary;
}

core::Section FicParser::GetSection(QString text)
{
    core::Section section;
    section.start = 0;
    section.end = text.length();
    return section;
}

QString FicParser::ExtractRecommenderNameFromUrl(QString url)
{
    int pos = url.lastIndexOf("/");
    return url.mid(pos+1);
}

