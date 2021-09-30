/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

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
#include "include/environment.h"
#include "include/ui/tagwidget.h"
#include "include/page_utils.h"
#include "include/regex_utils.h"
#include "include/Interfaces/recommendation_lists.h"
#include "include/Interfaces/fandoms.h"
#include "include/Interfaces/fandom_lists.h"
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
#include "include/parsers/ffn/desktop_favparser.h"
#include "include/tasks/author_cache_reprocessor.h"
#include "include/in_tag_accessor.h"
#include "include/timeutils.h"
#include "include/in_tag_accessor.h"

#ifdef USE_WEBVIEW
#include "include/webview/pagegetter_w.h"
#endif

#include "include/url_utils.h"
#include "include/Interfaces/interface_sqlite.h"
#include "sql_abstractions/sql_query.h"
#include <QSqlError>
#include <QTextCodec>
#include <QSettings>
#include <QMessageBox>
#include <QFile>
#include <QLineEdit>
#include <QLabel>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QStandardPaths>


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
    ficScores= interfaces.fanfics->GetScoresForFics();

    TimedAction action("Full data load",[&](){
        auto snoozeInfo = interfaces.fanfics->GetUserSnoozeInfo();

        QVector<core::Fanfic> newFanfics;
        interfaces::TagIDFetcherSettings tagFetcherSettings;
        tagFetcherSettings.allowSnoozed = filter.displaySnoozedFics;

        if(filter.exactFicIds.size() == 1)
        {
            FicSourceGRPC* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());

            grpcSource->FetchFic(filter.exactFicIds.at(0).id,
                                 &newFanfics);
        }
        else {
            UserData userData;
            userData.allTaggedFics = interfaces.tags->GetAllTaggedFics(tagFetcherSettings);
            // need to add non expired snoozes to tags if snooze show mode isn't selected
            for(const auto& snoozedFic : snoozeInfo){
                if(!snoozedFic.expired)
                    userData.allSnoozedFics.insert(snoozedFic.ficId);
                else
                    continue;
            }

            auto sources = interfaces.recs->GetAllSourceFicIDs(filter.listForRecommendations);
            if(filter.activeTags.size() > 0)
            {
                tagFetcherSettings.tags = filter.activeTags;
                tagFetcherSettings.useAND = filter.tagsAreANDed;
                userData.ficIDsForActivetags = interfaces.tags->GetFicsTaggedWith(tagFetcherSettings);
                if(userData.ficIDsForActivetags.size() == 0)
                {
                    QMessageBox::warning(nullptr, "Warning!", "There are no fics tagged with selected tag(s)\nAborting search.");
                    return;
                }

            }
            else
            {
                if(filter.showRecSources == core::StoryFilter::ssm_show)
                {
                    for(auto fic : sources)
                        userData.allTaggedFics.remove(fic);
                }
                else if(filter.showRecSources == core::StoryFilter::ssm_hide)
                {
                    for(auto fic : sources)
                        userData.allTaggedFics.insert(fic);

                }
            }

            userData.ignoredFandoms = interfaces.fandoms->GetIgnoredFandomsIDs();
            userData.fandomStates = filter.fandomStates;
            //userData.likedAuthors = likedAuthors;
            ficSource->userData = userData;

            core::StoryFilter::EScoreType scoreType = filter.sortMode != core::StoryFilter::sm_minimize_dislikes ? core::StoryFilter::st_points : core::StoryFilter::st_minimal_dislikes;

            //QVector<int> recFics;
            core::ReclistFilter reclistFilter;
            reclistFilter.mainListId = interfaces.recs->GetCurrentRecommendationList();
            reclistFilter.minMatchCount = filter.minRecommendations;
            reclistFilter.limiter = filter.sourcesLimiter;
            reclistFilter.scoreType = scoreType;
            reclistFilter.displayPurged = filter.displayPurgedFics;


            filter.recommendationScoresSearchToken = interfaces.recs->GetAllFicsHash(reclistFilter);
            if(filter.showRecSources == core::StoryFilter::ssm_hide)
            {
                for(auto fic : sources){
                    filter.recommendationScoresSearchToken.ficToScore.erase(fic);
                    filter.recommendationScoresSearchToken.ficToPureVotes.erase(fic);
                }

            }
            filter.scoresHash = ficScores;

            ficSource->FetchData(filter,
                                 &newFanfics);
        }
        interfaces.fandoms->FetchFandomsForFics(&newFanfics);
        interfaces.fanfics->FetchSnoozesForFics(&newFanfics);
        interfaces.fanfics->FetchNotesForFics(&newFanfics);
        interfaces.fanfics->FetchChaptersForFics(&newFanfics);
        interfaces.tags->FetchTagsForFics(&newFanfics);
        interfaces.recs->FetchRecommendationsBreakdown(&newFanfics, filter.listForRecommendations);
        for(auto& fic : newFanfics)
        {
            // actual assignment of purged param happens in LoadNewScoreValuesForFanfics
            if(fic.author_id > 1 && likedAuthors.contains(fic.author_id))
                fic.userData.likedAuthor = true;
            auto ficId = fic.GetIdInDatabase();
            if(ficScores.contains(ficId))
                fic.score = ficScores[ficId];
            if(snoozeInfo.contains(ficId))
            {
                auto info = snoozeInfo[ficId];
                fic.userData.snoozeExpired = info.expired;
                fic.userData.chapterTillSnoozed = info.snoozedTillChapter;
                if(info.snoozedTillChapter - info.snoozedAtChapter == 1)
                    fic.userData.snoozeMode = core::Fanfic::efsm_next_chapter;
                else if(info.untilFinished)
                    fic.userData.snoozeMode = core::Fanfic::efsm_til_finished;
                else
                    fic.userData.snoozeMode = core::Fanfic::efsm_target_chapter;
                fic.userData.chapterSnoozed = info.snoozedAtChapter;
                fic.userData.ficIsSnoozed = true;
            }
        }
        fanfics = newFanfics;
        currentLastFanficId = ficSource->lastFicId;

    });
    action.run();
}

