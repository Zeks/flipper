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
#include <QString>
#include <QSharedPointer>
#include "section.h"
#include "pagegetter.h"
#include <functional>
#include "include/ffnparserbase.h"

class FandomParser : public FFNParserBase
{
public:
    FandomParser(QSharedPointer<interfaces::Fanfics> fanfics);
    void ProcessPage(WebPage page);
    core::Section GetSection( QString text, int start);
    //void GetAuthor(core::Section& , int& startfrom, QString text);
    void GetAuthor(core::Section&,int& startfrom, QString text);
    void GetTitle(core::Section& , int& startfrom, QString text);
    void GetSummary(core::Section& , int& startfrom, QString text);
    void GetCrossoverFandomList(core::Section& , int& startfrom, QString text);
    void GetWordCount(core::Section& , int& startfrom, QString text);
    void GetPublishedDate(core::Section& , int& startfrom, QString text);
    void GetUpdatedDate(core::Section& , int& startfrom, QString text);
    void GetUrl(core::Section& , int& startfrom, QString text);
    QString GetNext(core::Section& , int& startfrom, QString text);
    void GetStatSection(core::Section& , int& startfrom, QString text);
    void GetTaggedSection(QString text, QString tag, std::function<void(QString)> functor);
    QString GetLast(QString pageContent);
    QString CreateURL(QString str);
    QString GetFandom(QString text);

    QStringList diagnostics;
    QString nextUrl;
    QDate minSectionUpdateDate;

    // FFNParserBase interface
public:
    void WriteProcessed();
    void ClearProcessed();
};
