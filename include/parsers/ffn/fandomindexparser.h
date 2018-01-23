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
