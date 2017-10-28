#pragma once
#include "include/section.h"
#include "include/ffnparserbase.h"
#include <QString>
#include <QSqlDatabase>
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
    QSharedPointer<core::Fic> ProcessPage(QString url,QString&);
    void ClearProcessed();
    void WriteProcessed();
    void SetRewriteAuthorNames(bool);
private:
    void ProcessUnmarkedSections(core::Section& );
    void DetermineMarkedSubsectionPresence(core::Section& );
    core::Section GetSection(QString text);
    QString ExtractRecommenderNameFromUrl(QString url);
    void GetAuthor(core::Section& , QString text);
    void GetTitle(core::Section& , QString text);
    void GetGenre(core::Section& , int& startfrom, QString text);
    void GetSummary(core::Section& , QString text);
    void GetWordCount(core::Section& , int& startfrom, QString text);
    void GetPublishedDate(core::Section& , int& startfrom, QString text);
    void GetUpdatedDate(core::Section& , int& startfrom, QString text);
    void GetStatSection(core::Section& , QString text);
    void GetTaggedSection(QString text, QString rxString, std::function<void (QString)> functor, int skipCount = 0);
    core::Section::Tag GetStatTag(QString text, QString tag);
    void GetStatSectionTag(QString, QString text, core::Section::Tag *);
    void GetFandom(core::Section &section, int &startfrom, QString text);

    QSharedPointer<core::Author> queuedAuthor; //! todo will need to write this
    QSharedPointer<interfaces::Genres> genres;
    bool rewriteAuthorName = false;
};

