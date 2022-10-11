/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

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

#include "Interfaces/db_interface.h"

#include <QString>
#include "sql_abstractions/sql_database.h"
#include <QDateTime>
#include <functional>

namespace interfaces {
class Fanfics;
class Authors;
}

class FFNParserBase
{
public:
    FFNParserBase(){}
    virtual ~FFNParserBase();
    virtual core::FanficSectionInFFNFavourites GetSection(QString text, QString sectionSeparator, int start);
    virtual void ProcessSection(core::FanficSectionInFFNFavourites &section, int &startfrom, QString str);
    virtual void ProcessGenres(core::FanficSectionInFFNFavourites & section, QString genreText);
    virtual void ProcessCharacters(core::FanficSectionInFFNFavourites & section, QString genreText);
    virtual void ProcessStatSection(core::FanficSectionInFFNFavourites & section);
    virtual void GetAuthor(core::FanficSectionInFFNFavourites & section, int &startfrom,  QString text);
    virtual void GetTitleAndUrl(core::FanficSectionInFFNFavourites & section, int& startfrom, QString text) = 0;
    virtual void GetTitle(core::FanficSectionInFFNFavourites & section, int& startfrom, QString text) = 0;
    virtual void GetSummary(core::FanficSectionInFFNFavourites & section, int& startfrom, QString text);
    virtual void GetStatSection(core::FanficSectionInFFNFavourites &section, int &startfrom, QString text);
    virtual void GetUrl(core::FanficSectionInFFNFavourites & section, int& startfrom, QString text);
    virtual void GetTaggedSection(QString text, QRegExp& rx,std::function<void (QString)> functor);
    virtual void GetFandomFromTaggedSection(core::FanficSectionInFFNFavourites & section,QString text);

    virtual void GetCharacters(QString text, std::function<void(QString)> functor);
    virtual void GetCrossoverFandomList(core::FanficSectionInFFNFavourites & section, QString text);
    virtual void ClearProcessed() = 0;

    //QSharedPointer<interfaces::Fanfics> fanfics;
    QList<QSharedPointer<core::Fanfic>> processedStuff;
    QStringList diagnostics;
};

