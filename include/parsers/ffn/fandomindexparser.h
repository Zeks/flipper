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
#include "core/section.h"
#include "pagegetter.h"

#include <QString>
#include <QStringList>
#include <QSharedPointer>

struct RegexProcessData
{
    QRegExp rxStartFandoms;
    QRegExp rxEndFandoms;
    QRegExp rxStartLink;
    QRegExp rxEndLink;
    QRegExp rxStartName;
    QRegExp rxEndName;
    int leftLinkAdjustment = 0;
    int rightLinkAdjustment = 0;
    int nameStartLength = 0;
    QString type = "fandom"; // or crossover
};

class FFNFandomIndexParserBase
{
    public:
    virtual ~FFNFandomIndexParserBase(){}
    virtual void Process() = 0;

    void SetPage(WebPage page);
    virtual void AddError(QString);
    bool HadErrors() const;
    QList<QSharedPointer<core::Fandom>> results;
protected:

    virtual void ProcessInternal(RegexProcessData);
    WebPage currentPage;
    bool hadErrors = false;
    QStringList diagnostics;

};

class FFNFandomParser : public FFNFandomIndexParserBase
{
    public:
    virtual void Process();
};

class FFNCrossoverFandomParser : public FFNFandomIndexParserBase
{
    public:
    virtual void Process();
};