//struct FilterFrame{
//    core::StoryFilter filter; // an intermediary to keep UI filter data to be passed into query builder

//    int sizeOfCurrentQuery = 0; // "would be" size of the used search query if LIMIT  was not applied
//    int pageOfCurrentQuery = 0; // current page that the used search query is at
//    int currentLastFanficId = -1;

//    QVector<core::Fic> fanfics; // filtered fanfic data

//    QSharedPointer<core::Query> currentQuery; // the last query created by query builder. reused when querying subsequent pages
//};

void CoreEnvironment::LoadHistoryFrame(FilterFrame frame)
{
    filter = frame.filter;
    fanfics = frame.fanfics;
    sizeOfCurrentQuery = frame.sizeOfCurrentQuery;
    pageOfCurrentQuery = frame.pageOfCurrentQuery;
    currentLastFanficId = frame.currentLastFanficId;
    currentQuery = frame.currentQuery;
}

CoreEnvironment::CoreEnvironment(QObject *obj): QObject(obj), searchHistory(250)
{
    ReadSettings();
    rngGenerator.reset(new core::DefaultRNGgenerator);
    interfaces.userDb.reset (new database::SqliteInterface());
    interfaces.tasks.reset (new database::SqliteInterface());
    interfaces.pageCache.reset (new database::SqliteInterface());
    interfaces.db.reset (new database::SqliteInterface());
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
    QString server_address = server_address_proto.arg(ip,port);
    return server_address;
}

bool CoreEnvironment::Init()
{
    InitMetatypes();

    QSettings settings("settings/settings.ini", QSettings::IniFormat);
    QSettings uiSettings("settings/ui.ini", QSettings::IniFormat);

    auto ip = settings.value("Settings/serverIp", "127.0.0.1").toString();
    auto port = settings.value("Settings/serverPort", "3055").toString();


    //interfaces.db->userToken
    interfaces.userDb->userToken = interfaces.userDb->GetUserToken();
    ficSource.reset(new FicSourceGRPC(CreateConnectString(ip, port), interfaces.userDb->userToken,  160));
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    status = grpcSource->GetStatus();
    if(!status.isValid)
    {
        QString statusString = QString("The server is not responding%1");
        if(!status.error.isEmpty())
            statusString=statusString.arg("\nReason: " + status.error);
        statusString+= "\nYou could try accessing it later or ping the maintainer at ficflipper@gmail.com";
        QMessageBox::critical(nullptr, "Warning!", statusString);
        return true;
    }
    if(status.protocolVersionMismatch)
    {
        QString statusString = QString("<html>Your client version is out of date.<br>Some (or all) features may not work.<br>Please get updated version <a href=https://github.com/Zeks/flipper/releases/latest>Here</a></html>");
        QMessageBox::critical(nullptr, "Warning!", statusString);
        return true;
    }
    if(status.messageRequired)
       QMessageBox::information(nullptr, "Attention!", status.motd);

    QVector<core::Fandom> fandoms;
    grpcSource->GetFandomListFromServer(interfaces.fandoms->GetLastFandomID(), &fandoms);
    if(fandoms.size() > 0)
        interfaces.fandoms->UploadFandomsIntoDatabase(fandoms);
    interfaces.fandoms->Load();
    interfaces.fandoms->FillFandomList(true);
    interfaces.fandoms->ReloadRecentFandoms();
    interfaces.fandomLists->ProcessIgnoreListIntoFandomList();
    interfaces.fandomLists->LoadFandomLists();
    auto recentFandoms = interfaces.fandoms->GetRecentFandoms();
    if(recentFandoms.size() == 0)
    {
        if(!interfaces.fandoms.isNull())
        {
            interfaces.fandoms->PushFandomToTopOfRecent("Naruto");
            interfaces.fandoms->PushFandomToTopOfRecent("RWBY");
            interfaces.fandoms->PushFandomToTopOfRecent("Worm");
            interfaces.fandoms->PushFandomToTopOfRecent("High School DxD");
            interfaces.fandoms->PushFandomToTopOfRecent("Harry Potter");
        }
    }

    auto result = interfaces.tags->ReadUserTags();

    ficSource->AddFicFilter(QSharedPointer<FicFilter>(new FicFilterSlash));

    auto storedRecList = uiSettings.value("Settings/currentList").toString();
    interfaces.recs->SetCurrentRecommendationList(interfaces.recs->GetListIdForName(storedRecList));
    //interfaces.fandoms->Load();
    interfaces.recs->LoadAvailableRecommendationLists();
    //interfaces.fandoms->FillFandomList(true);

    auto fics = interfaces.fanfics->GetFicIDsWithUnsetAuthors();
    if(fics.size() > 0)
    {
        auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
        if(grpcSource)
        {
            auto authors = grpcSource->GetAuthorsForFicList(fics);
            interfaces.fanfics->WriteAuthorsForFics(authors);
        }
    }

    auto positiveTags = settings.value("Tags/minorPositive").toString().split(",", Qt::SkipEmptyParts);
    positiveTags += settings.value("Tags/majorPositive").toString().split(",", Qt::SkipEmptyParts);
    likedAuthors = interfaces.tags->GetAuthorsForTags(positiveTags);
    RefreshSnoozes();

    return true;
}

