/*
FFSSE is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
    void GetAuthorId(core::Section& , int& startfrom, QString text);
    void GetTitle(core::Section& , int& startfrom, QString text);
    void GetGenre(core::Section& , int& startfrom, QString text);
    void GetSummary(core::Section& , int& startfrom, QString text);
    void GetWordCount(core::Section& , int& startfrom, QString text);
    void GetPublishedDate(core::Section& , int& startfrom, QString text);
    void GetUpdatedDate(core::Section& , int& startfrom, QString text);
    void GetUrl(core::Section& , int& startfrom, QString text);
    void GetStatSection(core::Section& , int& startfrom, QString text);
    void GetTaggedSection(QString text, QString tag, std::function<void(QString)> functor);
    void GetCharacters(QString text, std::function<void(QString)> functor);
    void GetCrossoverFandomList(core::Section& , int& startfrom, QString text);
    QString GetFandom(QString text);
    void ClearProcessed();
    void ClearDoneCache();
    void WriteProcessed();
    void WriteRecommenderInfo();
    void SetCurrentTag(QString);
    void SetAuthor(core::AuthorPtr);

    QStringList diagnostics;
    QList<QSharedPointer<core::Fic>> processedStuff;

    core::FavouritesPage recommender;
    QHash<QString, QString> alreadyDone;
    QString currentTagMode = "core";
    QString authorName;
    QSet<QString> fandoms;
    //! todo needs to be filled

    //QSharedPointer<interfaces::Fandoms> fandomInterface;
//    QSharedPointer<interfaces::DataInterfaces> interfaces;

};

