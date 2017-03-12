#pragma once
#include "include/section.h"
#include "include/init_database.h"
#include <QString>
#include <QSqlDatabase>
#include <QDateTime>
#include <functional>
class FavouriteStoryParser
{
public:

    QList<Section> ProcessPage(QString url,QString&, int authorWave = 0);
    Section GetSection( QString text, int start);
    QString ExtractRecommdenderNameFromUrl(QString url);
    void GetAuthor(Section& , int& startfrom, QString text);
    void GetAuthorUrl(Section& , int& startfrom, QString text);
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
    void ClearProcessed();
    void ClearDoneCache();
    void WriteProcessed();


    QStringList diagnostics;
    QList<Section> processedStuff;
    database::WriteStats writeSections;
    Recommender recommender;
    QHash<QString, QString> alreadyDone;

};

