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
#pragma once
#include <QString>
#include <QDateTime>
#include <QSharedPointer>
#include <QRegExp>
#include <QHash>
namespace core {

class DBEntity{
public:
    bool HasChanges() const {return hasChanges;}
    virtual ~DBEntity(){}
    bool hasChanges = false;
};

enum class UpdateMode
{
    none = -1,
    insert = 0,
    update = 1
};

enum class AuthorIdStatus
{
    unassigned = -1,
    not_found = -2,
    valid = 0
};
class Author;
typedef QSharedPointer<Author> AuthorPtr;

enum class EntitySizeType{
    small = 0,
    medium = 2,
    big = 3,
    huge = 4
};

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

struct IdPack{
  int db;
  int ffn;
  int ao3;
  int sb;
  int sv;
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

    int favourites = -1;
    int ficWordCount = 0;
    double wordsPerChapter = 0;

    ESRBType esrbType;
    MoodType prevalentMood = MoodType::neutral;
    EntitySizeType mostFavouritedSize;
    EntitySizeType sectionRelativeSize;


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
    //double moodDiversityFactor = 0.0;

    QString prevalentGenre;
    QHash<int, double> sizeFactors;

    QHash<QString, int> fandoms;
    QHash<QString, double> fandomFactors;

    QHash<int, int> fandomsConverted;
    QHash<int, double> fandomFactorsConverted;

    QHash<QString, double> genreFactors;

    QDate firstPublished;
    QDate lastPublished;
};

class AuthorStats
{
public:
    QDate pageCreated;
    QDate bioLastUpdated;
    int bioWordCount = -1;

    FicSectionStats favouriteStats;
    FicSectionStats ownFicStats;
};

class Author : public DBEntity{
public:
    static AuthorPtr NewAuthor() { return AuthorPtr(new Author);}
    //FicSectionStats MergeStats(QList<AuthorPtr>);
    ~Author(){}
    void Log();
    void LogWebIds();
    int id= -1;
    AuthorIdStatus idStatus = AuthorIdStatus::unassigned;
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
    QString name;
    QDateTime firstPublishedFic;
    QDateTime lastUpdated;
    int ficCount = -1;
    int recCount = -1;
    int favCount = -1;
    bool isValid = false;
    //QString website = "";
    QHash<QString, int> webIds;
    QString CreateAuthorUrl(QString urlType, int webId) const;
    QString url(QString type) const
    {
        if(webIds.contains(type))
            return CreateAuthorUrl(type, webIds[type]);
        return "";
    }
    QStringList GetWebsites() const;
    UpdateMode updateMode = UpdateMode::none;
    AuthorStats stats;
};

class FavouritesPage
{
public:
    QSharedPointer<Author> author;
    QString pageData;
    //type of website, ffn or ao3

};

class Fic;
typedef QSharedPointer<Fic> FicPtr;

class Fic : public DBEntity{
public:
    class FicCalcStats
    {
    public:
        void Log();
        bool isValid = false;
        double wcr;
        double wcr_adjusted;
        double reviewsTofavourites;
        int age;
        int daysRunning;

    };
    Fic(){
        author = QSharedPointer<Author>(new Author);
    };
    Fic(const Fic&) = default;
    Fic& operator=(const Fic&) = default;
    ~Fic(){}
    void Log();
    void LogUrls();
    void LogWebIds();
    static FicPtr NewFanfic() { return QSharedPointer<Fic>(new Fic);}
    int complete=0;
    int atChapter=0;
    int webId = -1;
    int id = -1;

    QString wordCount;
    QString chapters;
    QString reviews;
    QString favourites;
    QString follows;
    QString rated;

    QString fandom;
    QStringList fandoms;
    bool isCrossover = false;
    QString title;
    //QString genres;
    QStringList genres;
    QString genreString;
    QString summary;
    QString statSection;

    QString tags;
    //QString origin;
    QString language;

    QDateTime published;
    QDateTime updated;
    QString charactersFull;
    QStringList characters;
    bool isValid =false;

    int authorId = -1;
    QString authorName;
    QSharedPointer<Author> author;


    QHash<QString, QString> urls;

    void SetGenres(QString genreString, QString website){

        this->genreString = genreString;
        QStringList genresList;
        bool hasHurt = false;
        QString fixedGenreString = genreString;
        if(website == "ffn" && genreString.contains("Hurt/Comfort"))
        {
            hasHurt = true;
            fixedGenreString = fixedGenreString.replace("Hurt/Comfort", "");
        }

        if(website == "ffn")
            genresList = fixedGenreString.split("/", QString::SkipEmptyParts);
        if(hasHurt)
            genresList.push_back("Hurt/Comfort");
        for(auto& genre: genresList)
            genre = genre.trimmed();
        genres = genresList;


    };
    QString url(QString type)
    {
        if(urls.contains(type))
            return urls[type];
        return "";
    }
    void SetUrl(QString type, QString url);

    int ffn_id = -1;
    int ao3_id = -1;
    int sb_id = -1;
    int sv_id = -1;
    QString urlFFN;
    int recommendations = 0;
    QString webSite = "ffn";
    UpdateMode updateMode = UpdateMode::none;
    FicCalcStats calcStats;
    QList<int> fandomIds;
};

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
    QString GetName() const {return this->name;}
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


class RecommendationList : public DBEntity{
public:
    static RecPtr NewRecList() { return QSharedPointer<RecommendationList>(new RecommendationList);}
    void Log();
    int id = -1;
    int ficCount =-1;
    QString name;
    QString tagToUse;
    int minimumMatch = -1;
    int alwaysPickAt = -2;
    double pickRatio = -1;
    QDateTime created;
};

}
Q_DECLARE_METATYPE(core::AuthorPtr);
