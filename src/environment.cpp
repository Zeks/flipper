#include "include/environment.h"

#include "include/page_utils.h"
#include "include/regex_utils.h"
#include "include/Interfaces/recommendation_lists.h"
#include "include/Interfaces/fandoms.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/authors.h"
#include "include/Interfaces/db_interface.h"
#include "include/Interfaces/genres.h"
#include "include/Interfaces/tags.h"
#include "include/Interfaces/data_source.h"
#include "include/grpc/grpc_source.h"
#include "include/Interfaces/pagetask_interface.h"
#include "include/Interfaces/ffn/ffn_authors.h"
#include "include/Interfaces/ffn/ffn_fanfics.h"
#include "include/db_fixers.h"
#include "include/parsers/ffn/favparser.h"
#include "include/timeutils.h"
#include "include/url_utils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QTextCodec>


void CoreEnvironment::InitMetatypes()
{
    qRegisterMetaType<WebPage>("WebPage");
    qRegisterMetaType<PageResult>("PageResult");
    qRegisterMetaType<ECacheMode>("ECacheMode");
    qRegisterMetaType<FandomParseTask>("FandomParseTask");
    qRegisterMetaType<FandomParseTaskResult>("FandomParseTaskResult");
}

void CoreEnvironment::LoadData()
{
    ficSource->FetchData(filter, &fanfics);
    currentLastFanficId = ficSource->lastFicId;
}

CoreEnvironment::CoreEnvironment(QObject *obj): QObject(obj)
{
    ReadSettings();
}

void CoreEnvironment::ReadSettings()
{

}

void CoreEnvironment::WriteSettings()
{

}
static inline QString CreateConnectString(QString ip,QString port)
{
    QString server_address_proto("%1:%2");
    QString server_address = server_address_proto.arg(ip).arg(port);
    return server_address;
}

void CoreEnvironment::Init()
{
    InitMetatypes();

    QSettings settings("settings.ini", QSettings::IniFormat);

    auto ip = settings.value("Settings/serverIp", "127.0.0.1").toString();
    auto port = settings.value("Settings/serverPort", "3055").toString();

    if(settings.value("Settings/thinClient").toBool())
        ficSource.reset(new FicSourceGRPC(CreateConnectString(ip, port), 160));
    else
        ficSource.reset(new FicSourceDirect(interfaces.db));

    ficSource->AddFicFilter(QSharedPointer<FicFilter>(new FicFilterSlash));

    auto storedRecList = settings.value("Settings/currentList").toString();
    interfaces.recs->SetCurrentRecommendationList(interfaces.recs->GetListIdForName(storedRecList));

    interfaces.recs->LoadAvailableRecommendationLists();
    interfaces.fandoms->FillFandomList(true);
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

    interfaces.pageTask->db  = interfaces.tasks->GetDatabase();
    interfaces.fandoms->Load();
}

int CoreEnvironment::GetResultCount()
{
    return ficSource->GetFicCount(filter);
}
void CoreEnvironment::UseAuthorTask(PageTaskPtr task)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlDatabase tasksDb = QSqlDatabase::database("Tasks");
    AuthorLoadProcessor authorLoader(db,
                                     tasksDb,
                                     interfaces.fanfics,
                                     interfaces.fandoms,
                                     interfaces.authors,
                                     interfaces.pageTask);
    connect(&authorLoader, &AuthorLoadProcessor::requestProgressbar, this, &CoreEnvironment::requestProgressbar);
    connect(&authorLoader, &AuthorLoadProcessor::updateCounter, this, &CoreEnvironment::updateCounter);
    connect(&authorLoader, &AuthorLoadProcessor::updateInfo, this, &CoreEnvironment::updateInfo);
    connect(&authorLoader, &AuthorLoadProcessor::resetEditorText, this, &CoreEnvironment::resetEditorText);
    authorLoader.Run(task);
}

void CoreEnvironment::LoadMoreAuthors(QString listName, ECacheMode cacheMode)
{
    filter.mode = core::StoryFilter::filtering_in_recommendations;
    interfaces.recs->SetCurrentRecommendationList(interfaces.recs->GetListIdForName(listName));
    QStringList authorUrls = interfaces.recs->GetLinkedPagesForList(interfaces.recs->GetCurrentRecommendationList(), "ffn");
    emit requestProgressbar(authorUrls.size());
    emit updateInfo("Authors: " + QString::number(authorUrls.size()));
    QString comment = "Loading more authors from list: " + QString::number(interfaces.recs->GetCurrentRecommendationList());
    auto pageTask = page_utils::CreatePageTaskFromUrls(interfaces.pageTask,
                                                       interfaces.db->GetCurrentDateTime(),
                                                       authorUrls,
                                                       comment,
                                                       500,
                                                       3,
                                                       cacheMode,
                                                       true);

    UseAuthorTask(pageTask);

}


