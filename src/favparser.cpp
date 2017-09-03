#include "include/favparser.h"
#include "include/section.h"
#include "include/init_database.h"
#include <QDebug>
#include <QSqlDatabase>
#include <chrono>

QList<Fic> FavouriteStoryParser::ProcessPage(QString url, QString& str, int authorWave)
{
    Section section;
    int currentPosition = 0;
    int counter = 0;
    QList<Fic> sections;
    //recommender.name = ExtractRecommdenderNameFromUrl(url);
    recommender.name = authorName;
    recommender.url = url;
    recommender.wave = authorWave;
    recommender.website = "ffn";
    while(true)
    {

        counter++;
        section = GetSection(str, currentPosition);
        if(!section.isValid)
            break;
        currentPosition = section.start;


        //section.fandom = ui->rbNormal->isChecked() ? currentFandom: currentFandom + " CROSSOVER";
        GetTitle(section, currentPosition, str);
        GetUrl(section, currentPosition, str);
        GetAuthorUrl(section, currentPosition, str);
        GetAuthor(section, currentPosition, str);
        GetSummary(section, currentPosition, str);

        GetStatSection(section, currentPosition, str);

        GetTaggedSection(section.statSection.replace(",", ""), "(.*)(?=\\s-\\sRated:)", [&section](QString val){ section.result.fandom = val;});
        GetTaggedSection(section.statSection.replace(",", ""), "Words:\\s(\\d{1,8})", [&section](QString val){ section.result.wordCount = val;});
        GetTaggedSection(section.statSection.replace(",", ""), "Chapters:\\s(\\d{1,5})", [&section](QString val){ section.result.chapters = val;});
        GetTaggedSection(section.statSection.replace(",", ""), "Reviews:\\s(\\d{1,5})", [&section](QString val){ section.result.reviews = val;});
        GetTaggedSection(section.statSection.replace(",", ""), "Favs:\\s(\\d{1,5})", [&section](QString val){ section.result.favourites = val;});
        GetTaggedSection(section.statSection, "Published:\\s<span\\sdata-xutime='(\\d+)'", [&section](QString val){
            if(val != "not found")
                section.result.published.setTime_t(val.toInt()); ;
        });
        GetTaggedSection(section.statSection, "Updated:\\s<span\\sdata-xutime='(\\d+)'", [&section](QString val){
            if(val != "not found")
                section.result.updated.setTime_t(val.toInt());
            else
                section.result.updated.setTime_t(0);
        });
        GetTaggedSection(section.statSection, "Rated:\\s(.{1})", [&section](QString val){ section.result.rated = val;});
        GetTaggedSection(section.statSection, "English\\s-\\s([A-Za-z/\\-]+)\\s-\\sChapters", [&section](QString val){ section.result.genre = val;});
        GetTaggedSection(section.statSection, "</span>\\s-\\s([A-Za-z\\.\\s/]+)$", [&section](QString val){
            section.result.characters = val.replace(" - Complete", "");
        });
        GetTaggedSection(section.statSection, "(Complete)$", [&section](QString val){
            if(val != "not found")
                section.result.complete = 1;
        });
        if(section.statSection.contains("CROSSOVER", Qt::CaseInsensitive))
            GetCrossoverFandomList(section, currentPosition, str);
        section.result.origin = url;
        if(section.isValid)
        {
            sections.append(section.result);
        }

    }

    if(sections.size() == 0)
    {
        diagnostics.push_back("<span> No data found on the page.<br></span>");
        diagnostics.push_back("<span> \nFinished loading data <br></span>");
    }

    qDebug() << "Processed fic, count:  " << sections.count();
    processedStuff+=sections;
    currentPosition = 999;
    return sections;
}

QString FavouriteStoryParser::GetFandom(QString text)
{
    QRegExp rxStart(QRegExp::escape("xicon-section-arrow'></span>"));
    QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text, 0);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    return text.mid(indexStart + 28,indexEnd - (indexStart + 28));
}

void FavouriteStoryParser::ClearProcessed()
{
    processedStuff.clear();
    recommender = FavouritesPage();
}

