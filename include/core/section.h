/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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
#pragma once
#include <QString>
#include <QDateTime>
#include <QSharedPointer>
#include <QRegExp>
#include <QHash>
#include <QTextStream>
#include <QDataStream>
#include <QSet>
#include <array>
#include "reclist_author_result.h"
#include "core/fic_genre_data.h"
#include "core/identity.h"
#include "core/db_entity.h"
#include "core/fanfic.h"
namespace core {



enum class AuthorIdStatus
{
    unassigned = -1,
    not_found = -2,
    valid = 0
};
class Author;
typedef QSharedPointer<Author> AuthorPtr;

enum class EntitySizeType{
    none = 0,
    small = 1,
    medium = 2,
    big = 3,
    huge = 4
};

enum class ExplorerRanges{
    none = 0,
    barely_known= 1,
    relatively_unknown = 2,
    popular = 3
};
core::EntitySizeType ProcesWordcountIntoSizeCategory(int);
core::ExplorerRanges ProcesFavouritesIntoPopularityCategory(int);

class FicSectionStatsTemporaryToken
{
    public:
    QHash<int, int> ficSizeKeeper;
    QHash<int, int> crossKeeper;
    QHash<int, int> favouritesSizeKeeper;
    QHash<int, int> popularUnpopularKeeper;
    QHash<QString, int> fandomKeeper;
    QHash<int, int> unfinishedKeeper;
    QHash<int, int> esrbKeeper;
    QHash<int, int> wordsKeeper;
    QHash<QString, int> genreKeeper;
    QHash<int, int> moodKeeper;
    int chapterKeeper = 0;
    QList<int> sizes;
    QDate firstPublished;
    QDate lastPublished;
    int ficCount = 0;
    int wordCount = 0;
    int authorId = -1;
    QDate bioLastUpdated;
    QDate pageCreated;
};



class FicSectionStats{
public:
    enum class FavouritesType{
        tiny = 0 ,
        medium = 1,
        large = 2,
        bs = 3
    };
    enum class MoodType{
        sad = 0,
        neutral = 1,
        positive = 2,
    };
    enum class ESRBType{
        agnostic = 0,
        kiddy = 1,
        mature = 2,
    };
    bool isValid = false;
    int favourites = -1;
    int noInfoCount = 0;
    int ficWordCount = 0;

    double averageWordsPerChapter = 0;
    double averageLength = 0.0;
    double fandomsDiversity = 0.0;
    double explorerFactor = 0.0;
    double megaExplorerFactor = 0.0;
    double crossoverFactor = 0.0;
    double unfinishedFactor = 0.0;
    double esrbUniformityFactor = 0.0;
    double esrbKiddy = 0.0;
    double esrbMature= 0.0;
    double genreDiversityFactor = 0.0;
    double moodUniformity = 0.0;
    double moodNeutral = 0.0;
    double moodSad = 0.0;
    double moodHappy = 0.0;


    double crackRatio = 0.0;
    double slashRatio = 0.0;
    double notSlashRatio = 0.0;
    double smutRatio = 0.0;

    ESRBType esrbType;
    MoodType prevalentMood = MoodType::neutral;
    EntitySizeType mostFavouritedSize;
    EntitySizeType sectionRelativeSize;

    QString prevalentGenre;
    QHash<QString, double> genreFactors;

    QMap<int, double> sizeFactors;

    QHash<QString, int> fandoms;
    QHash<QString, double> fandomFactors;

    QHash<int, int> fandomsConverted;
    QHash<int, double> fandomFactorsConverted;



    QDate firstPublished;
    QDate lastPublished;

    void Serialize(QDataStream &out);
    void Deserialize(QDataStream &in);
};
class AuthorStats
{
public:
    QDate pageCreated;
    QDate bioLastUpdated;
    QDate favouritesLastUpdated;
    QDate favouritesLastChecked;
    int bioWordCount = -1;

    FicSectionStats favouriteStats;
    FicSectionStats ownFicStats;
    void Serialize(QDataStream &out);
    void Deserialize(QDataStream &in);
};

class Author : public DBEntity{
public:
    static AuthorPtr NewAuthor() { return AuthorPtr(new Author);}
    //FicSectionStats MergeStats(QList<AuthorPtr>);
    ~Author(){}
    void Log();
    void LogWebIds();
    void AssignId(int id){
        if(id == -1)
        {
            this->id = -1;
            this->idStatus = AuthorIdStatus::not_found;
        }
        if(id > -1)
        {
            this->id = id;
            this->idStatus = AuthorIdStatus::valid;
        }
    }
    AuthorIdStatus GetIdStatus() const {return idStatus;}
    void SetWebID(QString website, int id){webIds[website] = id;}
    int GetWebID(QString website) {
        if(webIds.contains(website))
            return webIds[website];
        return -1;
    }
    QString CreateAuthorUrl(QString urlType, int webId) const;
    QString url(QString type) const
    {
        if(webIds.contains(type))
            return CreateAuthorUrl(type, webIds[type]);
        return "";
    }
    QStringList GetWebsites() const;

