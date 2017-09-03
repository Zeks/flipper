#pragma once
#include <QString>
#include <QDateTime>
struct Author{
    QString name;
    QHash<QString, QString> urls;
    QDateTime firstPublishedFic;
    QDateTime lastUpdated;
    int ficCount;
    int favCount;
    bool isValid = false;

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

enum class ERecommenderIdStatus
{
    unassigned = -1,
    not_found = -2,
    valid = 0
};

struct Recommender
{
    QString name;
    QString url;
    QString pageData;
    //type of website, ffn or ao3
    QString website;
    QString tag = "core";
    bool relevantForTag = true;
    int wave = 0;
    int id = -3;
    ERecommenderIdStatus idStatus = ERecommenderIdStatus::unassigned;
    void AssignId(int id){
        if(id == -1)
        {
            this->id = -1;
            this->idStatus = ERecommenderIdStatus::not_found;
        }
        if(id > -1)
        {
            this->id = id;
            this->idStatus = ERecommenderIdStatus::valid;
        }
    }
    ERecommenderIdStatus GetIdStatus() const {return idStatus;}
};

struct RecommenderStats
{
    int recommenderId= -1;
    int totalFics = -1;
    int matchesWithReferenceTag = -1;
    double matchRatio = -1;
    bool isValid = false;
    QString tag;
    QString authorName;
};