void CoreEnvironment::InitInterfaces()
{
    QSettings settings("settings/settings.ini", QSettings::IniFormat);
    QSharedPointer<database::IDBWrapper> userDBInterface;
    userDBInterface = interfaces.userDb;


    interfaces.authors = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    interfaces.fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    interfaces.recs   = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    interfaces.fandoms = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
    interfaces.fandomLists = std::shared_ptr<interfaces::FandomLists> (new interfaces::FandomLists());
    interfaces.tags   = QSharedPointer<interfaces::Tags> (new interfaces::Tags());
    interfaces.genres  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    interfaces.pageTask= QSharedPointer<interfaces::PageTask> (new interfaces::PageTask());

    // probably need to change this to db accessor
    // to ensure db availability for later

    interfaces.authors->portableDBInterface = userDBInterface;
    interfaces.fanfics->authorInterface = interfaces.authors;
    interfaces.fanfics->fandomInterface = interfaces.fandoms;
    interfaces.recs->portableDBInterface = userDBInterface;
    interfaces.recs->authorInterface = interfaces.authors;
    interfaces.fandoms->portableDBInterface = userDBInterface;
    interfaces.tags->fandomInterface = interfaces.fandoms;

    //bool isOpen = interfaces.db.tags->GetDatabase().isOpen();
    interfaces.authors->db = userDBInterface->GetDatabase();
    interfaces.fanfics->db = userDBInterface->GetDatabase();
    interfaces.recs->db    = userDBInterface->GetDatabase();
    interfaces.fandoms->db = userDBInterface->GetDatabase();
    interfaces.fandomLists->db = userDBInterface->GetDatabase();
    interfaces.fandoms->isClient = true;
    interfaces.tags->db    = userDBInterface->GetDatabase();
    interfaces.genres->db  = userDBInterface->GetDatabase();

    interfaces.pageTask->db  = interfaces.tasks->GetDatabase();

}

void CoreEnvironment::InstantiateClientDatabases(QString folder)
{
    interfaces.userDb->InitAndUpdateDatabaseForFile(folder, "UserDB", QDir::currentPath() + "/dbcode/user_db_init.sql", "", true);
    interfaces.tasks->InitAndUpdateDatabaseForFile(folder, "Tasks", QDir::currentPath()  + "/dbcode/tasksinit.sql", "Tasks", false);
    interfaces.pageCache->InitAndUpdateDatabaseForFile(folder, "PageCache", QDir::currentPath()  + "/dbcode/pagecacheinit.sql", "PageCache", false);
}

int CoreEnvironment::GetResultCount()
{
    QSettings settings("settings/settings.ini", QSettings::IniFormat);

    UserData userData;

    interfaces::TagIDFetcherSettings tagFetcherSettings;
    tagFetcherSettings.allowSnoozed = filter.displaySnoozedFics;
    userData.allTaggedFics = interfaces.tags->GetAllTaggedFics(tagFetcherSettings);

    auto snoozeInfo = interfaces.fanfics->GetUserSnoozeInfo();
    for(const auto& snoozedFic : snoozeInfo){
        if(!snoozedFic.expired)
            userData.allSnoozedFics.insert(snoozedFic.ficId);
    }

    if(filter.activeTags.size() > 0)
    {
        tagFetcherSettings.tags = filter.activeTags;
        tagFetcherSettings.useAND = filter.tagsAreANDed;
        userData.ficIDsForActivetags = interfaces.tags->GetFicsTaggedWith(tagFetcherSettings);
    }
    userData.ignoredFandoms = interfaces.fandoms->GetIgnoredFandomsIDs();
    userData.fandomStates = filter.fandomStates;
    ficSource->userData = userData;

    //QVector<int> recFics;
    //filter.recsHash = interfaces.recs->GetAllFicsHash(interfaces.recs->GetCurrentRecommendationList(), filter.minRecommendations);
    core::StoryFilter::EScoreType scoreType = filter.sortMode != core::StoryFilter::sm_minimize_dislikes ? core::StoryFilter::st_points : core::StoryFilter::st_minimal_dislikes;

    core::ReclistFilter reclistFilter;
    reclistFilter.mainListId = interfaces.recs->GetCurrentRecommendationList();
    reclistFilter.minMatchCount = filter.minRecommendations;
    reclistFilter.limiter = filter.sourcesLimiter;
    reclistFilter.displayPurged = filter.displayPurgedFics;
    reclistFilter.scoreType = scoreType;
    filter.recommendationScoresSearchToken = interfaces.recs->GetAllFicsHash(reclistFilter);

    ficScores= interfaces.fanfics->GetScoresForFics();
    filter.scoresHash = ficScores;
    return ficSource->GetFicCount(filter);
}
void CoreEnvironment::UseAuthorTask(PageTaskPtr task)
{
    sql::Database db = sql::Database::database();
    sql::Database tasksDb = sql::Database::database("Tasks");
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
    authorLoader.dbInterface = interfaces.db;
    authorLoader.Run(task);
}

void CoreEnvironment::LoadMoreAuthors(QString listName, fetching::CacheStrategy cacheStrategy)
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
                                                       cacheStrategy,
                                                       true);

    UseAuthorTask(pageTask);
}

