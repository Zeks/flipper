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
#include "include/parsers/ffn/ffnparserbase.h"
#include <QString>
#include "sql_abstractions/sql_database.h"
#include <QDateTime>
#include <functional>

namespace interfaces {
class Genres;
}
struct AuthorMemo
{
    QSharedPointer<core::Author> author;
    QString newName;
};
class FicParser : public FFNParserBase
{
public:
    FicParser(QSharedPointer<interfaces::Fanfics> fanfics,
              QSharedPointer<interfaces::Genres> genres);
    QSharedPointer<core::Fanfic> ProcessPage(QString url,QString&);
    void ClearProcessed();
    //void WriteProcessed();
    void SetRewriteAuthorNames(bool);
private:
    void ProcessUnmarkedSections(core::FanficSectionInFFNFavourites& );
    void DetermineMarkedSubsectionPresence(core::FanficSectionInFFNFavourites& );
    core::FanficSectionInFFNFavourites GetSection(QString text);
    QString ExtractRecommenderNameFromUrl(QString url);
    void GetAuthor(core::FanficSectionInFFNFavourites& , QString text);
    void GetTitle(core::FanficSectionInFFNFavourites& , QString text);
    void GetGenre(core::FanficSectionInFFNFavourites& , int& startfrom, QString text);
    void GetSummary(core::FanficSectionInFFNFavourites& , QString text);
    void GetWordCount(core::FanficSectionInFFNFavourites& , int& startfrom, QString text);
    void GetPublishedDate(core::FanficSectionInFFNFavourites& , int& startfrom, QString text);
    void GetUpdatedDate(core::FanficSectionInFFNFavourites& , int& startfrom, QString text);
    void GetStatSection(core::FanficSectionInFFNFavourites& , QString text);
    void GetTaggedSection(QString text, QString rxString, std::function<void (QString)> functor, int skipCount = 0);
    core::FanficSectionInFFNFavourites::Tag GetStatTag(QString text, QString tag);
    void GetStatSectionTag(QString, QString text, core::FanficSectionInFFNFavourites::Tag *);
    void GetFandom(core::FanficSectionInFFNFavourites &section, int &startfrom, QString text);

    QSharedPointer<interfaces::Fanfics> fanfics;
    QSharedPointer<core::Author> queuedAuthor; //! todo will need to write this
    QSharedPointer<interfaces::Genres> genres;
    bool rewriteAuthorName = false;
};