void FavouriteStoryParser::ClearDoneCache()
{
    alreadyDone.clear();
}

void FavouriteStoryParser::WriteProcessed()
{
    auto startRecLoad = std::chrono::high_resolution_clock::now();
    writeSections = database::ProcessFicsIntoUpdateAndInsert(processedStuff);
    auto elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
    qDebug() << "Filtering done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    auto startInsert = std::chrono::high_resolution_clock::now();
    for(auto& section : writeSections.requiresUpdate)
    {
        if(!alreadyDone.contains(section.url("ffn")))
            database::UpdateInDB(section);
    }
    elapsed = std::chrono::high_resolution_clock::now() - startInsert;
    qDebug() << "Update done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    //database::DropAllFanficIndexes();
    auto startUpdate= std::chrono::high_resolution_clock::now();
    for(auto& section : writeSections.requiresInsert)
    {
        if(!alreadyDone.contains(section.url("ffn")))
            database::InsertIntoDB(section);
    }
    elapsed = std::chrono::high_resolution_clock::now() - startUpdate;
    qDebug() << "Inserts done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    auto startRecommending = std::chrono::high_resolution_clock::now();
    //database::RebuildAllFanficIndexes();
    for(auto& section : processedStuff)
    {
        int fic_id = database::GetFicIdByWebId(section.webId);
        database::WriteRecommendation(recommender.author, fic_id);
    }
    elapsed = std::chrono::high_resolution_clock::now() - startRecommending;
    qDebug() << "Recommendations done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    ClearProcessed();
    elapsed = std::chrono::high_resolution_clock::now() - startRecLoad;
    qDebug() << "Write cycle done in: " << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
}

void FavouriteStoryParser::WriteJustAuthorName()
{
    if(recommender.GetIdStatus() == AuthorIdStatus::unassigned)
        recommender.AssignId(database::GetAuthorIdFromUrl(recommender.url));
    if(recommender.GetIdStatus() == AuthorIdStatus::not_found)
    {
        database::WriteRecommender(recommender);
        recommender.AssignId(database::GetAuthorIdFromUrl(recommender.url));
    }
    database::AssignNewNameForRecommenderId(recommender);
}

void FavouriteStoryParser::WriteRecommenderInfo()
{
    //database::WriteRecommendersMetainfo(recommender);
}

void FavouriteStoryParser::SetCurrentTag(QString value)
{
    currentTagMode = value;
}

void FavouriteStoryParser::GetAuthorUrl(Section & section, int &startfrom, QString text)
{
    // looking for first href
    //QString currentSection = text.mid(startfrom);
    QRegExp rxStart(QRegExp::escape("href=\""));
    QRegExp rxEnd(QRegExp::escape("\">"));
    int indexStart = rxStart.indexIn(text,startfrom);
//    if(indexStart == -1)
//        qDebug() << currentSection;
    int indexEnd = rxEnd.indexIn(text, indexStart);
    section.result.author.SetUrl("ffn","https://www.fanfiction.net" + text.mid(indexStart + 6,indexEnd - (indexStart + 6)));
    startfrom = indexEnd+2;
}

void FavouriteStoryParser::GetAuthor(Section & section, int& startfrom, QString text)
{
    //QString currentStart = text.mid(startfrom);
    //QRegExp rxBy("\">");
    //QRegExp rxStart("</a>");
    QRegExp rxEnd(QRegExp::escape("</a>"));
    //int indexBy = rxBy.indexIn(text, startfrom);
    int indexStart = startfrom;
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.result.author.name = text.mid(indexStart,indexEnd - (indexStart));

}



void FavouriteStoryParser::GetTitle(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape("data-title=\""));
    QRegExp rxEnd(QRegExp::escape("\""));
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart+13);
    startfrom = indexEnd;
    section.result.title = text.mid(indexStart + 12,indexEnd - (indexStart + 12));
    section.result.title=section.result.title.replace("\\'","'");
    section.result.title=section.result.title.replace("\'","'");
}

