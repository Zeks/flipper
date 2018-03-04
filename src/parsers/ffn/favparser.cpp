/*
Flipper is a replacement search engine for fanfiction.net search results
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
#include "include/regex_utils.h"
#include <QDebug>
#include <QRegularExpression>
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

FieldSearcher CreateProfilePageUpdatedSearcher()
{
    using namespace SearchTokenNamespace;
    QList<SearchToken> tokens;
    tokens.push_back({"Profile\\sUpdated:",
                      "0000000 1100000000",
                      16,          find_first_instance, move_left_boundary});
    tokens.push_back({"'>","00",0, find_first_instance, move_right_boundary});
    tokens.push_back({"'","0",1,   find_first_instance, move_left_boundary});
    FieldSearcher result;
    result.tokens = tokens;
    result.name = "Profile Updated";
    return  result;
}
FieldSearcher CreateProfilePageCreatedSearcher()
{
    using namespace SearchTokenNamespace;
    QList<SearchToken> tokens;
    tokens.push_back({"=2>Joined\\s<",
                      "000000000 110",
                      13,          find_first_instance, move_left_boundary});
    tokens.push_back({"'>","00",0, find_first_instance, move_right_boundary});
    tokens.push_back({"'","0",1,   find_first_instance, move_left_boundary});
    FieldSearcher result;
    result.tokens = tokens;
    result.name = "Profile Created";
    return  result;
}

QDate DateFromXUtime(QString value)
{
    QDateTime temp;
    if(!value.isEmpty())
    {
        temp.setTime_t(value.toInt());
    }
    return temp.date();
}
inline void UpdatePopularity(QSharedPointer<core::Fic> fic, QHash<int, int>& popularUnpopularKeeper)
{
    auto faveCount = fic->favourites.toInt();
    if(faveCount <= 150)
        popularUnpopularKeeper[0]++;
    else
        popularUnpopularKeeper[1]++;
}

inline void UpdateFandoms(QSharedPointer<core::Fic> fic, QHash<int, int>& crossKeeper, QHash<QString, int>& fandomKeeper)
{
    if(fic->fandoms.size() == 1)
        crossKeeper[0]++;
    else
        crossKeeper[1]++;
    for(auto fandom: fic->fandoms)
    {
        fandomKeeper[core::Fandom::ConvertName(fandom)]++;
    }
}
inline void UpdateCompleteness(QSharedPointer<core::Fic> fic, QHash<int, int>& unfinishedKeeper)
{
    if(fic->complete == 1)
        unfinishedKeeper[0]++;
    else
        unfinishedKeeper[1]++;
}

inline void UpdateFicSize(QSharedPointer<core::Fic> fic, QHash<int, int>& favouritesSizeKeeper, QList<int>& sizes)
{
    auto wordCount = fic->wordCount.toInt();
    if(wordCount <= 20000)          // tiny
        favouritesSizeKeeper[0]++;
    else if(wordCount <= 100000)    // medium
        favouritesSizeKeeper[1]++;
    if(wordCount <= 400000)         // large
        favouritesSizeKeeper[2]++;
    else                            // huge
        favouritesSizeKeeper[3]++;
    sizes.push_back(wordCount);
}

inline void UpdateESRB(QSharedPointer<core::Fic> fic, QHash<int, int>& esrbKeeper)
{
    if(fic->rated != "M")
        esrbKeeper[0]++; // kiddy
    else
        esrbKeeper[1]++; // mature
}

inline void UpdateGenreResults(QSharedPointer<core::Fic> fic, QHash<QString, int>& genreKeeper)
{
    for(auto genre: fic->genres)
    {
        if(genre.length() < 3)
            genreKeeper[genre]++;
        genreKeeper[genre]++;
    }
}

inline void ProcessFavouriteSectionSize(QSharedPointer<core::Author> author, int favouritesSize)
{
    //tiny(<50)/medium(50-500)/large(500-2000)/bullshit(2k+);
    if(favouritesSize <= 50)
        author->stats.favouriteSectionSize = core::EntitySizeType::small;
    else if(favouritesSize <= 500)
        author->stats.favouriteSectionSize = core::EntitySizeType::medium;
    else if(favouritesSize <= 2000)
        author->stats.favouriteSectionSize = core::EntitySizeType::big;
    else
        author->stats.favouriteSectionSize = core::EntitySizeType::huge;
}
QHash<QString, int> CreateMoodRedirects(){
    QHash<QString, int> result;
    result["General"] = 1;
    result["Humor"] = 2;
    result["Poetry"] = 1;
    result["Adventure"] = 1;
    result["Mystery"] = 1;
    result["Horror"] = 1;
    result["Parody"] = 2;
    result["Angst"] = 0;
    result["Supernatural"] = 1;
    result["Suspense"] = 1;
    result["Romance"] = 1;
    result["not found"] = 1;
    result["Sci-Fi"] = 1;
    result["Fantasy"] = 1;
    result["Spiritual"] = 1;
    result["Tragedy"] = 0;
    result["Western"] = 1;
    result["Crime"] = 1;
    result["Family"] = 1;
    result["Hurt/Comfort"] = 0;
    result["Friendship"] = 2;
    return result;
}
inline void ProcessMood(QSharedPointer<core::Author> author, int ficTotal, QHash<QString, int>& genreKeeper)
{
    QHash<int, double> moodFactors;
    thread_local QHash<QString, int> moodRedirects = CreateMoodRedirects();
    double maxPrevalence = 0.0;
    //double averageFicsPerGenre = static_cast<double>(ficTotal)/static_cast<double>(genreKeeper.keys().size());
    int totalInClumps = 0;
    for(auto genre : genreKeeper.keys())
    {

        double factor = static_cast<double>(genreKeeper[genre])/static_cast<double>(ficTotal);
        if(static_cast<double>(genreKeeper[genre])/static_cast<double>(ficTotal) > 0.7)
            totalInClumps+=genreKeeper[genre];

        if(factor > maxPrevalence)
        {
            author->stats.prevalentGenre = genre;
            maxPrevalence = factor;
        }
        int redirectedTo = moodRedirects[genre];
        moodFactors[redirectedTo]+=factor;
    }
    int totalInRest = ficTotal - totalInClumps;
    author->stats.genreDiversityFactor = static_cast<double>(totalInRest)/static_cast<double>(ficTotal);
    double dominatingMoodValue = 0.0;
    double totalMoodValue = 0.0;
    for(auto mood : moodFactors.keys())
    {
        totalMoodValue+=moodFactors[mood];
        if(mood == 0)
            author->stats.moodSad = moodFactors[mood];
        else if (mood == 1)
            author->stats.moodNeutral = moodFactors[mood];
        else
            author->stats.moodHappy = moodFactors[mood];
        if(moodFactors[mood] > dominatingMoodValue)
        {
            author->stats.prevalentMood = static_cast<core::AuthorStats::MoodType>(mood);
            author->stats.moodDiversityFactor = moodFactors[mood]/totalMoodValue;
            dominatingMoodValue = moodFactors[mood];
        }
    }
}

inline void ProcessFicSize(QSharedPointer<core::Author> author, QList<int> sizes, QHash<int, int> ficSizeKeeper)
{
    int total = sizes.size();
    int dominatingValue = 0.0;
    for(auto ficSize : ficSizeKeeper.keys())
    {
        if(ficSizeKeeper[ficSize] > dominatingValue)
        {
            author->stats.mostFavouritedSize = static_cast<core::EntitySizeType>(ficSize);
            dominatingValue = ficSizeKeeper[ficSize];
        }
    }
    double averageFicSize = 0.0;
    int sum = 0;
    for(auto size : sizes)
        sum+=size;
    averageFicSize = static_cast<double>(sum)/static_cast<double>(total);
    author->stats.averageFavouritedLength = averageFicSize;
}

inline void ProcessCrossovers(QSharedPointer<core::Author> author, int ficTotal, QHash<int, int>& crossKeeper)
{
    author->stats.crossoverFactor = static_cast<double>(crossKeeper[1])/static_cast<double>(ficTotal);
}

inline void ProcessUnpopular(QSharedPointer<core::Author> author, int ficTotal, QHash<int, int>& popularUnpopularKeeper)
{
    author->stats.explorerFactor = static_cast<double>(popularUnpopularKeeper[0])/static_cast<double>(ficTotal);
}

inline void ProcessFandomDiversity(QSharedPointer<core::Author> author, int ficTotal, QHash<QString, int>& fandomKeeper)
{
    double averageFicsPerFandom = static_cast<double>(ficTotal)/static_cast<double>(fandomKeeper.keys().size());
    // need to find fics in fandoms 2x over average
    int totalInClumps = 0;
    for(auto fandom : fandomKeeper.keys())
    {
        if(fandomKeeper[fandom] >= 2*averageFicsPerFandom)
            totalInClumps+=fandomKeeper[fandom];
    }
    int totalInRest = ficTotal - totalInClumps;
    //diverse favourite list  will have this close to 1
    author->stats.favouriteFandomsDiversity = static_cast<double>(totalInRest)/static_cast<double>(ficTotal);
}

inline void ProcessUnfinished(QSharedPointer<core::Author> author, int ficTotal, QHash<int, int> unfinishedKeeper)
{
    author->stats.unfinishedFactor = static_cast<double>(unfinishedKeeper[1])/static_cast<double>(ficTotal);
}


inline void ProcessESRB(QSharedPointer<core::Author> author, int ficTotal, QHash<int, int> esrbKeeper)
{
    //auto minValue = std::min(esrbKeeper[0],esrbKeeper[1]);
    auto maxValue = std::max(esrbKeeper[0],esrbKeeper[1]);
    author->stats.esrbUniformityFactor= static_cast<double>(maxValue)/static_cast<double>(ficTotal);
    author->stats.esrbKiddy = static_cast<double>(esrbKeeper[0])/static_cast<double>(ficTotal);
    author->stats.esrbMature = static_cast<double>(esrbKeeper[1])/static_cast<double>(ficTotal);
    bool hasprevalentESRB = author->stats.esrbUniformityFactor > 0.65;
    bool kiddyPrevalent = esrbKeeper[0] > esrbKeeper[1];
    if(!hasprevalentESRB)
        author->stats.esrbType = core::AuthorStats::ESRBType::agnostic;
    else{
        if(kiddyPrevalent)
            author->stats.esrbType = core::AuthorStats::ESRBType::kiddy;
        else
            author->stats.esrbType = core::AuthorStats::ESRBType::mature;
    }
}


QList<QSharedPointer<core::Fic> > FavouriteStoryParser::ProcessPage(QString url, QString& str)
{
    thread_local FieldSearcher profilePageUpdatedFinder = CreateProfilePageUpdatedSearcher();
    thread_local FieldSearcher profilePageCreatedFinder = CreateProfilePageCreatedSearcher();

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

    auto profileUpdateDate = BouncingSearch(str, profilePageUpdatedFinder);
    auto profileCreateDate = BouncingSearch(str, profilePageCreatedFinder);

    author->stats.pageCreated    = DateFromXUtime(profileCreateDate);
    author->stats.bioLastUpdated = DateFromXUtime(profileUpdateDate);

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

        // joined = 1st data-xutime
        // updated = 2nd data-xutime


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
    QHash<int, int> ficSizeKeeper;
    QHash<int, int> crossKeeper;
    QHash<int, int> favouritesSizeKeeper;
    QHash<int, int> popularUnpopularKeeper;
    QHash<QString, int> fandomKeeper;
    QHash<int, int> unfinishedKeeper;
    QHash<int, int> esrbKeeper;
    QHash<QString, int> genreKeeper;
    QList<int> sizes;
    sizes.reserve(sections.size());
    QDate firstPublished;
    QDate lastPublished;
    for(auto fic: sections)
    {
        if(!firstPublished.isValid() || fic->published.date() < firstPublished)
            firstPublished = fic->published.date();
        if(!lastPublished.isValid() || fic->published.date() > lastPublished)
            lastPublished = fic->published.date();
        UpdateFicSize(fic, ficSizeKeeper, sizes);
        UpdatePopularity(fic, popularUnpopularKeeper);
        UpdateFandoms(fic, crossKeeper, fandomKeeper);
        UpdateCompleteness(fic, unfinishedKeeper);
        UpdateESRB(fic, esrbKeeper);
        UpdateGenreResults(fic, genreKeeper);
    }
    int ficCount = sections.size();
    ProcessFavouriteSectionSize(author, ficCount);
    ProcessMood(author, ficCount, genreKeeper);
    ProcessFicSize(author, sizes, ficSizeKeeper);
    ProcessCrossovers(author, ficCount, crossKeeper);
    ProcessUnpopular(author, ficCount, popularUnpopularKeeper);
    ProcessFandomDiversity(author, ficCount, fandomKeeper);
    ProcessUnfinished(author, ficCount, unfinishedKeeper);
    ProcessESRB(author, ficCount, esrbKeeper);

    author->stats.favourites = sections.size();
    author->stats.firstPublishedFavourite = firstPublished;
    author->stats.lastPublishedFavourite = lastPublished;
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
    GetTaggedSection(text, rxFandom,    [&section](QString val){
        val.replace("\\'", "'");
        section.result->fandom = val;
    });
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
