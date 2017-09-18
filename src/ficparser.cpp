#include "include/ficparser.h"
#include "include/section.h"
#include "include/db_ffn.h"
#include "include/regex_utils.h"
#include "include/url_utils.h"
#include <QDebug>
#include <QSqlDatabase>
#include <algorithm>
#include <chrono>

core::Fic FicParser::ProcessPage(QString url, QString& str)
{
    core::Section section;
    int currentPosition = 0;
    if(str.contains("Unable to locate story. Code 1"))
    {
        database::TryDeactivate(url, "ffn");
        return section.result;
    }

    section = GetSection(str, currentPosition);
    section.result.webId = url_utils::GetWebId(url, "ffn").toInt();
    qDebug() << "Processing fic: " << section.result.webId;
    currentPosition = section.start;

    GetAuthor(section, currentPosition, str);
    WriteJustAuthorName(section.result);

    GetFandom(section, currentPosition, str);
    GetTitle(section, currentPosition, str);
    GetSummary(section, currentPosition, str);
    GetStatSection(section, currentPosition, str);

    DetermineMarkedSubsectionPresence(section);
    ProcessUnmarkedSections(section);

    auto& stat = section.statSection;

    // reading marked section data
    GetTaggedSection(stat.text, stat.rated, "target='rating'>Fiction\\s\\s([A-Z])", [&section](QString val){ section.result.rated = val;});
    GetTaggedSection(stat.text, stat.chapters, "Chapters:\\s(\\d+)", [&section](QString val){ section.result.chapters = val;});
    GetTaggedSection(stat.text, stat.words, "Words:\\s(\\d+,\\d+)", [&section](QString val){section.result.wordCount = val;});
    GetTaggedSection(stat.text, stat.reviews, "(\\d+)</a>", [&section](QString val){ section.result.reviews = val;});
    GetTaggedSection(stat.text, stat.favs, "Favs:\\s(\\d+)", [&section](QString val){ section.result.favourites = val;});
    GetTaggedSection(stat.text, stat.follows, "Follows:\\s(\\d+)", [&section](QString val){ section.result.follows = val;});
    GetTaggedSection(stat.text, stat.status, "Status:\\s(\\d+)", [&section](QString val){
        if(val != "not found")
            section.result.complete = 1;}
    );
    GetTaggedSection(stat.text, stat.updated, "data-xutime='(\\d+)'", [&section](QString val){
        if(val != "not found")
        {
            section.result.updated.setTime_t(val.toInt());
            qDebug() << val << section.result.updated;
        }
    });
    GetTaggedSection(stat.text, stat.published, "data-xutime='(\\d+)'", [&section](QString val){
        if(val != "not found")
        {
            section.result.published.setTime_t(val.toInt());
            qDebug() << val << section.result.published;
        }
    }, 1);
    // if something doesn't have update date then we've read "published" incorrectly, fixing
    if(!section.statSection.updated.isValid)
        section.result.published = section.result.updated;
    section.result.isValid = true;
    processedStuff = section.result;
    return section.result;
}

core::Section::Tag FicParser::GetStatTag(QString text, QString tag)
{
    core::Section::Tag result;
    result.marker = tag;
    QRegExp rx(tag);
    auto index = rx.indexIn(text);
    if(index == -1)
        return result;
    result.isValid = true;
    result.position = index;
    return result;
}

void FicParser::GetTaggedSection(QString text, core::Section::Tag tag, QString rxString, std::function<void (QString)> functor, int skipCount)
{
    QRegExp rx(rxString);
    int indexStart = rx.indexIn(text);
    while(skipCount != 0)
    {
        indexStart = rx.indexIn(text, indexStart + 1);
        skipCount--;
    }
    if(indexStart != 1 && !rx.cap(1).trimmed().replace("-","").isEmpty())
        functor(rx.cap(1));
    else
        functor("not found");
}

