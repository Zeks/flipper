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
    void ProcessPage(WebPage page);
    core::Section GetSection( QString text, int start);
    void GetAuthor(core::Section& , int& startfrom, QString text);
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
    QDateTime minSectionUpdateDate;

    // FFNParserBase interface
public:
    void WriteProcessed();
    void ClearProcessed();
};