    int id= -1;
    AuthorIdStatus idStatus = AuthorIdStatus::unassigned;
    QString name;
    QDateTime firstPublishedFic;
    QDateTime lastUpdated;
    int ficCount = -1;
    int recCount = -1;
    int favCount = -1;
    bool isValid = false;
    QHash<QString, int> webIds;
    UpdateMode updateMode = UpdateMode::none;

    AuthorStats stats;

    void Serialize(QDataStream &out);
    void Deserialize(QDataStream &in);

};

class FavouritesPage
{
public:
    QSharedPointer<Author> author;
    QString pageData;
    //type of website, ffn or ao3

};
class Fic;



struct FicForWeightCalc
{
    bool complete = false;
    bool slash = false;
    bool dead = false;
    bool sameLanguage = true;
    bool adult = false;

    int id = -1;
    int sumAuthorFaves = -1;
    int favCount = -1;
    int reviewCount = -1;
    int wordCount = -1;
    int chapterCount = 0;
    int authorId = -1;

    QList<int> fandoms;
    QStringList genres;

    QString genreString;

    QDate published;
    QDate updated;


    friend  QTextStream &operator<<(QTextStream &out, const FicForWeightCalc &p);
    friend  QTextStream &operator>>(QTextStream &in, FicForWeightCalc &p);



    void Serialize(QDataStream &out)
    {
        out << id;
        out << adult;
        out << authorId;
        out << complete;
        out << dead;
        out << favCount;
        out << genreString;
        out << published;
        out << updated;
        out << reviewCount;
        out << sameLanguage;
        out << slash;
        out << wordCount;

        out << fandoms.size();
        for(auto fandom: fandoms)
            out << fandom;
    }
    void Log()
    {
        qDebug() << id;
        qDebug() << adult;
        qDebug() << authorId;
        qDebug() << complete;
        qDebug() << dead;
        qDebug() << favCount;
        qDebug() << genreString;
        qDebug() << published;
        qDebug() << updated;
        qDebug() << reviewCount;
        qDebug() << sameLanguage;
        qDebug() << slash;
        qDebug() << wordCount;

        qDebug() << fandoms.size();
        for(auto fandom: fandoms)
            qDebug() << fandom;
    }

