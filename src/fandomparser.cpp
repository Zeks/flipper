#include "fandomparser.h"\



void FandomParser::ProcessPage(WebPage page)
{
    processedStuff.clear();
    minSectionUpdateDate = QDateTime::currentDateTimeUtc();
    QString& str = page.content;
    core::Section section;
    int currentPosition = 0;
    int counter = 0;
    QList<core::Section> sections;

    while(true)
    {

        counter++;
        section = GetSection(str, currentPosition);
        if(!section.isValid)
            break;
        currentPosition = section.start;

        section.result.fandom = page.crossover ? page.fandom + " CROSSOVER" : page.fandom;
        GetUrl(section, currentPosition, str);
        GetTitle(section, currentPosition, str);
        GetAuthor(section, currentPosition, str);
        GetSummary(section, currentPosition, str);

        GetStatSection(section, currentPosition, str);

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

        if(section.result.fandom.contains("CROSSOVER"))
            GetCrossoverFandomList(section, currentPosition, str);


        if(section.isValid)
        {
            if((section.result.updated < minSectionUpdateDate) && (section.result.updated.date().year() > 1990))
                minSectionUpdateDate = section.result.updated;
            section.result.origin = page.url;
            processedStuff.append(section.result);
        }

    }

    if(sections.size() > 0)
        nextUrl = GetNext(sections.last(), currentPosition, str);
}

QString FandomParser::GetFandom(QString text)
{
    QRegExp rxStart(QRegExp::escape("xicon-section-arrow'></span>"));
    QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text, 0);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    return text.mid(indexStart + 28,indexEnd - (indexStart + 28));
}

void FandomParser::GetAuthor(core::Section & section, int& startfrom, QString text)
{
    QRegExp rxBy("by\\s<");
    QRegExp rxStart(">");
    QRegExp rxEnd(QRegExp::escape("</a>"));
    int indexBy = rxBy.indexIn(text, startfrom);
    int indexStart = rxStart.indexIn(text, indexBy + 3);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.result.author.name = text.mid(indexStart + 1,indexEnd - (indexStart + 1));

}

void FandomParser::GetTitle(core::Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape(">"));
    QRegExp rxEnd(QRegExp::escape("</a>"));
    int indexStart = rxStart.indexIn(text, startfrom + 1);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    startfrom = indexEnd;
    section.result.title = text.mid(indexStart + 1,indexEnd - (indexStart + 1));
    qDebug() << section.result.title;
}

void FandomParser::GetStatSection(core::Section &section, int &startfrom, QString text)
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

void FandomParser::GetSummary(core::Section & section, int& startfrom, QString text)
{
    QRegExp rxStart(QRegExp::escape("padtop'>"));
    QRegExp rxEnd(QRegExp::escape("<div"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);

    section.result.summary = text.mid(indexStart + 8,indexEnd - (indexStart + 8));
    section.summaryEnd = indexEnd;
    startfrom = indexEnd;
}

void FandomParser::GetCrossoverFandomList(core::Section & section, int &startfrom, QString text)
{
    QRegExp rxStart("Crossover\\s-\\s");
    QRegExp rxEnd("\\s-\\sRated:");

    int indexStart = rxStart.indexIn(text, startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart + 1);

    QString tmp = text.mid(indexStart + (rxStart.pattern().length() -2), indexEnd - (indexStart + rxStart.pattern().length() - 2)).trimmed();
    section.result.fandom = tmp + QString(" CROSSOVER");
    section.result.fandoms = tmp.split("&");
    startfrom = indexEnd;
}

void FandomParser::GetUrl(core::Section & section, int& startfrom, QString text)
{
    // looking for first href
    QRegExp rxStart(QRegExp::escape("href=\""));
    QRegExp rxEnd(QRegExp::escape("\"><img"));
    int indexStart = rxStart.indexIn(text,startfrom);
    int indexEnd = rxEnd.indexIn(text, indexStart);
    section.result.SetUrl("ffn",text.mid(indexStart + 6,indexEnd - (indexStart + 6)));
    startfrom = indexEnd+2;
}

QString FandomParser::GetNext(core::Section & section, int &startfrom, QString text)
{
    QString nextUrl;
    QRegExp rxEnd(QRegExp::escape("Next &#187"));
    int indexEnd = rxEnd.indexIn(text, startfrom);
    int posHref = indexEnd - 400 + text.midRef(indexEnd - 400,400).lastIndexOf("href='");
    //int indexStart = rxStart.indexIn(text,section.start);
    //indexStart = rxStart.indexIn(text,indexStart+1);
    //indexStart = rxStart.indexIn(text,indexStart+1);
    nextUrl = CreateURL(text.mid(posHref+6, indexEnd - (posHref+6)));
    if(!nextUrl.contains("&p="))
        nextUrl = "";
    indexEnd = rxEnd.indexIn(text, startfrom);
    return nextUrl;
}

void FandomParser::GetTaggedSection(QString text, QString rxString ,std::function<void (QString)> functor)
{
    QRegExp rx(rxString);
    int indexStart = rx.indexIn(text);
    //qDebug() << rx.capturedTexts();
    if(indexStart != 1 && !rx.cap(1).trimmed().replace("-","").isEmpty())
        functor(rx.cap(1));
    else
        functor("not found");
}

QString FandomParser::GetLast(QString pageContent)
{
    QString lastUrl;
    QRegExp rxEnd(QRegExp::escape("Last</a"));
    int indexEnd = rxEnd.indexIn(pageContent);
    indexEnd-=2;
    int posHref = indexEnd - 400 + pageContent.midRef(indexEnd - 400,400).lastIndexOf("href='");
    lastUrl = CreateURL(pageContent.mid(posHref+6, indexEnd - (posHref+6)));
    if(!lastUrl.contains("&p="))
        lastUrl = "";
    indexEnd = rxEnd.indexIn(pageContent);
    return lastUrl;
}


core::Section FandomParser::GetSection(QString text, int start)
{
    core::Section section;
    QRegExp rxStart("<div\\sclass=\'z-list\\szhover\\szpointer\\s\'");
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
QString FandomParser::CreateURL(QString str)
{
    return "https://www.fanfiction.net/" + str;
}
