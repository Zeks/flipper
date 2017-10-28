#pragma once
#include "include/section.h"
#include "include/Interfaces/fanfics.h"
#include "include/ffnparserbase.h"
#include <QString>
#include <QSqlDatabase>
#include <QDateTime>
#include <functional>
class FavouriteStoryParser : public FFNParserBase
{
public:
    FavouriteStoryParser(){}
    FavouriteStoryParser(QSharedPointer<interfaces::Fanfics> fanfics);
    QList<QSharedPointer<core::Fic>> ProcessPage(QString url,QString&);
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
    void WriteRecommenderInfo();
    void SetCurrentTag(QString);


    QStringList diagnostics;
    QList<QSharedPointer<core::Fic>> processedStuff;

    core::FavouritesPage recommender;
    QHash<QString, QString> alreadyDone;
    QString currentTagMode = "core";
    QString authorName;
    //! todo needs to be filled

//    QSharedPointer<interfaces::Fanfics> fanfics;
//    QSharedPointer<interfaces::DataInterfaces> interfaces;

};

