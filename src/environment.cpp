#include "include/environment.h"
#include "include/tasks/fandom_task_processor.h"
#include "include/page_utils.h"
#include "include/regex_utils.h"
#include "include/Interfaces/recommendation_lists.h"
#include "include/Interfaces/fandoms.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/authors.h"
#include "include/Interfaces/db_interface.h"
#include "include/Interfaces/genres.h"
#include "include/Interfaces/tags.h"
#include "include/Interfaces/pagetask_interface.h"
#include "include/Interfaces/ffn/ffn_authors.h"
#include "include/Interfaces/ffn/ffn_fanfics.h"
#include "include/db_fixers.h"
#include <QSqlQuery>
#include <QSqlError>


void CoreEnvironment::InitMetatypes()
{
    qRegisterMetaType<WebPage>("WebPage");
    qRegisterMetaType<PageResult>("PageResult");
    qRegisterMetaType<ECacheMode>("ECacheMode");
    qRegisterMetaType<FandomParseTask>("FandomParseTask");
    qRegisterMetaType<FandomParseTaskResult>("FandomParseTaskResult");
}

QSqlQuery CoreEnvironment::BuildQuery(bool countOnly)
{
    QSqlDatabase db = QSqlDatabase::database();
    if(countOnly)
        currentQuery = countQueryBuilder.Build(filter);
    else
        currentQuery = queryBuilder.Build(filter);
    QSqlQuery q(db);
    q.prepare(currentQuery->str);
    auto it = currentQuery->bindings.begin();
    auto end = currentQuery->bindings.end();
    while(it != end)
    {
        if(currentQuery->str.contains(it.key()))
        {
            qDebug() << it.key() << " " << it.value();
            q.bindValue(it.key(), it.value());
        }
        ++it;
    }
    return q;
}

void CoreEnvironment::LoadData(SlashFilterState slashFilter)
{
    auto q = BuildQuery();
    q.setForwardOnly(true);
    q.exec();
    qDebug().noquote() << q.lastQuery();
    if(q.lastError().isValid())
    {
        qDebug() << "Error loading data:" << q.lastError();
        qDebug().noquote() << q.lastQuery();
    }
    int counter = 0;
    fanfics.clear();
    currentLastFanficId = -1;
    auto rx = GetSlashRegex();
    CommonRegex regexToken;
    regexToken.Init();
    while(q.next())
    {
        counter++;
        bool allow = true;
        auto fic = LoadFanfic(q);
        QRegularExpression slashRx(rx, QRegularExpression::CaseInsensitiveOption);
        SlashPresence slashToken;
        if(slashFilter.invertedEnabled || slashFilter.slashOnlyEnabled)
            slashToken = regexToken.ContainsSlash(fic.summary, fic.charactersFull, fic.fandom);

        if(slashFilter.applyLocalEnabled && slashFilter.invertedLocalEnabled)
        {
            if(slashToken.IsSlash())
                allow = false;
        }
        if(slashFilter.applyLocalEnabled && slashFilter.slashOnlyLocalEnabled)
        {
            if(!slashToken.IsSlash())
                allow = false;
        }
        if(allow)
            fanfics.push_back(fic);
        if(counter%10000 == 0)
            qDebug() << "tick " << counter/1000;
        //qDebug() << "tick " << counter;
    }
    if(fanfics.size() > 0)
        currentLastFanficId = fanfics.last().id;

    qDebug() << "loaded fics:" << counter;
}

void CoreEnvironment::Init()
{
    InitMetatypes();

    std::unique_ptr<core::DefaultRNGgenerator> rng (new core::DefaultRNGgenerator());
    rng->portableDBInterface = interfaces.db;
    queryBuilder.SetIdRNGgenerator(rng.release());

    QSettings settings("settings.ini", QSettings::IniFormat);
    auto storedRecList = settings.value("Settings/currentList").toString();
    interfaces.recs->SetCurrentRecommendationList(interfaces.recs->GetListIdForName(storedRecList));

    interfaces.recs->LoadAvailableRecommendationLists();
    interfaces.fandoms->FillFandomList(true);
}



inline core::Fic CoreEnvironment::LoadFanfic(QSqlQuery& q)
{
    core::Fic result;
    result.id = q.value("ID").toInt();
    result.fandom = q.value("FANDOM").toString();
    result.author = core::Author::NewAuthor();
    result.author->name = q.value("AUTHOR").toString();
    result.title = q.value("TITLE").toString();
    result.summary = q.value("SUMMARY").toString();
    result.genreString = q.value("GENRES").toString();
    result.charactersFull = q.value("CHARACTERS").toString().replace("not found", "");
    result.rated = q.value("RATED").toString();
    auto published = q.value("PUBLISHED").toDateTime();
    auto updated   = q.value("UPDATED").toDateTime();
    result.published = published;
    result.updated= updated;
    result.SetUrl("ffn",q.value("URL").toString());
    result.tags = q.value("TAGS").toString();
    result.wordCount = q.value("WORDCOUNT").toString();
    result.favourites = q.value("FAVOURITES").toString();
    result.reviews = q.value("REVIEWS").toString();
    result.chapters = QString::number(q.value("CHAPTERS").toInt() + 1);
    result.complete= q.value("COMPLETE").toInt();
    result.atChapter = q.value("AT_CHAPTER").toInt();
    result.recommendations= q.value("SUMRECS").toInt();
    return result;
}
void CoreEnvironment::InitInterfaces()
{
    interfaces.authors = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    interfaces.fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    interfaces.recs   = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    interfaces.fandoms = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    interfaces.tags   = QSharedPointer<interfaces::Tags> (new interfaces::Tags());
    interfaces.genres  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    interfaces.pageTask= QSharedPointer<interfaces::PageTask> (new interfaces::PageTask());

    // probably need to change this to db accessor
    // to ensure db availability for later

    interfaces.authors->portableDBInterface = interfaces.db;
    interfaces.fanfics->authorInterface = interfaces.authors;
    interfaces.fanfics->fandomInterface = interfaces.fandoms;
    interfaces.recs->portableDBInterface = interfaces.db;
    interfaces.recs->authorInterface = interfaces.authors;
    interfaces.fandoms->portableDBInterface = interfaces.db;
    interfaces.tags->fandomInterface = interfaces.fandoms;

    //bool isOpen = interfaces.db.tags->GetDatabase().isOpen();
    interfaces.authors->db = interfaces.db->GetDatabase();
    interfaces.fanfics->db = interfaces.db->GetDatabase();
    interfaces.recs->db    = interfaces.db->GetDatabase();
    interfaces.fandoms->db = interfaces.db->GetDatabase();
    interfaces.tags->db    = interfaces.db->GetDatabase();
    interfaces.genres->db  = interfaces.db->GetDatabase();
    queryBuilder.portableDBInterface = interfaces.db;
    countQueryBuilder.portableDBInterface = interfaces.db;
    interfaces.pageTask->db  = interfaces.tasks->GetDatabase();
    interfaces.fandoms->Load();
}

WebPage CoreEnvironment::RequestPage(QString pageUrl, ECacheMode cacheMode, bool autoSaveToDB)
{
    WebPage result;
    An<PageManager> pager;
    pager->SetDatabase(QSqlDatabase::database());
    result = pager->GetPage(pageUrl, cacheMode);
    if(autoSaveToDB)
        pager->SavePageToDB(result);
    return result;
}
int CoreEnvironment::GetResultCount()
{
    auto q = BuildQuery(true);
    q.setForwardOnly(true);
    if(!database::puresql::ExecAndCheck(q))
        return -1;
    q.next();
    auto result =  q.value("records").toInt();
    return result;
}