void CoreEnvironment::LoadAllLinkedAuthors(fetching::CacheStrategy cacheStrategy)
{
    filter.mode = core::StoryFilter::filtering_in_recommendations;
    QStringList authorUrls;

    auto db = sql::Database::database();
    auto authorInterface = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authorInterface->db = db;
    authorInterface->portableDBInterface = interfaces.db;

    auto authors = authorInterface->GetAllUnprocessedLinkedAuthors();
    for(auto author : authors)
    {
        authorUrls.push_back("https://www.fanfiction.net/u/" + QString::number(author));
    }

    emit requestProgressbar(authorUrls.size());
    emit updateInfo("Authors: " + QString::number(authorUrls.size()));
    QString comment = "Loading all unprocessed authors";
    auto pageTask = page_utils::CreatePageTaskFromUrls(interfaces.pageTask,
                                                       interfaces.db->GetCurrentDateTime(),
                                                       authorUrls,
                                                       comment,
                                                       500,
                                                       3,
                                                       cacheStrategy,
                                                       true);

    UseAuthorTask(pageTask);
}

void CoreEnvironment::LoadAllLinkedAuthorsMultiFromCache()
{
    sql::Database db = sql::Database::database();
    AuthorCacheReprocessor reprocessor(db,
                                       interfaces.fanfics,
                                       interfaces.fandoms,
                                       interfaces.authors);

    reprocessor.ReprocessAllAuthorsStats();
}





void CoreEnvironment::UpdateAllAuthorsWith(std::function<void(QSharedPointer<core::Author>, WebPage)> updater)
{
    auto authors = interfaces.authors->GetAllAuthors("ffn");
    emit updateInfo("Authors: " + QString::number(authors.size()));
    emit requestProgressbar(authors.size());
    An<PageManager> pager;
    int counter = 0;
    for(const auto& author: authors)
    {
        auto webPage = pager->GetPage(author->url("ffn"), fetching::CacheStrategy::CacheOnly());
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
    sql::Database db = sql::Database::database();
    QStringList usedList;
    QVector<int> usedIdList;
    if (data.open(QFile::ReadOnly))
    {
        QTextStream in(&data);
        QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
        params->name = in.readLine().split("#").at(1);
        params->minimumMatch= in.readLine().split("#").at(1).toInt();
        params->maxUnmatchedPerMatch = in.readLine().split("#").at(1).toDouble();
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
            qDebug() << "Settign tag: " << "generictag" << " to: " << id;
            usedList.push_back(str);
            usedIdList.push_back(id);
            interfaces.tags->SetTagForFic(id, "generictag");
        }while(!str.isEmpty());
        params->tagToUse ="generictag";
        BuildRecommendations(params, usedIdList);
        interfaces.tags->DeleteTag("generictag");
        interfaces.recs->SetFicsAsListOrigin(usedIdList, params->id);
        transaction.finalize();
        qDebug() << "using list: " << usedList;
    }
}

QVector<int> CoreEnvironment::GetSourceFicsFromFile(QString filename)
{
    //QFile data("lists/source.txt");
    QFile data(filename);
    QVector<int> sourceList;
    if (data.open(QFile::ReadOnly))
    {
        QTextStream in(&data);
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
            sourceList.push_back(webId);
        }while(!str.isEmpty());
    }
    return sourceList;
}

int CoreEnvironment::BuildRecommendationsServerFetch(QSharedPointer<core::RecommendationList> params,
                                                     QVector<int> sourceFics)
{

    qDebug() << "At start list id is: " << params->id;
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());


    params->ficData->fics = sourceFics;

    database::Transaction transaction(interfaces.recs->db);
    params->id = interfaces.recs->GetListIdForName(params->name);
    qDebug() << "Fetched name for list is: " << params->name;

    QVector<core::Identity> pack;
    pack.resize(sourceFics.size());
    int i = 0;
    for(auto source: std::as_const(sourceFics))
    {
        pack[i].web.ffn = source;
        i++;
    }
    grpcSource->GetInternalIDsForFics(&pack);

    params->likedAuthors = likedAuthors;
    params->ficData->taggedFics = interfaces.tags->GetAllTaggedFics();
    //QLOG_INFO() << "Logging list params: ";
    params->Log();
    bool result = grpcSource->GetRecommendationListFromServer(params);


    //    QSet<int> sourceSet;
    //    sourceSet.reserve(sourceFics.size());
    for(const auto& identity: std::as_const(pack))
        params->ficData->sourceFics.insert(identity.id);


    if(!result)
    {
        QLOG_ERROR() << "list creation failed";
        return -1;
    }
    if(!params->success || params->ficData->fics.size() == 0)
    {
        return -1;
    }
    TimedAction action("List write: ",[&](){



        qDebug() << "Deleting list: " << params->id;
        interfaces.recs->DeleteList(params->id);

        interfaces.recs->LoadListIntoDatabase(params);
        //qDebug() << list->ficData->fics;
        interfaces.recs->LoadListFromServerIntoDatabase(params);

        interfaces.recs->UpdateFicCountInDatabase(params->id);
        interfaces.recs->SetCurrentRecommendationList(params->id);
        emit resetEditorText();
        auto keys = params->ficData->matchReport.keys();
        std::sort(keys.begin(), keys.end());
        emit updateInfo( QString("Matches: ") + QString("Times Found:") + "<br>");
        for(auto key: keys)
            emit updateInfo(QString::number(key).leftJustified(11, ' ').replace(" ", "&nbsp;")
                            + " " + QString::number(params->ficData->matchReport[key]) + "<br>");
        if(params->assignLikedToSources)
        {
            QLOG_INFO() << "autolike is enabled";
            for(auto fic: std::as_const(params->ficData->sourceFics))
            {
                //QLOG_INFO() << "liking fic: "  << fic;
                if(fic > 0)
                    interfaces.tags->SetTagForFic(fic, "Liked");
            }
        }
        auto fics = interfaces.fanfics->GetFicIDsWithUnsetAuthors();
        auto authors = grpcSource->GetAuthorsForFicList(fics);
        interfaces.fanfics->WriteAuthorsForFics(authors);

        QSettings settings("settings/settings.ini", QSettings::IniFormat);
        auto positiveTags = settings.value("Tags/minorPositive").toString().split(",", Qt::SkipEmptyParts);
        positiveTags += settings.value("Tags/majorPositive").toString().split(",", Qt::SkipEmptyParts);
        likedAuthors = interfaces.tags->GetAuthorsForTags(positiveTags);
    });
    action.run();
    transaction.finalize();
    return params->id;
}