void FicParser::ProcessUnmarkedSections(core::Section & section)
{
    auto& stat = section.statSection;
    // step 0, process rating to find the end of it
    QRegExp rxRated("'rating'>Fiction\\s\\s(.)");
    auto index = rxRated.indexIn(stat.text);
    int ratedTagLength = 11;
    if(index == -1)
        return;
    stat.rated = {rxRated.cap(1), index + ratedTagLength};
    // step 1, get language marker as it is necessary and always follows rating
    QRegExp rxlanguage("\\s-\\s(.*)(?=\\s-\\s)");
    rxlanguage.setMinimal(true);
    index = rxlanguage.indexIn(stat.text);
    int separatorTagLength = 3;
    if(index == -1)
        return;
    stat.language = core::Section::Tag(rxlanguage.cap(1), index + separatorTagLength + rxlanguage.cap(1).length());

    // step 2 decide if we have one or two sections between language and next found tagged position
    int nextTaggedSectionPosition = -1;
    if(stat.chapters.position != -1)
        nextTaggedSectionPosition = stat.chapters.position;
    else
        nextTaggedSectionPosition = stat.words.position;

    int size = nextTaggedSectionPosition - stat.language.end;
    QString temp = stat.text.mid(stat.language.end, size);
    temp=temp.trimmed();
    temp = temp.replace(QRegExp("(^-)|(-$)"), "");
    auto separatorCount = temp.count(" - ");
    if(separatorCount >= 1)
    {
        // two separators means we are have both genre and characters (hopefully)
        // thus, we are splitting what we have on " - "
        // 0 must be genre, 1 must be characters
        QStringList splittings = temp.split(" - ", QString::SkipEmptyParts);
        if(splittings.size() == 0)
            return;

        QStringList genreCandidates = splittings.at(0).split("/");
        bool isGenre = database::IsGenreList(genreCandidates, "ffn");
        if(isGenre)
        {
            ProcessGenres(section, splittings.at(0));
            splittings.pop_front();
        }

        if(splittings.size() == 0)
            return;

        ProcessCharacters(section, splittings.join(" "));
    }
    else{
        // step 3 decide if it has whitespaces before the next " - "
        //   if it has it's characters section
        auto splittings = temp.split(" - ", QString::SkipEmptyParts)        ;
        for(auto& part: splittings)
            part = part.trimmed();
        if(splittings.size() == 0)
            return;
        if(splittings.at(0).contains(" "))
            ProcessCharacters(section, splittings.at(0));
        else
        {
            //   if it has not, it's genre or a singlewords character like Dobby
            //       attempt to split it on "/" and check every token with genre's list in DB
            //       if no match is found its characters

            QStringList genreCandidates = splittings.at(0).split("/");
            bool isGenre = database::IsGenreList(genreCandidates, "ffn");
            if(isGenre)
                ProcessGenres(section, splittings.at(0));
            else
                ProcessCharacters(section, splittings.at(0));
        }
    }
}

void FicParser::DetermineMarkedSubsectionPresence(core::Section & section)
{
    auto& stat = section.statSection;
    stat.rated = GetStatTag(stat.text, "Rated:");
    stat.chapters = GetStatTag(stat.text, "Chapters:");
    stat.favs = GetStatTag(stat.text, "Favs:");
    stat.follows = GetStatTag(stat.text, "Follows:");
    stat.reviews = GetStatTag(stat.text, "Reviews:");
    stat.published = GetStatTag(stat.text, "Published:");
    stat.updated = GetStatTag(stat.text, "Updated:");
    stat.words= GetStatTag(stat.text, "Words:");
    stat.status = GetStatTag(stat.text, "Status:");
    stat.id = GetStatTag(stat.text, "Id:");
}



void FicParser::ClearProcessed()
{
    processedStuff = decltype(processedStuff)();
}