void CoreEnvironment::UpdateAllAuthorsWith(std::function<void(QSharedPointer<core::Author>, WebPage)> updater)
{
    auto authors = interfaces.authors->GetAllAuthors("ffn");
    emit updateInfo("Authors: " + QString::number(authors.size()));
    emit requestProgressbar(authors.size());
    An<PageManager> pager;
    int counter = 0;
    for(auto author: authors)
    {
        auto webPage = pager->GetPage(author->url("ffn"), ECacheMode::use_only_cache);
        //qDebug() << "Page loaded in: " << webPage.LoadedIn();
        emit updateCounter(counter);
        updater(author, webPage);
        counter++;
    }
}


void CoreEnvironment::ReprocessAuthorNamesFromTheirPages()
{
    auto functor = [&](QSharedPointer<core::Author> author, WebPage webPage){
        //auto splittings = SplitJob(webPage.content);
        if(!author || author->id == -1)
            return;
        QString authorName = ParseAuthorNameFromFavouritePage(webPage.content);
        author->name = authorName;
        interfaces.authors->AssignNewNameForAuthor(author, authorName);
    };
    UpdateAllAuthorsWith(functor);
}



void CoreEnvironment::ProcessListIntoRecommendations(QString list)
{
    QFile data(list);
    QSqlDatabase db = QSqlDatabase::database();
    QStringList usedList;
    QList<int> usedIdList;
    if (data.open(QFile::ReadOnly))
    {
        QTextStream in(&data);
        QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
        params->name = in.readLine().split("#").at(1);
        params->minimumMatch= in.readLine().split("#").at(1).toInt();
        params->pickRatio = in.readLine().split("#").at(1).toDouble();
        params->alwaysPickAt = in.readLine().split("#").at(1).toInt();
        interfaces.recs->LoadListIntoDatabase(params);
        database::Transaction transaction(db);
        QString str;
        do{
            str = in.readLine();
            QRegExp rx("/s/(\\d+)");
            int pos = rx.indexIn(str);
            QString ficIdPart;
            if(pos != -1)
            {
                ficIdPart = rx.cap(1);
            }
            if(ficIdPart.isEmpty())
                continue;

            if(ficIdPart.toInt() <= 0)
                continue;
            auto webId = ficIdPart.toInt();

            // at the moment works only for ffn and doesnt try to determine anything else
            auto id = interfaces.fanfics->GetIDFromWebID(webId, "ffn");
            if(id == -1)
                continue;
            qDebug()<< "Settign tag: " << "generictag" << " to: " << id;
            usedList.push_back(str);
            usedIdList.push_back(id);
            interfaces.tags->SetTagForFic(id, "generictag");
        }while(!str.isEmpty());
        params->tagToUse ="generictag";
        BuildRecommendations(params);
        interfaces.tags->DeleteTag("generictag");
        interfaces.recs->SetFicsAsListOrigin(usedIdList, params->id);
        transaction.finalize();
        qDebug() << "using list: " << usedList;
    }
}

int CoreEnvironment::BuildRecommendations(QSharedPointer<core::RecommendationList> params, bool clearAuthors)
{
    QSqlDatabase db = QSqlDatabase::database();
    database::Transaction transaction(db);

    if(clearAuthors)
        interfaces.authors->Clear();
    interfaces.authors->LoadAuthors("ffn");
    interfaces.recs->Clear();
    //fanficsInterface->ClearIndex()
    QList<int> allAuthors = interfaces.authors->GetAllAuthorIds();;
    std::sort(std::begin(allAuthors),std::end(allAuthors));
    qDebug() << "count of author ids: " << allAuthors.size();
    QList<int> filteredAuthors;
    filteredAuthors.reserve(allAuthors.size()/10);

    //QSharedPointer<core::RecommendationList> params
    auto listId = interfaces.recs->GetListIdForName(params->name);
    interfaces.recs->DeleteList(listId);
    interfaces.recs->LoadListIntoDatabase(params);
    int counter = 0;
    int alLCounter = 0;
    int authorCounter = 0;
    for(auto authorId: allAuthors)
    {
        ++authorCounter;
        if(authorCounter%10 == 0)
            QCoreApplication::processEvents();
        auto stats = interfaces.authors->GetStatsForTag(authorId, params);


        if( stats->matchesWithReference >= params->alwaysPickAt
                || (stats->matchRatio <= params->pickRatio && stats->matchesWithReference >= params->minimumMatch) )
        {
            alLCounter++;
            auto author = interfaces.authors->GetById(authorId);
            if(author)
                qDebug() << "Fit for criteria: " << author->name;
            interfaces.recs->LoadAuthorRecommendationsIntoList(authorId, params->id);
            interfaces.recs->LoadAuthorRecommendationStatsIntoDatabase(params->id, stats);
            interfaces.recs->IncrementAllValuesInListMatchingAuthorFavourites(authorId,params->id);
            filteredAuthors.push_back(authorId);
            counter++;
        }
    }

    interfaces.recs->UpdateFicCountInDatabase(params->id);
    interfaces.recs->SetCurrentRecommendationList(params->id);

    transaction.finalize();
    qDebug() << "processed authors: " << counter;
    qDebug() << "all authors: " << alLCounter;
    return params->id;
}

