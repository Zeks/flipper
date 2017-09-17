#pragma once
#include <QString>
#include <QDateTime>
namespace core {


enum class AuthorIdStatus
{
    unassigned = -1,
    not_found = -2,
    valid = 0
};

struct Author{
    void Log();
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
    QString name;
    QHash<QString, QString> urls;
    QDateTime firstPublishedFic;
    QDateTime lastUpdated;
    int ficCount = -1;
    int favCount = -1;
    bool isValid = false;
    QString website = "";

    void SetUrl(QString type, QString url)
    {
        urls[type] = url;
    }
    QString url(QString type) const
    {
        if(urls.contains(type))
            return urls[type];
        return "";
    }
};

struct FavouritesPage
{
    Author author;
    QString pageData;
    //type of website, ffn or ao3

};


struct Fic{
    int complete=0;
    int atChapter=0;
    int webId = -1;
    int ID = -1;

    QString wordCount = 0;
    QString chapters = 0;
    QString reviews = 0;
    QString favourites= 0;
    QString follows= 0;
    QString rated= 0;

    QString fandom;
    QStringList fandoms;
    QString isCrossover = false;
    QString title;
    //QString genres;
    QStringList genres;
    QString genreString;
    QString summary;
    QString statSection;

    QString tags;
    QString origin;
    QString language;
    QDateTime published;
    QDateTime updated;
    QString charactersFull;
    QStringList characters;
    bool isValid =false;
    Author author;
    QHash<QString, QString> urls;
    void SetGenres(QString genreString, QString website){

        this->genreString = genreString;
        QStringList genresList;
        if(website == "ffn" && genreString.contains("Hurt/Comfort"))
        {
            genreString.replace("Hurt/Comfort", "").split("/");
        }
        if(website == "ffn")
            genresList = genreString.split("/");
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
    void SetUrl(QString type, QString url)
    {
        urls[type] = url;
        urlFFN = url;
    }
    QString urlFFN;
    int recommendations = 0;
    QString webSite = "ffn";
};

struct Section
{
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
    Fic result;
    bool isValid =false;
};
struct Fandom
{
    Fandom(){}
    Fandom(QString name){this->name = name;}
    Fandom(QString name,QString section,QString url,QString crossoverUrl, QString source = "ffn"){
        this->name = name.trimmed();
        this->section = section.trimmed();
        this->url = url.trimmed();
        this->crossoverUrl = crossoverUrl.trimmed();
        this->source = source.trimmed();
    }
    int id = -1;
    QString name;
    QString section = "none";
    QString url = "none";
    QString crossoverUrl = "none";
    QString source = "ffn";
};




struct AuthorRecommendationStats
{
    int authorId= -1;
    int totalFics = -1;
    int matchesWithReferenceTag = -1;
    double matchRatio = -1;
    bool isValid = false;
    QString listName;
    QString usedTag;
    QString authorName;
};

struct RecommendationList{
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

