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
#include "include/core/section.h"
#include <QDebug>

//core::FicSectionStats core::Author::MergeStats(QList<core::AuthorPtr> authors)
//{

//    for(auto author : authors)
//    {

//    }
//}


template <typename T1, typename T2>
inline double DivideAsDoubles(T1 arg1, T2 arg2){
    return static_cast<double>(arg1)/static_cast<double>(arg2);
}
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

void core::RecommendationList::Log()
{

    qDebug() << "List id: " << id ;
    qDebug() << "name: " << name ;
    qDebug() << "ficCount: " << ficCount ;
    qDebug() << "tagToUse: " << tagToUse ;
    qDebug() << "minimumMatch: " << minimumMatch ;
    qDebug() << "alwaysPickAt: " << alwaysPickAt ;
    qDebug() << "pickRatio: " << pickRatio ;
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

core::EntitySizeType ProcesWordcountIntoSizeCategory(int wordCount)
{
    core::EntitySizeType result{core::EntitySizeType::none};

    if(wordCount <= 20000)
        result = core::EntitySizeType::small;
    else if(wordCount <= 100000)
        result = core::EntitySizeType::medium;
    else if(wordCount <= 400000)
        result = core::EntitySizeType::big;
    else
        result = core::EntitySizeType::huge;

    return result;
}

core::ExplorerRanges core::ProcesFavouritesIntoPopularityCategory(int favourites)
{
    core::ExplorerRanges result{core::ExplorerRanges::none};

    if(favourites <= 50)
        result = core::ExplorerRanges::barely_known;
    if(favourites <= 150)
        result = core::ExplorerRanges::relatively_unknown;
    if(favourites > 150)
        result = core::ExplorerRanges::popular;
    return result;
}


void core::FicListDataAccumulator::AddPublishDate(QDate date){
    if(!firstPublished.isValid() || firstPublished < date)
        firstPublished = date;
    if(!lastPublished.isValid() || lastPublished > date)
        lastPublished = date;
}
void core::FicListDataAccumulator::AddFavourites(int favCount) // favs, explorer, megaexplorer, fav size category
{
    ficCount++;
    favourites+=favCount;
    auto type = core::ProcesFavouritesIntoPopularityCategory(favCount);
    popularityCounters[static_cast<size_t>(type)]++;
}
void core::FicListDataAccumulator::AddFandoms(const QList<int>& fandoms){
    if(fandoms.size() > 1)
        crossoverCounter++;
    for(auto ficFandom : fandoms)
        fandomCounters[ficFandom]++;
}
// wordcount average fic wordcount, size factors
void core::FicListDataAccumulator::AddWordcount(int wordcount, int chapters){
    this->wordcount+=wordcount;
    this->chapterCounter+=chapters;
    sizeCounters[static_cast<size_t>(core::ProcesWordcountIntoSizeCategory(wordcount))]++;
}

void core::FicListDataAccumulator::ProcessIntoresult()
{
    ProcessFicSize();
    result.megaExplorerRatio = DivideAsDoubles(popularityCounters[1], ficCount);
    result.explorerRatio = DivideAsDoubles(popularityCounters[2], ficCount);
    result.matureRatio = DivideAsDoubles(matureCounter, ficCount);
    result.slashRatio = DivideAsDoubles(slashCounter, ficCount);
    result.unfinishedRatio = DivideAsDoubles(unfinishedCounter, ficCount);
}

void core::FicListDataAccumulator::ProcessFicSize()
{
    short dominatingValue = 0;
    for(size_t i = 0; i < sizeCounters.size(); i++)
    {
        result.sizeRatios[i] = DivideAsDoubles(sizeCounters[i],ficCount);
        if(sizeCounters[i] > dominatingValue)
            dominatingValue = static_cast<short>(i);
    }
    result.mostFavouritedSize = static_cast<EntitySizeType>(1 + dominatingValue);
    result.averageWordsPerFic = DivideAsDoubles(wordcount,ficCount);
    result.averageWordsPerChapter = DivideAsDoubles(wordcount,chapterCounter);
}

static QHash<QString, int> CreateGenreStringMoodRedirects(){
    QHash<QString, int> result;
    result["General"]       = 1;
    result["Humor"]         = 2;
    result["Poetry"]        = 1;
    result["Adventure"]     = 1;
    result["Mystery"]       = 1;
    result["Horror"]        = 1;
    result["Parody"]        = 2;
    result["Angst"]         = 0;
    result["Supernatural"]  = 1;
    result["Suspense"]      = 1;
    result["Romance"]       = 1;
    result["Drama"]         = 0;
    result["not found"]     = 1;
    result["Sci-Fi"]        = 1;
    result["Fantasy"]       = 1;
    result["Spiritual"]     = 1;
    result["Tragedy"]       = 0;
    result["Western"]       = 1;
    result["Crime"]         = 1;
    result["Family"]        = 1;
    result["Hurt/Comfort"]  = 0;
    result["Friendship"]    = 2;
    return result;
}
static QHash<QString, int> CreateGenreArrayIndexMoodRedirects(){
    QHash<int, int> result;
    result[0]       = 1;
    result[1]       = 2;
    result[2]       = 1;
    result["Adventure"]     = 1;
    result["Mystery"]       = 1;
    result["Horror"]        = 1;
    result["Parody"]        = 2;
    result["Angst"]         = 0;
    result["Supernatural"]  = 1;
    result["Suspense"]      = 1;
    result["Romance"]       = 1;
    result["Drama"]         = 0;
    result["not found"]     = 1;
    result["Sci-Fi"]        = 1;
    result["Fantasy"]       = 1;
    result["Spiritual"]     = 1;
    result["Tragedy"]       = 0;
    result["Western"]       = 1;
    result["Crime"]         = 1;
    result["Family"]        = 1;
    result["Hurt/Comfort"]  = 0;
    result["Friendship"]    = 2;
    return result;
}

inline void ProcessGenres()
{
    QHash<int, int> moodKeeper;
    thread_local QHash<QString, int> moodRedirects = CreateGenreStringMoodRedirects();
    //int moodSum = 0;
    for(auto genre: fic->genres)
    {
        genreKeeper[genre]++;
        auto redirected = moodRedirects[genre];
        if(redirected == 0)
            moodKeeper[1]++;
        else if(redirected < 0)
            moodKeeper[0]++;
        else
            moodKeeper[2]++;
    }

}
