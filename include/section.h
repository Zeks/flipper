#pragma once
#include <QString>
#include <QDateTime>
struct Section
{
    int ID = -1;
    int start = 0;
    int end = 0;

    int summaryEnd = 0;
    int wordCountStart = 0;
    int statSectionStart=0;
    int statSectionEnd=0;
    int complete=0;
    int atChapter=0;
    int recommendations=0;

    QString wordCount = 0;
    QString chapters = 0;
    QString reviews = 0;
    QString favourites= 0;
    QString rated= 0;


    QString fandom;
    QString isCrossover = false;
    QString title;
    QString genre;
    QString summary;
    QString statSection;
    QString author;
    QString authorUrl;
    QString url;
    QString tags;
    QString origin;
    QString language;
    QDateTime published;
    QDateTime updated;
    QString characters;
    bool isValid =false;
};
struct Fandom
{
    QString name;
    QString section;
    QString url;
    QString crossoverUrl;
};

struct Recommender
{
    QString name;
    QString url;
    QString pageData;
    int wave = 0;
    int id = -3;
};
