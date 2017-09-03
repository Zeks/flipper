#pragma once
#include <QString>
#include <QDateTime>
enum class AuthorIdStatus
{
    unassigned = -1,
    not_found = -2,
    valid = 0
};

struct Author{
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
    int ficCount;
    int favCount;
    bool isValid = false;
    QString website;

    void SetUrl(QString type, QString url)
    {
        urls[type] = url;
    }
    QString url(QString type)
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
    QString rated= 0;

    QString fandom;
    QStringList fandoms;
    QString isCrossover = false;
    QString title;
    QString genre;
    QString summary;
    QString statSection;

    QString tags;
    QString origin;
    QString language;
    QDateTime published;
    QDateTime updated;
    QString characters;
    bool isValid =false;
    Author author;
    QHash<QString, QString> urls;
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

};

struct Section
{
    int start = 0;
    int end = 0;

    int summaryEnd = 0;
    int wordCountStart = 0;
    int statSectionStart=0;
    int statSectionEnd=0;
    QString statSection;

    Fic result;
    bool isValid =false;
};
struct Fandom
{
    QString name;
    QString section;
    QString url;
    QString crossoverUrl;
};




struct AuthorRecommendationStats
{
    int authorId= -1;
    int totalFics = -1;
    int matchesWithReferenceTag = -1;
    double matchRatio = -1;
    bool isValid = false;
    QString tag;
    QString authorName;
};
