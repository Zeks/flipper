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
#include "core/section.h"
#include "pagegetter.h"
#include <functional>
#include "include/parsers/ffn/ffnparserbase.h"

class FandomParser : public FFNParserBase
{
public:
    FandomParser(QSharedPointer<interfaces::Fanfics> fanfics);
    void ProcessPage(WebPage page);
    void GetWordCount(core::Section& , int& startfrom, QString text);
    void GetPublishedDate(core::Section& , int& startfrom, QString text);
    void GetUpdatedDate(core::Section& , int& startfrom, QString text);
    QString GetNext(int& startfrom, QString text);
    QString GetLast(QString pageContent);
    QString CreateURL(QString str);
    void GetTitle(core::Section & section, int& startfrom, QString text) override;
    virtual void GetTitleAndUrl(core::Section & , int& , QString ) override;
    QStringList diagnostics;
    QString nextUrl;
    QDate minSectionUpdateDate;

public:
    void ClearProcessed() override;
};
