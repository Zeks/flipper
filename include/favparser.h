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

    QList<core::Fic> ProcessPage(QString url,QString&);
    core::Section GetSection( QString text, int start);
    QString ExtractRecommenderNameFromUrl(QString url);
    void GetAuthor(core::Section& , int& startfrom, QString text);
    void GetAuthorUrl(core::Section& , int& startfrom, QString text);
    void GetTitle(core::Section& , int& startfrom, QString text);
    void GetGenre(core::Section& , int& startfrom, QString text);
    void GetSummary(core::Section& , int& startfrom, QString text);
    void GetWordCount(core::Section& , int& startfrom, QString text);
    void GetPublishedDate(core::Section& , int& startfrom, QString text);
    void GetUpdatedDate(core::Section& , int& startfrom, QString text);
    void GetUrl(core::Section& , int& startfrom, QString text);
    void GetStatSection(core::Section& , int& startfrom, QString text);
    void GetTaggedSection(QString text, QString tag, std::function<void(QString)> functor);
    void GetCrossoverFandomList(core::Section& , int& startfrom, QString text);
    QString GetFandom(QString text);
    void ClearProcessed();
    void ClearDoneCache();
    void WriteProcessed();
    void WriteJustAuthorName();
    void WriteRecommenderInfo();
    void SetCurrentTag(QString);


    QStringList diagnostics;
    QList<core::Fic> processedStuff;
    database::WriteStats writeSections;
    core::FavouritesPage recommender;
    QHash<QString, QString> alreadyDone;
    QString currentTagMode = "core";
    QString authorName;

};

