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


#include "include/parsers/ffn/desktop_favparser.h"
#include "include/core/section.h"
#include "include/pure_sql.h"
#include "include/url_utils.h"
#include "include/regex_utils.h"
#include "include/Interfaces/fandoms.h"
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include "sql_abstractions/sql_database.h"
#include <chrono>
#include <algorithm>
//CommonRegex FavouriteStoryParser::commonRegex;
FavouriteStoryParser::FavouriteStoryParser()
{
    if(!commonRegex.initComplete)
        commonRegex.Init();
    //commonRegex.Log();
}

void ReserveSpaceForSections(QList<QSharedPointer<core::Fanfic>>& sections,  core::FanficSectionInFFNFavourites& section, QString& str)
{
    int parts = str.length()/(section.end-section.start);
    sections.reserve(parts);
}

FieldSearcher CreateProfilePageUpdatedSearcher()
{
    using namespace SearchTokenNamespace;
    QList<SearchToken> tokens;
    tokens.push_back({QStringLiteral("Profile\\sUpdated:"),
                      QStringLiteral("0000000 1100000000"),
                      16,          find_first_instance, move_left_boundary});
    tokens.push_back({QStringLiteral("'>"),QStringLiteral("00"),0, find_first_instance, move_right_boundary});
    tokens.push_back({QStringLiteral("'"),QStringLiteral("0"),1,   find_first_instance, move_left_boundary});
    FieldSearcher result;
    result.tokens = tokens;
    result.name = QStringLiteral("Profile Updated");
    return  result;
}
FieldSearcher CreateProfilePageCreatedSearcher()
{
    using namespace SearchTokenNamespace;
    QList<SearchToken> tokens;
    tokens.push_back({QStringLiteral("=2>Joined\\s<"),
                      QStringLiteral("000000000 110"),
                      13,          find_first_instance, move_left_boundary});
    tokens.push_back({QStringLiteral("'>"),QStringLiteral("00"),0, find_first_instance, move_right_boundary});
    tokens.push_back({QStringLiteral("'"),QStringLiteral("0"),1,   find_first_instance, move_left_boundary});
    FieldSearcher result;
    result.tokens = tokens;
    result.name = QStringLiteral("Profile Created");
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

inline void UpdatePopularity(QSharedPointer<core::Fanfic> fic, QHash<int, int>& popularUnpopularKeeper)
{
    auto faveCount = fic->favourites.toInt();
    if(faveCount <= 50)
        popularUnpopularKeeper[0]++;
    if(faveCount <= 150)
        popularUnpopularKeeper[1]++;
    if(faveCount > 150)
        popularUnpopularKeeper[2]++;
}

inline void UpdateFandoms(QSharedPointer<core::Fanfic> fic,
                          QHash<int, int>& crossKeeper,
                          QHash<QString, int>& fandomKeeper)
{
    if(fic->fandoms.size() == 1)
        crossKeeper[0]++;
    else
        crossKeeper[1]++;
    for(const auto& fandom: std::as_const(fic->fandoms))
    {
        fandomKeeper[fandom]++;
    }
}
inline void UpdateCompleteness(QSharedPointer<core::Fanfic> fic, QHash<int, int>& unfinishedKeeper)
{
    if(fic->complete == 1)
        unfinishedKeeper[0]++;
    else
        unfinishedKeeper[1]++;
}
inline void FavouriteStoryParser::UpdateWordsCounterNew(QSharedPointer<core::Fanfic> fic,
                                  const CommonRegex& regexToken,
                                  QHash<int, int>& wordsKeeper)
{
    if(fic->summary.contains(QStringLiteral("crack"), Qt::CaseInsensitive))
        wordsKeeper[0]++;

    auto result = regexToken.ContainsSlash(fic->summary, fic->charactersFull, fic->fandom);
    //auto ficPtr = fanfics->GetFicById(fanfics->GetIDFromWebID(fic->ffn_id, "ffn"));
    bool isInSlashSet = knownSlashFics.size() > 0 && knownSlashFics.contains(fic->identity.web.ffn);
    if(isInSlashSet || result.IsSlash())
        wordsKeeper[1]++;

    bool hasSmut = fic->summary.contains(QRegularExpression(regexToken.smut, QRegularExpression::CaseInsensitiveOption));

    if(hasSmut)
        wordsKeeper[2]++;

}
inline void UpdateWordsCounter(QSharedPointer<core::Fanfic> fic, QHash<int, int>& wordsKeeper)
{
//    if(fic->summary.contains("crack", Qt::CaseInsensitive))
//        wordsKeeper[0]++;


    bool containsSlash = false;
    QString slashRx = GetSlashRegex();
//    QString dontLikeRx = "don''t\\slike";
//    QString dontReadRx = "don''t\\sread";
    bool dontLikeDontRead = false;
//    dontLikeDontRead = fic->summary.contains(QRegularExpression(dontLikeRx, QRegularExpression::CaseInsensitiveOption));
//    dontLikeDontRead = dontLikeDontRead && fic->summary.contains(QRegularExpression(dontReadRx, QRegularExpression::CaseInsensitiveOption));
    containsSlash = containsSlash  || fic->summary.contains(QRegularExpression(slashRx, QRegularExpression::CaseInsensitiveOption));
    containsSlash = containsSlash  || fic->charactersFull.contains(QRegularExpression(slashRx, QRegularExpression::CaseInsensitiveOption));
    containsSlash = containsSlash  || dontLikeDontRead;

    bool containsNotSlash = false;
    QString notSlashRx = QStringLiteral("((no[tn]{0,1}|isn[']t)(\\s|-)(slash|yaoi))|(jack\\sslash)|(fem[!]{0,1})|(naruko)");
    containsNotSlash = containsNotSlash  || fic->summary.contains(QRegularExpression(notSlashRx, QRegularExpression::CaseInsensitiveOption));

    if(containsSlash && !containsNotSlash)
        wordsKeeper[1]++;

    bool hasSmut = false;
    QString smutRx = QStringLiteral("(\\srape)|(harem)|(smut)|(lime)|(\\ssex)|(dickgirl)|(shemale)|(nsfw)|(porn)"
                     "(futanari)|(lemon)|(yuri)|(incest)|(succubus)|(incub)|(\\sanal\\s)|(vagina)|(\\sfem\\s)");
    hasSmut = hasSmut  || fic->summary.contains(QRegularExpression(smutRx, QRegularExpression::CaseInsensitiveOption));

    if(hasSmut)
        wordsKeeper[2]++;

}

inline void UpdateFicSize(QSharedPointer<core::Fanfic> fic, QHash<int, int>& favouritesSizeKeeper, QList<int>& sizes, int& chapterCount)
{
    auto wordCount = fic->wordCount.toInt();
    chapterCount+=fic->chapters.toInt();
    if(wordCount <= 20000)          // tiny
        favouritesSizeKeeper[0]++;
    else if(wordCount <= 100000)    // medium
        favouritesSizeKeeper[1]++;
    else if(wordCount <= 400000)         // large
        favouritesSizeKeeper[2]++;
    else                            // huge
        favouritesSizeKeeper[3]++;
    sizes.push_back(wordCount);
}



inline void UpdateESRB(QSharedPointer<core::Fanfic> fic, QHash<int, int>& esrbKeeper)
{
    if(fic->rated != "M")
        esrbKeeper[0]++; // kiddy
    else
        esrbKeeper[1]++; // mature
}

static QHash<QString, int> CreateMoodRedirects(){
    QHash<QString, int> result;
    result["General"] = 0;
    result["Humor"] = 1;
    result["Poetry"] = 0;
    result["Adventure"] = 0;
    result["Mystery"] = 0;
    result["Horror"] = 0;
    result["Parody"] = 1;
    result["Angst"] = -1;
    result["Supernatural"] = 0;
    result["Suspense"] = 0;
    result["Romance"] = 0;
    result["Drama"] = -1;
    result["not found"] = 0;
    result["Sci-Fi"] = 0;
    result["Fantasy"] = 0;
    result["Spiritual"] = 0;
    result["Tragedy"] = -1;
    result["Western"] = 0;
    result["Crime"] = 0;
    result["Family"] = 0;
    result["Hurt/Comfort"] = -1;
    result["Friendship"] = 1;
    return result;
}
inline void UpdateGenreResults(QSharedPointer<core::Fanfic> fic,
                               QHash<QString, int>& genreKeeper,
                               QHash<int, int>& moodKeeper
                               )
{
    thread_local QHash<QString, int> moodRedirects = CreateMoodRedirects();
    //int moodSum = 0;
    for(const auto& genre: std::as_const(fic->genres))
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

inline void ProcessFavouriteSectionSize(QSharedPointer<core::Author> author, int favouritesSize)
{
    //tiny(<50)/medium(50-500)/large(500-2000)/bullshit(2k+);
    if(favouritesSize <= 50)
        author->stats.favouriteStats.sectionRelativeSize = core::EntitySizeType::small;
    else if(favouritesSize <= 500)
        author->stats.favouriteStats.sectionRelativeSize = core::EntitySizeType::medium;
    else if(favouritesSize <= 2000)
        author->stats.favouriteStats.sectionRelativeSize = core::EntitySizeType::big;
    else
        author->stats.favouriteStats.sectionRelativeSize = core::EntitySizeType::huge;
}

inline void ProcessGenre(QSharedPointer<core::Author> author,
                        int ficTotal,
                        QHash<QString, int>& genreKeeper)
{
    double maxPrevalence = 0.0;
    int totalInClumps = 0;
    auto it = genreKeeper.cbegin();
    auto itEnd = genreKeeper.cend();
    while(it != itEnd){
        const auto value = it.value();
        double factor = static_cast<double>(value)/static_cast<double>(ficTotal);
        if(static_cast<double>(value)/static_cast<double>(ficTotal) > 0.5)
            totalInClumps+=value;

        if(factor > maxPrevalence)
        {
            author->stats.favouriteStats.prevalentGenre = it.key();
            maxPrevalence = factor;
        }
        author->stats.favouriteStats.genreFactors[it.key()]=factor;
        it++;
    }

    int totalInRest = ficTotal - totalInClumps;
    author->stats.favouriteStats.genreDiversityFactor = static_cast<double>(totalInRest)/static_cast<double>(ficTotal);
}

//if(static_cast<double>(genreKeeper[genre])/static_cast<double>(ficTotal) > 0.5)
//    totalInClumps+=genreKeeper[genre];
//int totalInRest = ficTotal - totalInClumps;
//author->stats.favouriteStats.genreDiversityFactor = static_cast<double>(totalInRest)/static_cast<double>(ficTotal);

inline void ProcessFicSize(QSharedPointer<core::Author> author, QList<int> sizes, QHash<int, int>& ficSizeKeeper)
{
    int total = sizes.size();
    int dominatingValue = 0.0;

    for(auto i = ficSizeKeeper.cbegin(); i != ficSizeKeeper.cend(); i++)
    {
        if(i.value() > dominatingValue)
        {
            author->stats.favouriteStats.mostFavouritedSize = static_cast<core::EntitySizeType>(i.key());
            dominatingValue = i.value();
        }
        author->stats.favouriteStats.sizeFactors[i.key()] = static_cast<double>(i.value())/static_cast<double>(total);
    }
    double averageFicSize = 0.0;
    int sum = 0;
    for(auto size : sizes)
        sum+=size;
    averageFicSize = static_cast<double>(sum)/static_cast<double>(total);
    author->stats.favouriteStats.averageLength = averageFicSize;
}

inline void ProcessCrossovers(QSharedPointer<core::Author> author, int ficTotal, QHash<int, int>& crossKeeper)
{
    author->stats.favouriteStats.crossoverFactor = static_cast<double>(crossKeeper[1])/static_cast<double>(ficTotal);
}

inline void ProcessUnpopular(QSharedPointer<core::Author> author, int ficTotal, QHash<int, int>& popularUnpopularKeeper)
{
    author->stats.favouriteStats.megaExplorerFactor = static_cast<double>(popularUnpopularKeeper[0])/static_cast<double>(ficTotal);
    author->stats.favouriteStats.explorerFactor = static_cast<double>(popularUnpopularKeeper[1])/static_cast<double>(ficTotal);
}

inline void ProcessFandomDiversity(QSharedPointer<core::Author> author, int ficTotal, QHash<QString, int>& fandomKeeper)
{
    double averageFicsPerFandom = static_cast<double>(ficTotal)/static_cast<double>(fandomKeeper.size());
    // need to find fics in fandoms 2x over average
    int totalInClumps = 0;
    int totalValue = 0;

    for(auto i = fandomKeeper.cbegin(); i != fandomKeeper.cend(); i++)
    {
        auto tempValue = i.value();
        totalValue+=tempValue;
        if(tempValue >= 2*averageFicsPerFandom)
            totalInClumps+=tempValue;
        double  fandomFactor = static_cast<double>(tempValue)/static_cast<double>(ficTotal);
        author->stats.favouriteStats.fandomFactors[i.key()]=fandomFactor;
    }
    int totalInRest = totalValue - totalInClumps;
    //diverse favourite list  will have this close to 1
    author->stats.favouriteStats.fandomsDiversity = static_cast<double>(totalInRest)/static_cast<double>(totalValue);
}

inline void ProcessUnfinished(QSharedPointer<core::Author> author, int ficTotal, QHash<int, int>& unfinishedKeeper)
{
    author->stats.favouriteStats.unfinishedFactor = static_cast<double>(unfinishedKeeper[1])/static_cast<double>(ficTotal);
}


inline void ProcessESRB(QSharedPointer<core::Author> author, int ficTotal, QHash<int, int>& esrbKeeper)
{
    //auto minValue = std::min(esrbKeeper[0],esrbKeeper[1]);
    auto maxValue = std::max(esrbKeeper[0],esrbKeeper[1]);
    author->stats.favouriteStats.esrbUniformityFactor= static_cast<double>(maxValue)/static_cast<double>(ficTotal);
    author->stats.favouriteStats.esrbKiddy = static_cast<double>(esrbKeeper[0])/static_cast<double>(ficTotal);
    author->stats.favouriteStats.esrbMature = static_cast<double>(esrbKeeper[1])/static_cast<double>(ficTotal);
    bool hasprevalentESRB = author->stats.favouriteStats.esrbUniformityFactor > 0.65;
    bool kiddyPrevalent = esrbKeeper[0] > esrbKeeper[1];
    if(!hasprevalentESRB)
        author->stats.favouriteStats.esrbType = core::FavListDetails::ESRBType::agnostic;
    else{
        if(kiddyPrevalent)
            author->stats.favouriteStats.esrbType = core::FavListDetails::ESRBType::kiddy;
        else
            author->stats.favouriteStats.esrbType = core::FavListDetails::ESRBType::mature;
    }
}

inline void ProcessKeyWords(QSharedPointer<core::Author> author, int ficTotal, QHash<int, int>& keyWordsKeeper)
{
    //auto minValue = std::min(esrbKeeper[0],esrbKeeper[1]);
    author->stats.favouriteStats.crackRatio =  static_cast<double>(keyWordsKeeper[0])/static_cast<double>(ficTotal);
    author->stats.favouriteStats.slashRatio =  static_cast<double>(keyWordsKeeper[1])/static_cast<double>(ficTotal);
    author->stats.favouriteStats.smutRatio =  static_cast<double>(keyWordsKeeper[2])/static_cast<double>(ficTotal);
}


inline void ProcessMood(QSharedPointer<core::Author> author,
                        int ficTotal,
                         QHash<int, int>& moodKeeper)
{
    author->stats.favouriteStats.moodSad = static_cast<double>(moodKeeper[0])/static_cast<double>(ficTotal);
    author->stats.favouriteStats.moodNeutral = static_cast<double>(moodKeeper[1])/static_cast<double>(ficTotal);
    author->stats.favouriteStats.moodHappy= static_cast<double>(moodKeeper[2])/static_cast<double>(ficTotal);
    if(author->stats.favouriteStats.moodHappy > 0.5)
        author->stats.favouriteStats.prevalentMood = core::FavListDetails::MoodType::positive;
    else if(author->stats.favouriteStats.moodNeutral> 0.5)
        author->stats.favouriteStats.prevalentMood = core::FavListDetails::MoodType::neutral;
    if(author->stats.favouriteStats.moodSad> 0.5)
        author->stats.favouriteStats.prevalentMood = core::FavListDetails::MoodType::sad;
    auto maxMood = std::max(author->stats.favouriteStats.moodSad, std::max(author->stats.favouriteStats.moodNeutral, author->stats.favouriteStats.moodHappy));
    auto minMood = std::min(author->stats.favouriteStats.moodSad, std::min(author->stats.favouriteStats.moodNeutral, author->stats.favouriteStats.moodHappy));
    author->stats.favouriteStats.moodUniformity = static_cast<double>(minMood)/static_cast<double>(maxMood);
}

QList<QSharedPointer<core::Fanfic> > FavouriteStoryParser::ProcessPage(QString url, QString& str)
{
    thread_local FieldSearcher profilePageUpdatedFinder = CreateProfilePageUpdatedSearcher();
    thread_local FieldSearcher profilePageCreatedFinder = CreateProfilePageCreatedSearcher();
    statToken = core::FicSectionStatsTemporaryToken();
    QList<QSharedPointer<core::Fanfic>>  sections;
    core::FanficSectionInFFNFavourites section;
    int currentPosition = 0;
    int counter = 0;

    thread_local QSharedPointer<interfaces::Fandoms> fandomInterface(new interfaces::Fandoms());

//    QFile data("currentPage.html");
//      if (data.open(QFile::WriteOnly | QFile::Truncate)) {
//          QTextStream out(&data);
//          out << str;
//          data.flush();
//          data.close();
//      }

    core::AuthorPtr author(new core::Author);
    section.result->author = author;
    recommender.author = author;

    recommender.author->name = authorName;
    recommender.author->SetWebID("ffn", url_utils::GetWebId(url, QStringLiteral("ffn")).toInt());
    str = str.replace("s\" s", "s' s");
    str = str.replace("=\"", "='");
    str = str.replace("\">", "'>");
    str = str.replace("\\'", "'");
    str = str.replace(";\"", ";'");
    str = str.replace("ago<", "<");
    str = str.replace("&amp;", "&");



    auto profileUpdateDate = BouncingSearch(str, profilePageUpdatedFinder);
    auto profileCreateDate = BouncingSearch(str, profilePageCreatedFinder);

    this->statToken.pageCreated    = DateFromXUtime(profileCreateDate);
    this->statToken.bioLastUpdated = DateFromXUtime(profileUpdateDate);
    bool ownStory = false;
    int favCounter = 0;
    int ownCounter= 0;



    while(true)
    {
        counter++;
        favCounter++;
        section = GetSection(str, QStringLiteral("<div\\sclass=\[\'\"]z-list\\sfavstories[\'\"]"), currentPosition);
        if(!section.isValid)
        {
            favCounter--;
            ownCounter++;
            section = GetSection(str, QStringLiteral("<div\\sclass=[\'\"]z-list\\smystories[\'\"]"), currentPosition);
            if(!section.isValid)
                ownCounter--;
            ownStory = true;
        }
        if(!section.isValid)
            break;

        if(ownStory)
        {
            section.result->ficSource = core::Fanfic::efs_own_works;
            section.result->author = author;
        }
        else
            section.result->ficSource = core::Fanfic::efs_favourites;

        if(sections.size() == 0)
            ReserveSpaceForSections(sections, section, str);

        currentPosition = section.start;

        ProcessSection(section, currentPosition, str);

        // can be set invalid during the parse
        if(section.isValid)
            sections.append(section.result);
    }

    if(sections.size() == 0)
    {
        diagnostics.push_back("<span> No data found on the page.<br></span>");
        diagnostics.push_back("<span> \nFinished loading data <br></span>");
    }

    for(const auto& fic: std::as_const(sections))
    {
        statToken.wordCount+=fic->wordCount.toInt();
        if(!statToken.firstPublished.isValid() || fic->published.date() < statToken.firstPublished)
            statToken.firstPublished = fic->published.date();
        if(!statToken.lastPublished.isValid() || fic->published.date() > statToken.lastPublished)
            statToken.lastPublished = fic->published.date();
        UpdateFicSize(fic, statToken.ficSizeKeeper, statToken.sizes, statToken.chapterKeeper);
        UpdatePopularity(fic, statToken.popularUnpopularKeeper);
        UpdateFandoms(fic, statToken.crossKeeper, statToken.fandomKeeper);
        UpdateCompleteness(fic, statToken.unfinishedKeeper);
        UpdateESRB(fic, statToken.esrbKeeper);
        UpdateGenreResults(fic, statToken.genreKeeper, statToken.moodKeeper);
        UpdateWordsCounterNew(fic, commonRegex, statToken.wordsKeeper);
    }
    statToken.ficCount = sections.size();

//    qDebug() << "All fics: " << counter;
//    qDebug() << "Own fics: " << ownCounter;
//    qDebug() << "Fav fics: " << favCounter;
    processedStuff+=sections;
    authorStats = author;
    //currentPosition = 999;
    return sections;
}

QSet<QString> FavouriteStoryParser::FetchFavouritesIdList()
{
    QSet<QString> idResult;
    for(const auto& fic : std::as_const(processedStuff))
    {
        if(fic->ficSource != core::Fanfic::efs_own_works)
            idResult.insert(QString::number(fic->identity.web.ffn));
    }
    return idResult;
}

void FavouriteStoryParser::ClearProcessed()
{
    processedStuff.clear();
    fandoms.clear();
    diagnostics = QStringList{};
    alreadyDone.clear();
    recommender = FavouritesPage();
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

template <typename T, typename Y>
void Combine(QHash<T, Y>& firstHash, const QHash<T, Y> secondHash)
{
    for(const auto& key: secondHash.keys())
    {
        firstHash[key] += secondHash.value(key);
    }
}

void ConvertFandomsToIds(core::AuthorPtr author, QSharedPointer<interfaces::Fandoms> fandomInterface)
{
    auto& stats = author->stats.favouriteStats;

    for(auto i = author->stats.favouriteStats.fandoms.cbegin(); i != author->stats.favouriteStats.fandoms.cend(); i++)
    {
        stats.fandomsConverted[fandomInterface->GetIDForName(i.key())] = stats.fandoms.value(i.key());
        stats.fandomFactorsConverted[fandomInterface->GetIDForName(i.key())] = stats.fandomFactors.value(i.key());
    }
}


void FavouriteStoryParser::MergeStats(core::AuthorPtr author,
                                      QSharedPointer<interfaces::Fandoms> fandomsInterface,
                                      const QList<core::FicSectionStatsTemporaryToken>& tokens)
{
    core::FicSectionStatsTemporaryToken resultingToken;
    for(const auto& statToken : tokens)
    {
        resultingToken.chapterKeeper += statToken.chapterKeeper;
        resultingToken.ficCount+= statToken.ficCount;

        if(!resultingToken.firstPublished.isValid())
            resultingToken.firstPublished = statToken.firstPublished;
        resultingToken.firstPublished = std::min(statToken.firstPublished, resultingToken.firstPublished);
        if(!resultingToken.lastPublished.isValid())
            resultingToken.lastPublished = statToken.lastPublished;
        resultingToken.lastPublished = std::max(statToken.lastPublished, resultingToken.lastPublished);

        resultingToken.sizes += statToken.sizes;

        Combine(resultingToken.crossKeeper, statToken.crossKeeper);
        Combine(resultingToken.esrbKeeper, statToken.esrbKeeper);
        Combine(resultingToken.fandomKeeper, statToken.fandomKeeper);
        Combine(resultingToken.favouritesSizeKeeper, statToken.favouritesSizeKeeper);
        Combine(resultingToken.ficSizeKeeper, statToken.ficSizeKeeper);
        Combine(resultingToken.genreKeeper, statToken.genreKeeper);
        Combine(resultingToken.moodKeeper, statToken.moodKeeper);
        Combine(resultingToken.popularUnpopularKeeper, statToken.popularUnpopularKeeper);
        Combine(resultingToken.unfinishedKeeper, statToken.unfinishedKeeper);
        Combine(resultingToken.wordsKeeper, statToken.wordsKeeper);
        resultingToken.wordCount += statToken.wordCount;
        if(statToken.bioLastUpdated.isValid())
            author->stats.bioLastUpdated = statToken.bioLastUpdated;
        if(statToken.pageCreated.isValid())
            author->stats.pageCreated = statToken.pageCreated;
    }
    // this needs to be done outside, after multithreading has finished
    ProcessFavouriteSectionSize(author, resultingToken.ficCount);
    ProcessGenre(author, resultingToken.ficCount, resultingToken.genreKeeper);
    ProcessMood(author, resultingToken.ficCount, resultingToken.moodKeeper);
    ProcessFicSize(author, resultingToken.sizes, resultingToken.ficSizeKeeper);
    ProcessCrossovers(author, resultingToken.ficCount, resultingToken.crossKeeper);
    ProcessUnpopular(author, resultingToken.ficCount, resultingToken.popularUnpopularKeeper);
    ProcessFandomDiversity(author, resultingToken.ficCount, resultingToken.fandomKeeper);
    ProcessUnfinished(author, resultingToken.ficCount, resultingToken.unfinishedKeeper);
    ProcessESRB(author, resultingToken.ficCount, resultingToken.esrbKeeper);
    ProcessKeyWords(author, resultingToken.ficCount, resultingToken.wordsKeeper);

    author->stats.favouritesLastUpdated = resultingToken.lastPublished;
    author->stats.favouritesLastChecked = QDateTime::currentDateTime().date();

    author->stats.favouriteStats.fandoms = resultingToken.fandomKeeper;

    author->stats.favouriteStats.ficWordCount = resultingToken.wordCount;
    author->stats.favouriteStats.averageWordsPerChapter =
            static_cast<double>(author->stats.favouriteStats.ficWordCount)/static_cast<double>(resultingToken.chapterKeeper);

    author->stats.favouriteStats.favourites = resultingToken.ficCount;
    author->stats.favouriteStats.fandoms = resultingToken.fandomKeeper;

    author->stats.favouriteStats.firstPublished= resultingToken.firstPublished;
    author->stats.favouriteStats.lastPublished= resultingToken.lastPublished;
    author->isValid = true;

    ConvertFandomsToIds(author, fandomsInterface);
}

void FavouriteStoryParser::MergeStats(core::AuthorPtr author, QSharedPointer<interfaces::Fandoms> fandomsInterface, const QList<FavouriteStoryParser>& parsers)
{
    QList<core::FicSectionStatsTemporaryToken> tokenList;
    tokenList.reserve(parsers.size());
    for(const auto& parser : parsers)
    {
      tokenList.push_back(parser.statToken);
    }
    MergeStats(author, fandomsInterface, tokenList);
}

QString FavouriteStoryParser::ExtractRecommenderNameFromUrl(QString url)
{
    int pos = url.lastIndexOf(QStringLiteral("/"));
    return url.mid(pos+1);
}

void FavouriteStoryParser::GetFandomFromTaggedSection(core::FanficSectionInFFNFavourites & section, QString text)
{
    thread_local QRegExp rxFandom(QStringLiteral("(.*)(?=\\s-\\sRated:)"));
    GetTaggedSection(text, rxFandom,    [&section](QString val){
        val.replace(QStringLiteral("\\'"), QStringLiteral("'"));
        section.result->fandom = val;
    });
}

void FavouriteStoryParser::GetTitle(core::FanficSectionInFFNFavourites & section, int& startfrom, QString text)
{
    thread_local QRegExp rxStart(QRegExp::escape("data-title='"));
    thread_local QRegExp rxEnd(QRegExp::escape("\""));
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart+13);
    startfrom = indexEnd;
    section.result->title = text.mid(indexStart + 12,indexEnd - (indexStart + 12));
    section.result->title=section.result->title.replace(QStringLiteral("\\'"),QStringLiteral("'"));
    section.result->title=section.result->title.replace(QStringLiteral("\'"),QStringLiteral("'"));
}


void FavouriteStoryParser::GetTitleAndUrl(core::FanficSectionInFFNFavourites & section, int& currentPosition, QString str)
{
    GetTitle(section, currentPosition, str);
    GetUrl(section, currentPosition, str);
}


QString ParseAuthorNameFromFavouritePage(QString data)
{
    QString result;
    thread_local QRegExp rx(QStringLiteral("title>([A-Za-z0-9.\\-\\s']+)(?=\\s|\\sFanFiction)"));
    int index = rx.indexIn(data);
    if(index == -1)
        return result;
    result = rx.cap(1);
    if(result.trimmed().isEmpty())
        result = rx.cap(1);
    return result;
}
