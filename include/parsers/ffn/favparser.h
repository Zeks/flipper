/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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
#include "include/core/section.h"
#include "include/core/author.h"
#include "include/Interfaces/fanfics.h"
#include "include/parsers/ffn/ffnparserbase.h"
#include "regex_utils.h"
#include <QString>
#include <QSqlDatabase>
#include <QDateTime>
#include <functional>

class FavouritesPage
{
public:
    QSharedPointer<core::Author> author;
    QString pageData;
    //type of website, ffn or ao3

};

class FavouriteStoryParser : public FFNParserBase
{
public:
    FavouriteStoryParser(){
        if(!commonRegex.initComplete)
            commonRegex.Init();
    }
    FavouriteStoryParser(QSharedPointer<interfaces::Fanfics> fanfics);
    QList<QSharedPointer<core::Fanfic>> ProcessPage(QString url,QString&);
    QString ExtractRecommenderNameFromUrl(QString url);
    void GetGenre(core::FanficSectionInFFNFavourites& , int& startfrom, QString text);
    void GetWordCount(core::FanficSectionInFFNFavourites& , int& startfrom, QString text);
    void GetPublishedDate(core::FanficSectionInFFNFavourites& , int& startfrom, QString text);
    void GetUpdatedDate(core::FanficSectionInFFNFavourites& , int& startfrom, QString text);
    QString GetFandom(QString text);
    virtual void GetFandomFromTaggedSection(core::FanficSectionInFFNFavourites & section,QString text) override;
    void GetTitle(core::FanficSectionInFFNFavourites & , int& , QString ) override;
    virtual void GetTitleAndUrl(core::FanficSectionInFFNFavourites & , int& , QString ) override;
    void ClearProcessed() override;
    void ClearDoneCache();
    void SetCurrentTag(QString);
    void SetAuthor(core::AuthorPtr);
    void UpdateWordsCounterNew(QSharedPointer<core::Fanfic> fic,
                                      const CommonRegex& regexToken,
                                      QHash<int, int>& wordsKeeper);

    //QSharedPointer<interfaces::Fandoms> fandomInterface;
    static void MergeStats(core::AuthorPtr,  QSharedPointer<interfaces::Fandoms> fandomsInterface, QList<FavouriteStoryParser> parsers);
    static void MergeStats(core::AuthorPtr author, QSharedPointer<interfaces::Fandoms> fandomsInterface, QList<core::FicSectionStatsTemporaryToken> tokens);
    core::FicSectionStatsTemporaryToken statToken;
    FavouritesPage recommender;
    QHash<QString, QString> alreadyDone;
    QString currentTagMode = "core";
    QString authorName;
    QSet<QString> fandoms;
    core::AuthorPtr authorStats;
    CommonRegex commonRegex;
    QSet<int> knownSlashFics;
};

QString ParseAuthorNameFromFavouritePage(QString data);
