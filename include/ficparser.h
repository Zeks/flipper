#pragma once
#include "include/section.h"
#include "include/db_ffn.h"
#include "include/ffnparserbase.h"
#include <QString>
#include <QSqlDatabase>
#include <QDateTime>
#include <functional>



class FicParser : public FFNParserBase
{
public:
    core::Fic ProcessPage(QString url,QString&);
    void ClearProcessed();
    void WriteProcessed(QHash<QString, int>& );
    void WriteJustAuthorName(core::Fic& fic);
    void SetRewriteAuthorNames(bool);
private:
    void ProcessUnmarkedSections(core::Section& );
    void DetermineMarkedSubsectionPresence(core::Section& );
    core::Section GetSection( QString text, int start);
    QString ExtractRecommenderNameFromUrl(QString url);
    void GetAuthor(core::Section& , int& startfrom, QString text);
    void GetTitle(core::Section& , int& startfrom, QString text);
    void GetGenre(core::Section& , int& startfrom, QString text);
    void GetSummary(core::Section& , int& startfrom, QString text);
    void GetWordCount(core::Section& , int& startfrom, QString text);
    void GetPublishedDate(core::Section& , int& startfrom, QString text);
    void GetUpdatedDate(core::Section& , int& startfrom, QString text);
    void GetStatSection(core::Section& , int& startfrom, QString text);
    void GetTaggedSection(QString text, core::Section::Tag tag, QString rxString, std::function<void (QString)> functor, int skipCount = 0);
    core::Section::Tag GetStatTag(QString text, QString tag);
    void GetStatSectionTag(QString, QString text, core::Section::Tag *);
    void GetFandom(core::Section &section, int &startfrom, QString text);
    void EnsureAuthorAvailable(const core::Fic&);

    database::WriteStats writeSections;
    bool rewriteAuthorName = false;
};

