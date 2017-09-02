#pragma once
#include <QString>
#include "section.h"
#include "pagegetter.h"
#include "init_database.h"
#include <functional>

class FandomParser
{
public:
    void ProcessPage(WebPage page);
    Section GetSection( QString text, int start);
    void GetAuthor(Section& , int& startfrom, QString text);
    void GetTitle(Section& , int& startfrom, QString text);
    void GetGenre(Section& , int& startfrom, QString text);
    void GetSummary(Section& , int& startfrom, QString text);
    void GetCrossoverFandomList(Section& , int& startfrom, QString text);
    void GetWordCount(Section& , int& startfrom, QString text);
    void GetPublishedDate(Section& , int& startfrom, QString text);
    void GetUpdatedDate(Section& , int& startfrom, QString text);
    void GetUrl(Section& , int& startfrom, QString text);
    QString GetNext(Section& , int& startfrom, QString text);
    void GetStatSection(Section& , int& startfrom, QString text);
    void GetTaggedSection(QString text, QString tag, std::function<void(QString)> functor);
    QString GetLast(QString pageContent);
    QString CreateURL(QString str);
    QString GetFandom(QString text);

    QStringList diagnostics;
    QList<Fic> processedStuff;
    database::WriteStats writeSections;
    Recommender recommender;
    QHash<QString, QString> alreadyDone;
    QString nextUrl;
    QDateTime minSectionUpdateDate;
};