void CoreEnvironment::ResumeUnfinishedTasks()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    if(settings.value("Settings/skipUnfinishedTasksCheck",true).toBool())
        return;
    auto tasks = interfaces.pageTask->GetUnfinishedTasks();
    TaskList tasksToResume;
    for(auto task : tasks)
    {
        QString diagnostics;
        diagnostics+= "Unfinished task:\n";
        diagnostics+= task->taskComment + "\n";
        diagnostics+= "Started: " + task->startedAt.toString("yyyyMMdd hh:mm") + "\n";
        Log(diagnostics);
        tasksToResume.push_back(task);
    }

    // later this needs to be preceded by code that routes tasks based on their type.
    // hard coding for now to make sure base functionality works
    {
        for(auto task : tasksToResume)
        {
            auto fullTask = interfaces.pageTask->GetTaskById(task->id);
            if(fullTask->type == 0)
                UseAuthorTask(fullTask);
            else
            {
                if(!task)
                    return;

                QSqlDatabase db = QSqlDatabase::database();
                FandomLoadProcessor proc(db, interfaces.fanfics, interfaces.fandoms, interfaces.pageTask, interfaces.db);

                proc.Run(fullTask);

                thread_local QString status = "%1:%2\n";
                QString diagnostics;
                diagnostics+= "Finished the job:\n";
                diagnostics+= status.arg("Inserted fics").arg(fullTask->addedFics);
                diagnostics+= status.arg("Updated fics").arg(fullTask->updatedFics);
                diagnostics+= status.arg("Duplicate fics").arg(fullTask->skippedFics);
            }
        }
    }
}

void CoreEnvironment::CreateSimilarListForGivenFic(int id, QSqlDatabase db)
{
    static bool authorsLoaded = false;
    database::Transaction transaction(db);
    QSharedPointer<core::RecommendationList> params = core::RecommendationList::NewRecList();
    params->alwaysPickAt = 1;
    params->minimumMatch = 1;
    params->name = "similar";
    params->tagToUse = "generictag";
    params->pickRatio = 9999999999;
    interfaces.tags->SetTagForFic(id, "generictag");
    BuildRecommendations(params, !authorsLoaded);
    interfaces.tags->DeleteTag("generictag");
    interfaces.recs->SetFicsAsListOrigin({id}, params->id);
    transaction.finalize();
}

core::AuthorPtr CoreEnvironment::LoadAuthor(QString url, QSqlDatabase db)
{
    WebPage page;
    TimedAction fetchAction("Author page fetch", [&](){
        page = env::RequestPage(url.trimmed(),  ECacheMode::dont_use_cache);
    });
    fetchAction.run(false);
    emit resetEditorText();
    FavouriteStoryParser parser(interfaces.fanfics);
    QString name = ParseAuthorNameFromFavouritePage(page.content);
    parser.authorName = name;
    parser.ProcessPage(page.url, page.content);
    emit updateInfo(parser.diagnostics.join(""));
    database::Transaction transaction(db);
    QSet<QString> fandoms;
    interfaces.authors->EnsureId(parser.recommender.author); // assuming ffn
    auto author =interfaces.authors->GetByWebID("ffn", url_utils::GetWebId(url, "ffn").toInt());
    parser.authorStats->id = author->id;
    FavouriteStoryParser::MergeStats(author,interfaces.fandoms, {parser});

    interfaces.authors->UpdateAuthorRecord(author);
    {
        interfaces.fanfics->ProcessIntoDataQueues(parser.processedStuff);
        fandoms =interfaces.fandoms->EnsureFandoms(parser.processedStuff);
        QList<core::FicRecommendation> recommendations;
        recommendations.reserve(parser.processedStuff.size());
        for(auto& section : parser.processedStuff)
            recommendations.push_back({section, author});
        interfaces.fanfics->AddRecommendations(recommendations);
        auto result =interfaces.fanfics->FlushDataQueues();
        Q_UNUSED(result);
        interfaces.fandoms->RecalculateFandomStats(fandoms.values());
    }
    transaction.finalize();
    return author;
}