    void Deserialize(QDataStream &in)
    {
        in >> id;
        //qDebug() << "id: " << id;
        in >> adult;
        //qDebug() << "adult: " << adult;
        in >> authorId;
        //qDebug() << "authorId " << authorId;
        in >> complete;
        //qDebug() << "complete: " << complete;
        in >> dead;
        //qDebug() << "dead: " << dead;
        in >> favCount;
        //qDebug() << "favCount: " << favCount;
        in >> genreString;
        //qDebug() << "genreString: " << genreString;
        in >> published;
        //qDebug() << "published: " << published;
        in >> updated;
        //qDebug() << "updated: " << updated;
        in >> reviewCount;
        //qDebug() << "reviewCount: " << reviewCount;
        in >> sameLanguage;
        //qDebug() << "sameLanguage: " << sameLanguage;
        in >> slash;
        //qDebug() << "slash: " << slash;
        in >> wordCount;
        //qDebug() << "wordCount: " << wordCount;
        int size = -1;
        in >> size;
        if(size > 2 || size < 0)
            qDebug() << "crap fandom size for fic: " << id << " size:" << size;
        int fandom = -1;
        for(int i = 0; i < size; i++)
        {
            in >> fandom;
            fandoms.push_back(fandom);
        }
    }

};
inline QTextStream &operator>>(QTextStream &in, FicForWeightCalc &p)
{
    QString temp;

    in >> temp;
    p.id = temp.toInt();

    in >> temp;
    p.adult = temp.toInt();

    in >> temp;
    p.authorId = temp.toInt();

    in >> temp;
    p.complete = temp.toInt();

    in >> temp;
    p.dead = temp.toInt();

    in >> temp;
    p.favCount = temp.toInt();

    in >> p.genreString;
    if(p.genreString == "#")
        p.genreString.clear();
    p.genreString.replace("_", " ");

    in >> temp;
    p.published = QDate::fromString("yyyyMMdd");

    in >> temp;
    p.updated = QDate::fromString("yyyyMMdd");

    in >> temp;
    p.reviewCount = temp.toInt();

    in >> temp;
    p.sameLanguage = temp.toInt();

    in >> temp;
    p.slash = temp.toInt();

    in >> temp;
    p.wordCount = temp.toInt();

    in >> temp;
    int fandomSize = temp.toInt();

    for(int i = 0; i < fandomSize; i++)
    {
        in >> temp;
        p.fandoms.push_back(temp.toInt());
    }

    return in;
}

inline QTextStream &operator<<(QTextStream &out, const FicForWeightCalc &p)
{
    out << QString::number(p.id) << " ";
    out << QString::number(static_cast<int>(p.adult)) << " ";
    out << QString::number(p.authorId) << " ";
    out << QString::number(p.complete) << " ";
    out << QString::number(p.dead) << " ";
    out << QString::number(p.favCount) << " ";
    if(p.genreString.trimmed().isEmpty())
        out << "#" << " ";
    else
        out << p.genreString << " ";
    if(p.published.isValid())
        out << p.published.toString("yyyyMMdd") << " ";
    else
        out << "0" << " ";
    if(p.updated.isValid())
        out << p.updated.toString("yyyyMMdd") << " ";
    else
        out << "0" << " ";

    out << QString::number(p.reviewCount) << " ";
    out << QString::number(p.sameLanguage) << " ";
    out << QString::number(p.slash) << " ";
    out << QString::number(p.wordCount) << " ";

    out << QString::number(p.fandoms.size()) << " ";

    for(auto fandom: p.fandoms)
        out << QString::number(fandom) << " ";
    out << "\n";

    return out;
}



struct FandomStatsForWeightCalc{
  int listId = -1;
  int fandomCount = -1;
  int ficCount = -1;

  double fandomDiversity = 0.0;

  QHash<int, double> fandomPresence;
  QHash<int, int> fandomCounts;


  void Serialize(QDataStream &out)
  {
        out << listId;
        out << fandomCount;
        out << ficCount;
        out << fandomDiversity;

        out << fandomPresence;
        out << fandomCounts;
  }

  void Deserialize(QDataStream &in)
  {
      in >> listId;
      in >> fandomCount;
      in >> ficCount;
      in >> fandomDiversity;

      in >> fandomPresence;
      in >> fandomCounts;

  }

};


struct FicWeightResult{
    int ficId1;
    int ficId2;
    int ficListCount1;
    int ficListCount2;
    int meetingCount;
    bool sameFandom;
    double attraction;
    double repulsion;
    double finalAttraction;
};

typedef QSharedPointer<FicForWeightCalc> FicWeightPtr;
typedef QSharedPointer<FandomStatsForWeightCalc> AuthorFavFandomStatsPtr;

class Section : public DBEntity
{
public:
    Section();
    struct Tag
    {
        Tag(){}
        Tag(QString value, int end){
            this->value = value;
            this->end = end;
            isValid = true;
        }
        QString marker;
        QString value;
        int position = -1;
        int end = -1;
        bool isValid = false;
    };
    struct StatSection{
        // these are tagged if they appear
        Tag rated;
        Tag reviews;
        Tag chapters;
        Tag words;
        Tag favs;
        Tag follows;
        Tag published;
        Tag updated;
        Tag status;
        Tag id;

        // these can only be inferred based on their content
        Tag genre;
        Tag language;
        Tag characters;

        QString text;
        bool success = false;
    };

    int start = 0;
    int end = 0;

    int summaryEnd = 0;
    int wordCountStart = 0;
    int statSectionStart=0;
    int statSectionEnd=0;

