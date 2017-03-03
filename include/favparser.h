#pragma once
#include "include/section.h"
#include <QString>
#include <QDateTime>
#include <functional>
class FavouriteStoryParser
{
public:

    void ProcessPage(QString url,QString);
    Section GetSection( QString text, int start);
    QString ExtractRecommdenderNameFromUrl(QString url);
    void GetAuthor(Section& , int& startfrom, QString text);
    void GetTitle(Section& , int& startfrom, QString text);
    void GetGenre(Section& , int& startfrom, QString text);
    void GetSummary(Section& , int& startfrom, QString text);
    void GetWordCount(Section& , int& startfrom, QString text);
    void GetPublishedDate(Section& , int& startfrom, QString text);
    void GetUpdatedDate(Section& , int& startfrom, QString text);
    void GetUrl(Section& , int& startfrom, QString text);
    void GetStatSection(Section& , int& startfrom, QString text);
    void GetTaggedSection(QString text, QString tag, std::function<void(QString)> functor);
    void GetCrossoverFandomList(Section& , int& startfrom, QString text);
    QString GetFandom(QString text);
    QStringList diagnostics;
};