PageTaskPtr CoreEnvironment::LoadTrackedFandoms(ForcedFandomUpdateDate forcedDate,
                                                ECacheMode cacheMode,
                                                QString wordCutoff)
{
    interfaces.fanfics->ClearProcessedHash();
    auto fandoms = interfaces.fandoms->ListOfTrackedFandoms();
    qDebug() << "Tracked fandoms: " << fandoms;
    emit resetEditorText();

    QStringList nameList;
    for(auto fandom : fandoms)
    {
        if(!fandom)
            continue;
        nameList.push_back(fandom->GetName());
    }

    emit updateInfo(QString("Starting load of tracked %1 fandoms <br>").arg(fandoms.size()));
    auto result = ProcessFandomsAsTask(fandoms,
                             "",
                             true,
                             cacheMode,
                             wordCutoff,
                             forcedDate);
    return result;
}

void CoreEnvironment::UseFandomTask(PageTaskPtr task)
{
    QSqlDatabase db = QSqlDatabase::database();
    FandomLoadProcessor proc(db, interfaces.fanfics, interfaces.fandoms, interfaces.pageTask, interfaces.db);

    connect(&proc, &FandomLoadProcessor::requestProgressbar, this, &CoreEnvironment::requestProgressbar);
    connect(&proc, &FandomLoadProcessor::updateCounter, this, &CoreEnvironment::updateCounter);
    connect(&proc, &FandomLoadProcessor::updateInfo, this, &CoreEnvironment::updateInfo);
    proc.Run(task);
}

//fixes the crossover url and selects between 60 and 100k words to add to search params
static QString CreatePrototypeWithSearchParams(QString cutoffText)
{
    QString lastPart = "?&srt=1&lan=1&r=10&len=%1";
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    int lengthCutoff = cutoffText == "100k Words" ? 100 : 60;
    lastPart=lastPart.arg(lengthCutoff);
    QString resultString = "https://www.fanfiction.net%1" + lastPart;
    qDebug() << resultString;
    return resultString;
}

PageTaskPtr CoreEnvironment::ProcessFandomsAsTask(QList<core::FandomPtr> fandoms,
                                                  QString taskComment,
                                                  bool allowCacheRefresh,
                                                  ECacheMode cacheMode,
                                                  QString cutoffText,
                                                  ForcedFandomUpdateDate forcedDate)
{
    interfaces.fanfics->ClearProcessedHash();
    QSqlDatabase db = QSqlDatabase::database();
    FandomLoadProcessor proc(db, interfaces.fanfics, interfaces.fandoms, interfaces.pageTask, interfaces.db);

    connect(&proc, &FandomLoadProcessor::requestProgressbar, this, &CoreEnvironment::requestProgressbar);
    connect(&proc, &FandomLoadProcessor::updateCounter, this, &CoreEnvironment::updateCounter);
    connect(&proc, &FandomLoadProcessor::updateInfo, this, &CoreEnvironment::updateInfo);

    auto result = proc.CreatePageTaskFromFandoms(fandoms,
                                                 CreatePrototypeWithSearchParams(cutoffText),
                                                 taskComment,
                                                 cacheMode,
                                                 allowCacheRefresh,
                                                 forcedDate);

    proc.Run(result);
    return result;
}


void CoreEnvironment::Log(QString value)
{
    qDebug() << value;
}


namespace env {
WebPage RequestPage(QString pageUrl, ECacheMode cacheMode, bool autoSaveToDB)
{
    WebPage result;
    An<PageManager> pager;
    pager->SetDatabase(QSqlDatabase::database());
    result = pager->GetPage(pageUrl, cacheMode);
    if(autoSaveToDB)
        pager->SavePageToDB(result);
    return result;
}
}

void UserInterface::EnsureUUIDForUserTbale(QSqlDatabase db)
{

}