void FicParser::WriteProcessed(QHash<QString, int> & knownFandoms)
{
    auto startRecLoad = std::chrono::high_resolution_clock::now();
    writeSections = database::ProcessFicsIntoUpdateAndInsert({processedStuff}, true);
    auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
    qDebug() << "Filtering done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    auto startInsert = std::chrono::high_resolution_clock::now();
    for(auto& section : writeSections.requiresUpdate)
    {
        database::UpdateInDB(section);
        database::WriteFandomsForStory(section, knownFandoms);
    }
    elapsed = std::chrono::high_resolution_clock::now() - startInsert;
    qDebug() << "Update done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    //database::DropAllFanficIndexes();
    auto startUpdate= std::chrono::high_resolution_clock::now();
    for(auto& section : writeSections.requiresInsert)
    {
        database::InsertIntoDB(section);
        database::WriteFandomsForStory(section, knownFandoms);
    }
    elapsed = std::chrono::high_resolution_clock::now() - startUpdate;
    qDebug() << "Inserts done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

    ClearProcessed();
    elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
    qDebug() << "Write cycle done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
}

void FicParser::WriteJustAuthorName(core::Fic& fic)
{
    fic.author.Log();

    if(fic.author.GetIdStatus() == core::AuthorIdStatus::unassigned)
        fic.author.AssignId(database::GetAuthorIdFromUrl(fic.author.url("ffn")));

    if(fic.author.GetIdStatus() == core::AuthorIdStatus::not_found)
    {
        database::WriteRecommender(fic.author);
        fic.author.AssignId(database::GetAuthorIdFromUrl(fic.author.url("ffn")));
    }
    if(rewriteAuthorName)
        database::AssignNewNameForRecommenderId(fic.author);
}

void FicParser::SetRewriteAuthorNames(bool value)
{
    rewriteAuthorName = value;
}

void FicParser::GetFandom(core::Section & section, int &, QString text)
{
    auto full = GetDoubleNarrow(text,"id=pre_story_links", "</a>\\n", true,
                                "",  "/\">", true,
                                3);
    full = full.replace(" Crossover", "");
    QStringList split = full.split(" + ", QString::SkipEmptyParts);

    section.result.fandom = full;
    section.result.fandoms = split;
    qDebug() << section.result.fandoms;
    qDebug() << section.result.fandom;
}


void FicParser::GetAuthor(core::Section & section, int &startfrom, QString text)
{
    auto full = GetDoubleNarrow(text,"/u/\\d+/", "</a>", true,
                                "",  "'>", false,
                                2);

    QRegExp rxEnd("(/u/\\d+/)(.*)(?='>)");
    rxEnd.setMinimal(true);
    auto index = rxEnd.indexIn(text);
    if(index == -1)
        return;
    section.result.author.SetUrl("ffn",rxEnd.cap(1));
    section.result.author.name = full;
}

void FicParser::GetTitle(core::Section & section, int& startfrom, QString text)
{
    QRegExp rx("Follow/Fav</button><b\\sclass='xcontrast[_]txt'>(.*)</");
    rx.setMinimal(true);
    int indexStart = rx.indexIn(text);
    if(indexStart == -1)
    {
        qDebug() << "failed to get title";
        return;
    }
    section.result.title = rx.cap(1);
    qDebug() << section.result.title;
}

void FicParser::GetStatSection(core::Section &section, int &startfrom, QString text)
{
    auto full = GetSingleNarrow(text,"Rated:", "\\sid:", true);
    qDebug() << full;
    section.statSection.text = full;
}

void FicParser::GetSummary(core::Section & section, int& startfrom, QString text)
{
    auto summary = GetDoubleNarrow(text,
                    "Private\\sMessage", "</div", true,
                    "", "'>", false,
                    2);

    section.result.summary = summary;
    qDebug() << summary;
}

core::Section FicParser::GetSection(QString text, int start)
{
    core::Section section;
    section.start = 0;
    section.end = text.length();
    return section;
}

QString FicParser::ExtractRecommenderNameFromUrl(QString url)
{
    int pos = url.lastIndexOf("/");
    return url.mid(pos+1);
}