void FavouriteStoryParser::GetStatSection(Section &section, int &startfrom, QString text)
{
    QRegExp rxStart("padtop2\\sxgray");
    QRegExp rxEnd("</div></div></div>");
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    section.statSection= text.mid(indexStart + 15,indexEnd - (indexStart + 15));
    section.statSectionStart = indexStart + 15;
    section.statSectionEnd = indexEnd;
    //qDebug() << section.statSection;
}

void FavouriteStoryParser::GetSummary(Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape("padtop'>"));
    QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);

    section.result.summary = text.mid(indexStart + 8,indexEnd - (indexStart + 8));
    section.summaryEnd = indexEnd;
    startfrom = indexEnd;
}

void FavouriteStoryParser::GetCrossoverFandomList(Section & section, int &startfrom, QString text)
{
    QRegExp rxStart("Crossover\\s-\\s");
    QRegExp rxEnd("\\s-\\sRated:");


    int indexStart = rxStart.indexIn(text, startfrom);
    if(indexStart != -1 )
    {
        section.result.fandom.replace("Crossover - ", "");
        section.result.fandom += " CROSSOVER";
    }

    int indexEnd = rxEnd.indexIn(text, indexStart + 1);

    section.result.fandom = text.mid(indexStart + (rxStart.pattern().length() -2), indexEnd - (indexStart + rxStart.pattern().length() - 2)).trimmed() + QString(" CROSSOVER");
    startfrom = indexEnd;
}

void FavouriteStoryParser::GetUrl(Section & section, int& startfrom, QString text)
{
    // looking for first href
    QRegExp rxStart(QRegExp::escape("href=\""));
    QRegExp rxEnd(QRegExp::escape("\">"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int tempStart = indexStart;
    int indexEnd = rxEnd.indexIn(text, indexStart);
    int tempEnd = indexEnd;
    //QString tempUrl = text.mid(indexStart + 6,indexEnd - (indexStart + 6));
    indexStart = rxStart.indexIn(text,indexStart+1);
    indexEnd = rxEnd.indexIn(text, indexStart);
    QString secondUrl = text.mid(indexStart + 6,indexEnd - (indexStart + 6));
    if(!secondUrl.contains("/s/"))
    {
        indexStart = tempStart;
        indexEnd  = tempEnd;
    }
    QString tempUrl = text.mid(indexStart + 6,indexEnd - (indexStart + 6));
    if(tempUrl.length() > 200)
    {
        QString currentSection = text.mid(startfrom);
        //qDebug() << currentSection;
        //qDebug() << currentSection;
    }
    section.result.SetUrl("ffn",text.mid(indexStart + 6,indexEnd - (indexStart + 6)));
    QRegExp rxWebId("/s/(\\d+)");
    auto indexWeb = rxWebId.indexIn(section.result.url("ffn"));
    if(indexWeb != -1)
    {
        QString captured = rxWebId.cap(1);
        section.result.webId = captured.toInt();
        section.result.SetUrl("ffn", section.result.url("ffn").left(3 + captured.length()));
    }
    startfrom = indexEnd+2;
}



void FavouriteStoryParser::GetTaggedSection(QString text, QString rxString ,std::function<void (QString)> functor)
{
    QRegExp rx(rxString);
    int indexStart = rx.indexIn(text);
    //qDebug() << rx.capturedTexts();
    if(indexStart != 1 && !rx.cap(1).trimmed().replace("-","").isEmpty())
        functor(rx.cap(1));
    else
        functor("not found");
}


Section FavouriteStoryParser::GetSection(QString text, int start)
{
    Section section;
    QRegExp rxStart("<div\\sclass=\'z-list\\sfavstories\'");

    int index = rxStart.indexIn(text, start);
    if(index != -1)
    {
        section.isValid = true;
        section.start = index;
        int end = rxStart.indexIn(text, index+1);
        if(end == -1)
            end = index + 2000;
        section.end = end;
    }
    return section;
}

QString FavouriteStoryParser::ExtractRecommdenderNameFromUrl(QString url)
{
    int pos = url.lastIndexOf("/");
    return url.mid(pos+1);
}