core::FavListDetails CoreEnvironment::GetStatsForFicList(QVector<int> sourceFics)
{
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    QVector<core::Identity> pack;
    pack.resize(sourceFics.size());
    int i = 0;
    for(auto source: sourceFics)
    {
        pack[i].web.ffn = source;
        i++;
    }
    auto result = grpcSource->GetStatsForFicList(pack);
    if(result.has_value())
        return *result;
    return core::FavListDetails();
}

int CoreEnvironment::BuildRecommendationsLocalVersion(QSharedPointer<core::RecommendationList> params, bool clearAuthors)
{
    sql::Database db = sql::Database::database();
    database::Transaction transaction(db);

    if(clearAuthors)
        interfaces.authors->Clear();
    interfaces.authors->LoadAuthors("ffn");
    interfaces.recs->Clear();
    //fanficsInterface->ClearIndex()
    QList<int> allAuthors = interfaces.authors->GetAllAuthorIds();
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
    for(auto authorId: std::as_const(allAuthors))
    {
        ++authorCounter;
        if(authorCounter%10 == 0)
            QCoreApplication::processEvents();
        auto stats = interfaces.authors->GetStatsForTag(authorId, params);


        if( stats->matchesWithReference >= params->alwaysPickAt
                || (stats->matchRatio <= params->maxUnmatchedPerMatch && stats->matchesWithReference >= params->minimumMatch) )
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


int CoreEnvironment::BuildRecommendations(QSharedPointer<core::RecommendationList> params,
                                          QVector<int> sourceFics)
{
    return BuildRecommendationsServerFetch(params,sourceFics);
}

int CoreEnvironment::BuildDiagnosticsForRecList(QSharedPointer<core::RecommendationList> list,
                                                QVector<int> sourceFics)
{
    list->id = interfaces.recs->GetListIdForName(list->name);
    qDebug() << "At start list id is: " << list->id;
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    list->ficData->fics = sourceFics;

    database::Transaction transaction(interfaces.recs->db);
    list->id = interfaces.recs->GetListIdForName(list->name);
    qDebug() << "Fetched name for list is: " << list->name;

    QVector<core::Identity> pack;
    pack.resize(sourceFics.size());
    int i = 0;
    for(auto source: std::as_const(sourceFics))
    {
        pack[i].web.ffn = source;
        i++;
    }
    grpcSource->GetInternalIDsForFics(&pack);

    list->likedAuthors = likedAuthors;
    auto result = grpcSource->GetDiagnosticsForRecListFromServer(list);
    list->quadraticDeviation = result.quad;
    list->ratioMedian = result.ratioMedian;
    list->sigma2Distance = result.sigma2Dist;
    list->hasAuxDataFilled = true;




    if(!result.isValid)
    {
        QLOG_ERROR() << "diagnostics creation failed";
        return -1;
    }

    TimedAction action("Diagnostics write: ",[&](){
        qDebug() << "Deleting list: " << list->id;
        interfaces.recs->WriteAuthorStatsForRecList(list->id, result.authorData);
        interfaces.recs->WriteFicRecommenderRelationsForRecList(list->id, result.authorsForFics);
        interfaces.recs->LoadListAuxDataIntoDatabase(list);
        interfaces.recs->SetCurrentRecommendationList(list->id);
    });
    action.run();
    transaction.finalize();
    qDebug() << "finished building diagnostics for reclist";
    return list->id;

}

bool CoreEnvironment::ResumeUnfinishedTasks()
{
    QSettings settings("settings/settings.ini", QSettings::IniFormat);
    if(settings.value("Settings/skipUnfinishedTasksCheck",true).toBool())
        return false;
    auto tasks = interfaces.pageTask->GetUnfinishedTasks();
    TaskList tasksToResume;
    tasksToResume.reserve(tasks.size());
    for(const auto& task : std::as_const(tasks))
    {
        QString diagnostics;
        diagnostics+= "Unfinished task:\n";
        diagnostics+= task->taskComment + "\n";
        diagnostics+= "Started: " + task->startedAt.toString("yyyyMMdd hh:mm") + "\n";
        Log(diagnostics);
        tasksToResume.push_back(task);
    }
    if(!tasksToResume.size())
        return false;

    // later this needs to be preceded by code that routes tasks based on their type.
    // hard coding for now to make sure base functionality works
    {
        for(const auto& task : tasksToResume)
        {
            auto fullTask = interfaces.pageTask->GetTaskById(task->id);
            if(fullTask->type == 0)
            {
                UseAuthorTask(fullTask);
            }
            else
            {
                if(!task)
                    return false;

                sql::Database db = sql::Database::database();
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
    return true;
}

void CoreEnvironment::CreateSimilarListForGivenFic(int id, sql::Database db)
{
    database::Transaction transaction(db);
    QSharedPointer<core::RecommendationList> params = core::RecommendationList::NewRecList();
    params->alwaysPickAt = 1;
    params->minimumMatch = 1;
    params->name = "similar";
    params->tagToUse = "generictag";
    interfaces.tags->SetTagForFic(id, "generictag");
    BuildRecommendations(params, {id});
    interfaces.tags->DeleteTag("generictag");
    interfaces.recs->SetFicsAsListOrigin({id}, params->id);
    transaction.finalize();
}

QVector<int> CoreEnvironment::GetListSourceFFNIds(int listId)
{
    QVector<int> result;
    auto sources = interfaces.recs->GetAllSourceFicIDs(listId);
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    QVector<core::Identity> pack;
    pack.resize(sources.size());
    result.reserve(sources.size());
    int i = 0;
    for(auto source: sources)
    {
        pack[i].id = source;
        i++;
    }
    grpcSource->GetFFNIDsForFics(&pack);
    for(const auto& identity : std::as_const(pack))
        result.push_back(identity.web.ffn);


    return result;
}

QVector<int> CoreEnvironment::GetFFNIds(QSet<int> sources)
{
    QVector<int> result;
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    QVector<core::Identity> pack;
    pack.resize(sources.size());
    result.reserve(sources.size());
    int i = 0;
    for(auto source: sources)
    {
        pack[i].id = source;
        i++;
    }
    grpcSource->GetFFNIDsForFics(&pack);
    for(const auto& id : std::as_const(pack))
        result.push_back(id.web.ffn);

    return result;
}

core::AuthorPtr CoreEnvironment::LoadAuthor(QString url, sql::Database db)
{
    WebPage page;
    TimedAction fetchAction("Author page fetch", [&](){
        page = env::RequestPage(url.trimmed(),  fetching::CacheStrategy::NetworkOnly());
    });
    fetchAction.run(false);
    emit resetEditorText();
    FavouriteStoryParser parser;
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
        Q_UNUSED(result)
        interfaces.fandoms->RecalculateFandomStats(fandoms.values());
    }
    transaction.finalize();
    return author;
}

QSet<QString>  CoreEnvironment::LoadAuthorFicIdsForRecCreation(QString url, QLabel* infoTarget, bool silent)
{
    auto result = LoadAuthorFics(url);
    QSet<QString> urlResult;
    for(const auto& fic : result)
    {
        if(fic->ficSource != core::Fanfic::efs_own_works)
            urlResult.insert(QString::number(fic->identity.web.ffn));
    }

    if(result.size() < 500)
        return urlResult;

    QString infoString = "Due to fanfiction.net still not fixing their favourites page displaying only 500 entries.\n"
                         "Your profile will have to be parsed from m.fanfiction.net page.\n";
   QString coffeePart = "Please grab a cup of coffee or smth after you press OK to close this window and wait a bit.";

    QString templateString = "<font color=\"darkBlue\">Status: %1</font>";
    if(!silent)
    {
        if(!infoTarget)
            QMessageBox::information(nullptr, "Attention!", infoString + coffeePart);
        else
        {
            coffeePart = "Please grab a cup of coffee or smth and wait a bit.";
            infoTarget->setText(templateString.arg(infoString + coffeePart));
            QCoreApplication::processEvents();
        }
    }

    // first we need to create an m. link
    url = url.remove("https://www.");
    url = url.prepend("https://m.");
    url = url.append("/?a=fs");
    QString prototype = url;
    // we need to grab the initial page and figure out the exact amount of pages we need to parse

    WebPage page;
    TimedAction fetchAction("Author initial mobile page fetch", [&](){
        page = env::RequestPage(prototype.trimmed(),  fetching::CacheStrategy::NetworkOnly());
    });
    fetchAction.run(false);

    //QString content;
    QRegularExpression rx("p[=](\\d+)['][>]Last[<][/]a[>]");
    page.content = page.content.replace("s\" s", "s' s");
    page.content = page.content.replace("=\"", "='");
    page.content = page.content.replace("\">", "'>");
    page.content = page.content.replace("\\'", "'");
    page.content = page.content.replace(";\"", ";'");
    page.content = page.content.replace("ago<", "<");
    page.content = page.content.replace("&amp;", "&");
    qDebug() << "got page text: " <<  page.content;
    auto match = rx.match(page.content);
    if(!match.hasMatch())
    {
        // failed to grab the last record, exiting with info from just basic page
        return urlResult;
    }
    int amountOfPagesToGrab = match.captured(1).toInt();

    // generating all of the urls that will need to be grabbed
    QStringList mobileUrls;
    // 26th page onwards can't be reached without m.
    const int startOfUnreachablePart = 26;
    mobileUrls.reserve(amountOfPagesToGrab);
    for(int i = startOfUnreachablePart; i <= amountOfPagesToGrab; i++)
        mobileUrls.push_back(prototype + "&s=0&cid=0&p=" + QString::number(i));

    //QList<QString> mobileStories;
    QRegularExpression rxStoryId("/s/(\\d+)/1");
    emit requestProgressbar(amountOfPagesToGrab - 25);
    int counter = 1;
    for(const auto& mobileUrl : mobileUrls)
    {
        WebPage page;
        TimedAction fetchAction("Author mobile page fetch", [&](){
            page = env::RequestPage(mobileUrl.trimmed(),  fetching::CacheStrategy::NetworkOnly());
        });
        fetchAction.run(false);
        // need to fetch only story ids for now
        // this should be enough to create the rec list

        QRegularExpressionMatchIterator iterator = rxStoryId.globalMatch(page.content);
        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();
            QString word = match.captured(1);
            urlResult << word;
        }
        counter++;
        emit updateCounter(counter);
        if(page.source != EPageSource::cache)
            QThread::msleep(1000);
        QCoreApplication::processEvents();
    }
    return urlResult;
}

bool CoreEnvironment::TestAuthorID(QString id)
{
    QString url = "https://www.fanfiction.net/u/" + id;
    WebPage page;
    TimedAction fetchAction("Author page fetch", [&](){
        page = env::RequestPage(url.trimmed(),  fetching::CacheStrategy::CacheThenFetchIfNA());
    });
    fetchAction.run(false);
    if(!page.content.contains("Anime"))
        return false;
    return true;
}
//QRegularExpression rx("https://www.fanfiction.net/u/(\\d)+");
//auto match = rx.match(url);
//if(!match.hasMatch())
//{
//    QMessageBox::warning(nullptr, "Warning!", "URL is not an FFN author url\nNeeeds to be a https://www.fanfiction.net/u/NUMERIC_ID");
//    return result;
//}
bool CoreEnvironment::TestAuthorID(QLineEdit * input, QLabel * lblStatus)
{
    auto userID = input->text();
    //https://www.fanfiction.net/u/4507073/
    QRegularExpression rx("^(https://www.fanfiction.net/u/){0,1}(\\d+)/{0,1}");
    auto match = rx.match(userID);
    if(!match.hasMatch())
    {
        lblStatus->setVisible(true);
        lblStatus->setText("<font color=\"Red\">Provided user ID is not a valid number.</font>");
        return false;
    }
    userID=match.captured(2);
    input->setText(userID);
    auto validUser = TestAuthorID(userID);
    if(!validUser){
        lblStatus->setVisible(true);
        lblStatus->setText("<font color=\"Red\">Provided user ID is not a valid FFN user.</font>");
        return false;
    }
    lblStatus->setVisible(true);
    lblStatus->setText("<font color=\"darkGreen\">Valid FFN user.</font>");
    return true;
}

QList<QSharedPointer<core::Fanfic> > CoreEnvironment::LoadAuthorFics(QString url)
{
    QStringList result;
    WebPage page;
    TimedAction fetchAction("Author page fetch", [&](){
        page = env::RequestPage(url.trimmed(),  fetching::CacheStrategy::NetworkOnly());
    });
    fetchAction.run(false);
    emit resetEditorText();
    FavouriteStoryParser parser;
    QString name = ParseAuthorNameFromFavouritePage(page.content);
    parser.authorName = name;
    parser.ProcessPage(page.url, page.content);
    emit updateInfo(parser.diagnostics.join(""));

    return parser.processedStuff;
}

PageTaskPtr CoreEnvironment::LoadTrackedFandoms(ForcedFandomUpdateDate forcedDate,
                                                fetching::CacheStrategy cacheStrategy,
                                                QString wordCutoff)
{
    interfaces.fanfics->ClearProcessedHash();
    auto fandoms = interfaces.fandoms->ListOfTrackedFandoms();
    //qDebug()  << "Tracked fandoms: " << fandoms;
    emit resetEditorText();

    QStringList nameList;
    for(const auto& fandom : fandoms)
    {
        if(!fandom)
            continue;
        nameList.push_back(fandom->GetName());
    }

    emit updateInfo(QString("Starting load of tracked %1 fandoms <br>").arg(fandoms.size()));
    auto result = ProcessFandomsAsTask(fandoms,
                                       "",
                                       true,
                                       cacheStrategy,
                                       wordCutoff,
                                       forcedDate);
    return result;
}

void CoreEnvironment::FillDBIDsForTags()
{
    database::Transaction transaction(interfaces.tags->db);
    auto pack = interfaces.tags->GetAllFicsThatDontHaveDBID();
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    grpcSource->GetInternalIDsForFics(&pack);
    interfaces.tags->FillDBIDsForFics(pack);
    transaction.finalize();
}

QList<int> CoreEnvironment::GetDBIDsForFics(QVector<int> ids)
{
    QVector<core::Identity> pack;
    pack.resize(ids.size());
    int i = 0;
    for(auto source: ids)
    {
        pack[i].web.ffn = source;
        i++;
    }

    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    grpcSource->GetInternalIDsForFics(&pack);
    QList<int> result;
    result.reserve(pack.size());
    for(const auto& source: std::as_const(pack))
    {
        result.push_back(source.id);
    }
    return result;
}

QSet<int> CoreEnvironment::GetAuthorsContainingFicFromRecList(int fic, QString recList)
{
    auto currentList = interfaces.recs->GetListIdForName(recList);
    auto authors = interfaces.recs->GetAuthorsForRecommendationListClient(currentList);
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    auto resultingAuthors = grpcSource->GetAuthorsForFicInRecList(fic, authors);
    return resultingAuthors;
}

QSet<int> CoreEnvironment::GetFicsForTags(QStringList tags)
{
    interfaces::TagIDFetcherSettings tagFetcherSettings;
    tagFetcherSettings.tags = tags;
    tagFetcherSettings.allowSnoozed = true;

    auto fics  = interfaces.tags->GetFicsTaggedWith(tagFetcherSettings);
    return fics;
}

QSet<int> CoreEnvironment::GetFicsForNegativeTags()
{
    QSettings settings("settings/settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    QStringList negativeTags = settings.value("Tags/majorNegative", "").toString().split(",");
    auto majorNegativeFics = GetFicsForTags(negativeTags);
    return majorNegativeFics;
}

QSet<int> CoreEnvironment::GetIgnoredDeadFics()
{
    return GetFicsForTags({"Limbo"});
}

void CoreEnvironment::LoadNewScoreValuesForFanfics(core::ReclistFilter filter, QVector<core::Fanfic>& fanfics)
{
    interfaces.recs->FetchRecommendationsBreakdown(&fanfics, filter.mainListId);
    for(auto& fic : fanfics)
    {
        if(!filter.displayPurged && fic.recommendationsData.purged)
            fic.recommendationsData.purged = false;
    }
    if(fanfics.size() <= 100){
        interfaces.recs->LoadPlaceAndRecommendationsData(&fanfics, filter);
    }
}

void CoreEnvironment::BackupUserDatabase()
{
    QSettings uiSettings("settings/ui.ini", QSettings::IniFormat);
    uiSettings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    qDebug() << uiSettings.value("Settings/dbPath").toString();
    //QDir dbDir (uiSettings.value("Settings/dbPath").toString());
    QDir backupDir (QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups");
    backupDir.mkpath(backupDir.path());

    QString backupFileName = "UserDB_" + QDateTime::currentDateTime().toString("yyMMdd") ;

    QString backupFullFile = backupDir.path() + "/" + backupFileName + ".sqlite";
    if(QFileInfo::exists(backupFullFile))
        return;

    interfaces.backupDb.reset (new database::SqliteInterface());
    auto backupDb = interfaces.backupDb->InitAndUpdateDatabaseForFile(backupDir.path(), backupFileName, QDir::currentPath() + "/dbcode/user_db_init.sql", backupFileName, false);
    database::Transaction transaction(backupDb);
    interfaces.userDb->PassScoresToAnotherDatabase(backupDb);
    interfaces.userDb->PassTagSetToAnotherDatabase(backupDb);
    interfaces.userDb->PassFicTagsToAnotherDatabase(backupDb);
    interfaces.userDb->PassSnoozesToAnotherDatabase(backupDb);
    interfaces.userDb->PassFicNotesToAnotherDatabase(backupDb);
    interfaces.userDb->PassClientDataToAnotherDatabase(backupDb);
    interfaces.userDb->PassRecentFandomsToAnotherDatabase(backupDb);
    interfaces.userDb->PassReadingDataToAnotherDatabase(backupDb);
    interfaces.userDb->PassIgnoredFandomsToAnotherDatabase(backupDb);
    interfaces.userDb->PassFandomListSetToAnotherDatabase(backupDb);
    interfaces.userDb->PassFandomListDataToAnotherDatabase(backupDb);
    transaction.finalize();
}

int CoreEnvironment::CreateDefaultRecommendationsForCurrentUser()
{
    auto user = interfaces.recs->GetUserProfile();
    if(user < 1)
        return -1;

    QString url = "https://www.fanfiction.net/u/" + QString::number(interfaces.recs->GetUserProfile());
    auto sourceFicsSet = LoadAuthorFicIdsForRecCreation(url, nullptr, true);

    QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
    params->minimumMatch = 1;
    params->alwaysPickAt = 9999;
    params->isAutomatic = true;
    params->useWeighting = true;
    params->useMoodAdjustment = true;
    params->name = "Recommendations";
    params->userFFNId = interfaces.recs->GetUserProfile();
    QVector<int> sourceFics;
    sourceFics.reserve(sourceFicsSet.size());
    for(const auto& fic : sourceFicsSet)
        sourceFics.push_back(fic.toInt());

    auto ids = interfaces.fandoms->GetIgnoredFandomsIDs();

    for(auto i = ids.cbegin(); i != ids.cend(); i++)
        params->ignoredFandoms.insert(i.key());

    auto result = BuildRecommendations(params, sourceFics);
    return result;
}

void CoreEnvironment::RefreshSnoozes()
{
    auto snoozeInfo = interfaces.fanfics->GetUserSnoozeInfo(false);
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    auto expiredSnoozes = grpcSource->GetExpiredSnoozes(snoozeInfo);
    interfaces.fanfics->WriteExpiredSnoozes(expiredSnoozes);
}

void CoreEnvironment::UseFandomTask(PageTaskPtr task)
{
    sql::Database db = sql::Database::database();
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
    QSettings settings("settings/settings.ini", QSettings::IniFormat);
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
                                                  fetching::CacheStrategy cacheStrategy,
                                                  QString cutoffText,
                                                  ForcedFandomUpdateDate forcedDate)
{
    interfaces.fanfics->ClearProcessedHash();
    sql::Database db = sql::Database::database();
    FandomLoadProcessor proc(db, interfaces.fanfics, interfaces.fandoms, interfaces.pageTask, interfaces.db);

    connect(&proc, &FandomLoadProcessor::requestProgressbar, this, &CoreEnvironment::requestProgressbar);
    connect(&proc, &FandomLoadProcessor::updateCounter, this, &CoreEnvironment::updateCounter);
    connect(&proc, &FandomLoadProcessor::updateInfo, this, &CoreEnvironment::updateInfo);

    auto result = proc.CreatePageTaskFromFandoms(fandoms,
                                                 CreatePrototypeWithSearchParams(cutoffText),
                                                 taskComment,
                                                 cacheStrategy,
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
WebPage RequestPage(QString pageUrl, fetching::CacheStrategy cacheStrategy, bool autoSaveToDB)
{
    WebPage result;
#ifdef USE_WEBVIEW
    An<webview::PageManager> pager;
#endif
#ifdef NO_WEBVIEW
    An<PageManager> pager;
#endif
    pager->SetDatabase(sql::Database::database("PageCache"));
    result = pager->GetPage(pageUrl, cacheStrategy);
    if(autoSaveToDB && !result.isFromCache)
        pager->SavePageToDB(result);
    return result;
}

WebPage RequestPage(QString pageUrl, sql::Database db, fetching::CacheStrategy cacheStrategy,  bool autoSaveToDB)
{
    WebPage result;
#ifdef USE_WEBVIEW
    An<webview::PageManager> pager;
#endif
#ifdef NO_WEBVIEW
    An<PageManager> pager;
#endif
    pager->SetDatabase(db);
    result = pager->GetPage(pageUrl, cacheStrategy);
    if(autoSaveToDB)
        pager->SavePageToDB(result);
    return result;
}

}



