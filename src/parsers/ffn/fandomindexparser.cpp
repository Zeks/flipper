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
#include "include/parsers/ffn/fandomindexparser.h"

void FFNFandomIndexParserBase::AddError(QString)
{
    hadErrors = true;
    diagnostics.push_back("failed to find the start of fandom section");
}

bool FFNFandomIndexParserBase::HadErrors() const
{
    return hadErrors;
}

void FFNFandomIndexParserBase::SetPage(WebPage page)
{
    hadErrors = false;
    diagnostics.clear();
    currentPage = page;
}

static QRegExp CreateRegex(QString value){
    return QRegExp(value);
}

void FFNFandomIndexParserBase::ProcessInternal(RegexProcessData procData)
{
    QString str(currentPage.content);
    int startAt = procData.rxStartFandoms.indexIn(str);
    if(startAt == -1)
    {
        AddError(QString("failed to find the start of %1 section").arg(procData.type));
        return;
    }

    if(procData.rxEndFandoms.indexIn(str) == -1)
    {
        AddError(QString("failed to find the end of %1 section").arg(procData.type));
        return;
    }

    while(true)
    {
        int linkStart = procData.rxStartLink.indexIn(str, startAt);
        if(linkStart == -1)
            break;
        int linkEnd= procData.rxEndLink.indexIn(str, linkStart);

        if(linkStart == -1 || linkEnd == -1)
        {
            AddError("failed to fetch link at: " +  str.mid(linkStart, str.size() - linkStart));
            break;
        }
        //qDebug() << str.mid(linkStart, linkEnd - linkStart);
        QString link = str.mid(linkStart + procData.rxStartLink.pattern().length() - procData.leftLinkAdjustment,
                               linkEnd - (linkStart + procData.rxStartLink.pattern().length()) + procData.rightLinkAdjustment);

        int nameStart = procData.rxStartName.indexIn(str, linkEnd);
        int nameEnd= procData.rxEndName.indexIn(str, nameStart);
        if(nameStart == -1 || nameEnd == -1)
        {
            AddError("failed to fetch name at: " + str.mid(nameStart, str.size() - nameStart));
            break;
        }

        QString name = str.mid(nameStart + procData.nameStartLength,
                               nameEnd - (nameStart + procData.nameStartLength));
        name.replace("\\'", "'");
        if(name.contains("Yu-Gi-Oh!"))
            startAt = linkEnd;

        startAt = linkEnd;

        auto fandom = core::Fandom::NewFandom();
        fandom->SetName(name);
        core::Url url(link, "ffn", procData.type);
        fandom->AddUrl(url);
        results.push_back(fandom);
    }
}


static RegexProcessData CreateFandomControls(){
    RegexProcessData controls;
    controls.rxStartFandoms = CreateRegex("list_output");
    controls.rxEndFandoms = CreateRegex("</TABLE>");
    controls.rxStartLink= CreateRegex("href=\"");
    controls.rxEndLink = CreateRegex("/\"");
    controls.rxStartName= CreateRegex("title=[\"]");
    controls.rxEndName = CreateRegex("[\"]>");
    controls.nameStartLength = 7;
    controls.leftLinkAdjustment = 0;
    controls.rightLinkAdjustment = 1;
    controls.type = "normal";
    return controls;
}

static RegexProcessData CreateCrossoverControls(){
    RegexProcessData controls;
    controls.rxStartFandoms = CreateRegex("<TABLE\\sWIDTH='100%'><TR>");
    controls.rxEndFandoms = CreateRegex("</TABLE>");
    controls.rxStartLink= CreateRegex("href=[\"]");
    controls.rxEndLink = CreateRegex("/\"");
    controls.rxStartName= CreateRegex("title=[\"]");
    controls.rxEndName = CreateRegex("[\"]>");
    controls.nameStartLength = 7;
    controls.leftLinkAdjustment = 2;
    controls.rightLinkAdjustment = 3;
    controls.type = "crossover";
    return controls;
}

void FFNCrossoverFandomParser::Process()
{
    thread_local RegexProcessData controls = CreateCrossoverControls();
    ProcessInternal(controls);
}

void FFNFandomParser::Process()
{
    thread_local RegexProcessData controls = CreateFandomControls();
    ProcessInternal(controls);
}