    StatSection statSection;
    QSharedPointer<Fic> result;
    bool isValid =false;
};
class Fandom;
typedef  QSharedPointer<Fandom> FandomPtr;

class Url
{
public:
    Url(QString url, QString source, QString type = "default"){
        this->url = url;
        this->source = source;
        this->type = type;
    }
    QString GetUrl(){return url;}
    QString GetSource(){return source;}
    QString GetType(){return type;}
    void SetType(QString value) {type = value;}
private:
    QString url;
    QString source;
    QString type;
};

class Fandom : public DBEntity
{
public:
    Fandom(){}
    Fandom(QString name){this->name = ConvertName(name);}
    //    Fandom(QString name,QString section,QString source = "ffn"){
    //        this->name = ConvertName(name);
    //        this->section = section.trimmed();
    //        this->source = source.trimmed();
    //    }
    static FandomPtr NewFandom() { return QSharedPointer<Fandom>(new Fandom);}
    QList<Url> GetUrls(){
        return urls;
    }
    void AddUrl(Url url){
        urls.append(url);
    }
    void SetName(QString name){this->name = ConvertName(name);}
    QString GetName() const;
    int id = -1;
    int idInRecentFandoms = -1;
    int ficCount = 0;
    double averageFavesTop3 = 0.0;

    QString section = "none";
    QString source = "ffn";
    QList<Url> urls;
    QDate dateOfCreation;
    QDate dateOfFirstFic;
    QDate dateOfLastFic;
    QDate lastUpdateDate;
    bool tracked = false;
    static QString ConvertName(QString name)
    {
        thread_local QHash<QString, QString> cache;
        name=name.trimmed();
        QString result;
        if(cache.contains(name))
            result = cache[name];
        else
        {
            QRegExp rx = QRegExp("(/(.|\\s){0,}[^\\x0000-\\x007F])|(/(.|\\s){0,}[?][?][?])");
            rx.setMinimal(true);
            int index = name.indexOf(rx);
            if(index != -1)
                cache[name] = name.left(index).trimmed();
            else
                cache[name] = name.trimmed();
            result = cache[name];
        }
        return result;
    }
private:
    QString name;

};


class AuthorRecommendationStats;
typedef QSharedPointer<AuthorRecommendationStats> AuhtorStatsPtr;

class AuthorRecommendationStats : public DBEntity
{
public:
    static AuhtorStatsPtr NewAuthorStats() { return QSharedPointer<AuthorRecommendationStats>(new AuthorRecommendationStats);}
    int authorId= -1;
    int totalRecommendations = -1;
    int matchesWithReference = -1;
    double matchRatio = -1;
    bool isValid = false;
    //QString listName;
    int listId = -1;
    QString usedTag;
    QString authorName;
};

struct FicRecommendation
{
    QSharedPointer<core::Fic> fic;
    QSharedPointer<core::Author> author;
    bool IsValid(){
        if(!fic || !author)
            return false;
        return true;
    }
};

class RecommendationList;
typedef QSharedPointer<RecommendationList> RecPtr;

struct RecommendationListFicData
{
    int id = -1;
    QSet<int> sourceFics;
    QVector<int> fics;
    QVector<int> purges;
    QVector<int> matchCounts;
    QVector<double> noTrashScores;
    QVector<int> authorIds;
    QHash<int, int> matchReport;
    QHash<int, core::MatchBreakdown> breakdowns;
};


class RecommendationList : public DBEntity{
public:
    static RecPtr NewRecList() { return QSharedPointer<RecommendationList>(new RecommendationList);}
    void Log();
    void PassSetupParamsInto(RecommendationList& other);
    bool success = false;
    bool isAutomatic = true;
    bool useWeighting = false;
    bool useMoodAdjustment = false;
    bool hasAuxDataFilled = false;
    bool useDislikes = false;
    bool useDeadFicIgnore= false;
    bool assignLikedToSources = false;

    int id = -1;
    int ficCount =-1;
    int minimumMatch = -1;
    int alwaysPickAt = -2;
    int maxUnmatchedPerMatch = -1;
    int userFFNId = -1;
    int sigma2Distance = -1;

    double quadraticDeviation = -1;
    double ratioMedian = -1;

    QString name;
    QString tagToUse;

    QSet<int> ignoredFandoms;
    QSet<int> ignoredDeadFics;
    QSet<int> likedAuthors;
    QSet<int> minorNegativeVotes;
    QSet<int> majorNegativeVotes;
    QDateTime created;
    RecommendationListFicData ficData;

    QStringList errors;
};


struct MatchedFics{
    int ratio = 0;
    int ratioWithoutIgnores = 0;
    QList<int> matches;
};

struct SnoozeInfo{
    int ficId = -1;
    bool finished = false;
    int atChapter = -1;
};
struct SnoozeTaskInfo{
    int ficId = -1;
    bool untilFinished = false;
    int snoozedAtChapter = -1;
    int snoozedTillChapter = -1;
    bool expired = false;
    QDateTime added;
};
}
Q_DECLARE_METATYPE(core::AuthorPtr);

