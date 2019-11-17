/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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
#include "include/tagwidget.h"
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
#include "include/tasks/author_cache_reprocessor.h"
#include "include/in_tag_accessor.h"
#include "include/timeutils.h"
#include "include/in_tag_accessor.h"
#include "include/url_utils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QTextCodec>
#include <QSettings>
#include <QMessageBox>
#include <QFile>
#include <QCoreApplication>


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

        QVector<core::Fic> newFanfics;
        interfaces::TagIDFetcherSettings tagFetcherSettings;
        tagFetcherSettings.allowSnoozed = filter.displaySnoozedFics;

        if(filter.useThisFic != -1)
        {
            FicSourceGRPC* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());

            grpcSource->FetchFic(filter.useThisFic,
                                 &newFanfics);
        }
        else {

            UserData userData;
            userData.allTaggedFics = interfaces.tags->GetAllTaggedFics(tagFetcherSettings);
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
                if(filter.showRecSources)
                {
                    auto sources = interfaces.recs->GetAllSourceFicIDs(filter.listForRecommendations);
                    for(auto fic : sources)
                        userData.allTaggedFics.remove(fic);
                }
                else
                {
                    auto sources = interfaces.recs->GetAllSourceFicIDs(filter.listForRecommendations);
                    for(auto fic : sources)
                        userData.allTaggedFics.insert(fic);
                }
            }

            userData.ignoredFandoms = interfaces.fandoms->GetIgnoredFandomsIDs();
            //userData.likedAuthors = likedAuthors;
            ficSource->userData = userData;

            core::StoryFilter::EScoreType scoreType = filter.sortMode != core::StoryFilter::sm_minimize_dislikes ? core::StoryFilter::st_points : core::StoryFilter::st_minimal_dislikes;

            QVector<int> recFics;
            core::ReclistFilter reclistFilter;
            reclistFilter.mainListId = interfaces.recs->GetCurrentRecommendationList();
            reclistFilter.minMatchCount = filter.minRecommendations;
            reclistFilter.limiter = filter.sourcesLimiter;
            reclistFilter.scoreType = scoreType;
            reclistFilter.displayPurged = filter.displayPurgedFics;

            filter.recsHash = interfaces.recs->GetAllFicsHash(reclistFilter);
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

        fanfics = newFanfics;
        currentLastFanficId = ficSource->lastFicId;
        for(auto& fic : fanfics)
        {
            if(fic.author_id > 1 && likedAuthors.contains(fic.author_id))
                fic.likedAuthor = true;
            if(ficScores.contains(fic.id))
                fic.score = ficScores[fic.id];
            if(snoozeInfo.contains(fic.id))
                fic.snoozeExpired = snoozeInfo[fic.id].expired;
        }
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
    ServerStatus status = grpcSource->GetStatus();
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
        QString statusString = QString("Your client version is out of date.\nSome (or all) features may not work.\nPlease get updated binary at https://github.com/Zeks/flipper");
        QMessageBox::critical(nullptr, "Warning!", statusString);
        return true;
    }
    if(status.messageRequired)
        QMessageBox::information(nullptr, "Attention!", status.motd);

    QVector<core::Fandom> fandoms;
    grpcSource->GetFandomListFromServer(interfaces.fandoms->GetLastFandomID(), &fandoms);
    if(fandoms.size() > 0)
        interfaces.fandoms->UploadFandomsIntoDatabase(fandoms);
    interfaces.fandoms->ReloadRecentFandoms();
    auto recentFandoms = interfaces.fandoms->GetRecentFandoms();
    if(recentFandoms.size() == 0)
    {
        interfaces.fandoms->PushFandomToTopOfRecent("Naruto");
        interfaces.fandoms->PushFandomToTopOfRecent("RWBY");
        interfaces.fandoms->PushFandomToTopOfRecent("Worm");
        interfaces.fandoms->PushFandomToTopOfRecent("High School DxD");
        interfaces.fandoms->PushFandomToTopOfRecent("Harry Potter");
    }

    //    else
    //        ficSource.reset(new FicSourceDirect(interfaces.db));

    auto result = interfaces.tags->ReadUserTags();

    ficSource->AddFicFilter(QSharedPointer<FicFilter>(new FicFilterSlash));

    auto storedRecList = uiSettings.value("Settings/currentList").toString();
    interfaces.recs->SetCurrentRecommendationList(interfaces.recs->GetListIdForName(storedRecList));
    interfaces.fandoms->Load();
    interfaces.recs->LoadAvailableRecommendationLists();
    interfaces.fandoms->FillFandomList(true);

    auto fics = interfaces.fanfics->GetFicIDsWithUnsetAuthors();
    if(fics.size() > 0)
    {
        auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
        if(grpcSource)
        {
            auto authors = grpcSource->GetAuthorsForFicList(fics);
            //            for(auto fic: authors.keys())
            //                QLOG_INFO() << "Fic: "  << fic << " Author: " << authors[fic];
            interfaces.fanfics->WriteAuthorsForFics(authors);
        }
    }

    auto positiveTags = settings.value("Tags/minorPositive").toString().split(",", QString::SkipEmptyParts);
    positiveTags += settings.value("Tags/majorPositive").toString().split(",", QString::SkipEmptyParts);
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
    interfaces.fandoms->isClient = true;
    interfaces.tags->db    = userDBInterface->GetDatabase();
    interfaces.genres->db  = userDBInterface->GetDatabase();

    interfaces.pageTask->db  = interfaces.tasks->GetDatabase();

}

int CoreEnvironment::GetResultCount()
{
    QSettings settings("settings/settings.ini", QSettings::IniFormat);

    UserData userData;

    interfaces::TagIDFetcherSettings tagFetcherSettings;
    tagFetcherSettings.allowSnoozed = filter.displaySnoozedFics;
    userData.allTaggedFics = interfaces.tags->GetAllTaggedFics(tagFetcherSettings);
    if(filter.activeTags.size() > 0)
    {
        tagFetcherSettings.tags = filter.activeTags;
        tagFetcherSettings.useAND = filter.tagsAreANDed;
        userData.ficIDsForActivetags = interfaces.tags->GetFicsTaggedWith(tagFetcherSettings);
    }
    userData.ignoredFandoms = interfaces.fandoms->GetIgnoredFandomsIDs();
    ficSource->userData = userData;

    QVector<int> recFics;
    //filter.recsHash = interfaces.recs->GetAllFicsHash(interfaces.recs->GetCurrentRecommendationList(), filter.minRecommendations);
    core::StoryFilter::EScoreType scoreType = filter.sortMode != core::StoryFilter::sm_minimize_dislikes ? core::StoryFilter::st_points : core::StoryFilter::st_minimal_dislikes;

    core::ReclistFilter reclistFilter;
    reclistFilter.mainListId = interfaces.recs->GetCurrentRecommendationList();
    reclistFilter.minMatchCount = filter.minRecommendations;
    reclistFilter.limiter = filter.sourcesLimiter;
    reclistFilter.scoreType = scoreType;
    filter.recsHash = interfaces.recs->GetAllFicsHash(reclistFilter);

    ficScores= interfaces.fanfics->GetScoresForFics();
    filter.scoresHash = ficScores;
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
    authorLoader.dbInterface = interfaces.db;
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

void CoreEnvironment::LoadAllLinkedAuthors(ECacheMode cacheMode)
{
    filter.mode = core::StoryFilter::filtering_in_recommendations;
    QStringList authorUrls;

    auto db = QSqlDatabase::database();
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
                                                       cacheMode,
                                                       true);

    UseAuthorTask(pageTask);
}

void CoreEnvironment::LoadAllLinkedAuthorsMultiFromCache()
{
    QSqlDatabase db = QSqlDatabase::database();
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
            qDebug()<< "Settign tag: " << "generictag" << " to: " << id;
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

int CoreEnvironment::BuildRecommendationsServerFetch(QSharedPointer<core::RecommendationList> list,
                                                     QVector<int> sourceFics,
                                                     bool automaticLike)
{

    qDebug() << "At start list id is: " << list->id;
    FicSourceGRPC* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());


    list->ficData.fics = sourceFics;



    database::Transaction transaction(interfaces.recs->db);
    list->id = interfaces.recs->GetListIdForName(list->name);
    qDebug() << "Fetched name for list is: " << list->name;

    QVector<core::IdPack> pack;
    pack.resize(sourceFics.size());
    int i = 0;
    for(auto source: sourceFics)
    {
        pack[i].ffn = source;
        i++;
    }
    grpcSource->GetInternalIDsForFics(&pack);

    list->likedAuthors = likedAuthors;
    //QLOG_INFO() << "Logging list params: ";
    list->Log();
    bool result = grpcSource->GetRecommendationListFromServer(*list);


    //    QSet<int> sourceSet;
    //    sourceSet.reserve(sourceFics.size());
    for(auto id: pack)
        list->ficData.sourceFics.insert(id.db);



    if(!result)
    {
        QLOG_ERROR() << "list creation failed";
        return -1;
    }
    if(list->ficData.fics.size() == 0)
    {
        return -1;
    }
    TimedAction action("List write: ",[&](){



        qDebug() << "Deleting list: " << list->id;
        interfaces.recs->DeleteList(list->id);

        interfaces.recs->LoadListIntoDatabase(list);
        //qDebug() << list->ficData.fics;
        interfaces.recs->LoadListFromServerIntoDatabase(list);

        interfaces.recs->UpdateFicCountInDatabase(list->id);
        interfaces.recs->SetCurrentRecommendationList(list->id);
        emit resetEditorText();
        auto keys = list->ficData.matchReport.keys();
        std::sort(keys.begin(), keys.end());
        emit updateInfo( QString("Matches: ") + QString("Times Found:") + "<br>");
        for(auto key: keys)
            emit updateInfo(QString::number(key).leftJustified(11, ' ').replace(" ", "&nbsp;")
                            + " " + QString::number(list->ficData.matchReport[key]) + "<br>");
        if(automaticLike)
        {
            QLOG_INFO() << "autolike is enabled";
            for(auto fic: list->ficData.sourceFics)
            {
                //QLOG_INFO() << "liking fic: "  << fic;
                if(fic > 0)
                    interfaces.tags->SetTagForFic(fic, "Liked");
            }
        }
    });
    action.run();
    transaction.finalize();
    return list->id;
}

core::FicSectionStats CoreEnvironment::GetStatsForFicList(QVector<int> sourceFics)
{
    FicSourceGRPC* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    QVector<core::IdPack> pack;
    pack.resize(sourceFics.size());
    int i = 0;
    for(auto source: sourceFics)
    {
        pack[i].ffn = source;
        i++;
    }
    auto result = grpcSource->GetStatsForFicList(pack);
    if(result.has_value())
        return *result;
    return core::FicSectionStats();
}

int CoreEnvironment::BuildRecommendationsLocalVersion(QSharedPointer<core::RecommendationList> params, bool clearAuthors)
{
    QSqlDatabase db = QSqlDatabase::database();
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
    for(auto authorId: allAuthors)
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
                                          QVector<int> sourceFics,
                                          bool automaticLike,
                                          bool clearAuthors)
{
    QSettings settings("settings/settings.ini", QSettings::IniFormat);
    int result = -1;
    if(settings.value("Settings/thinClient").toBool())
    {
        result =  BuildRecommendationsServerFetch(params,sourceFics, automaticLike);
    }
    else
        result = BuildRecommendationsLocalVersion(params, clearAuthors);
    return result;
}

int CoreEnvironment::BuildDiagnosticsForRecList(QSharedPointer<core::RecommendationList> list,
                                                    QVector<int> sourceFics)
{
    list->id = interfaces.recs->GetListIdForName(list->name);
    qDebug() << "At start list id is: " << list->id;
    FicSourceGRPC* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    list->ficData.fics = sourceFics;

    database::Transaction transaction(interfaces.recs->db);
    list->id = interfaces.recs->GetListIdForName(list->name);
    qDebug() << "Fetched name for list is: " << list->name;

    QVector<core::IdPack> pack;
    pack.resize(sourceFics.size());
    int i = 0;
    for(auto source: sourceFics)
    {
        pack[i].ffn = source;
        i++;
    }
    grpcSource->GetInternalIDsForFics(&pack);

    list->likedAuthors = likedAuthors;
    auto result = grpcSource->GetDiagnosticsForRecListFromServer(*list);
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
        interfaces.recs->LoadListIntoDatabase(list);
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
    for(auto task : tasks)
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
        for(auto task : tasksToResume)
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
    return true;
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
    params->maxUnmatchedPerMatch = 9999;
    interfaces.tags->SetTagForFic(id, "generictag");
    BuildRecommendations(params, {id}, false, !authorsLoaded);
    interfaces.tags->DeleteTag("generictag");
    interfaces.recs->SetFicsAsListOrigin({id}, params->id);
    transaction.finalize();
}

QVector<int> CoreEnvironment::GetListSourceFFNIds(int listId)
{
    QVector<int> result;
    auto sources = interfaces.recs->GetAllSourceFicIDs(listId);
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    QVector<core::IdPack> pack;
    pack.resize(sources.size());
    result.reserve(sources.size());
    int i = 0;
    for(auto source: sources)
    {
        pack[i].db = source;
        i++;
    }
    grpcSource->GetFFNIDsForFics(&pack);
    for(auto id : pack)
        result.push_back(id.ffn);


    return result;
}

QVector<int> CoreEnvironment::GetFFNIds(QSet<int> sources)
{
    QVector<int> result;
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    QVector<core::IdPack> pack;
    pack.resize(sources.size());
    result.reserve(sources.size());
    int i = 0;
    for(auto source: sources)
    {
        pack[i].db = source;
        i++;
    }
    grpcSource->GetFFNIDsForFics(&pack);
    for(auto id : pack)
        result.push_back(id.ffn);

    return result;
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
        Q_UNUSED(result)
        interfaces.fandoms->RecalculateFandomStats(fandoms.values());
    }
    transaction.finalize();
    return author;
}

QSet<QString>  CoreEnvironment::LoadAuthorFicIdsForRecCreation(QString url)
{
    auto result = LoadAuthorFics(url);
    QSet<QString> urlResult;
    for(auto fic : result)
    {
        if(fic->ficSource != core::Fic::efs_own_works)
            urlResult.insert(QString::number(fic->ffn_id));
    }

    if(result.size() < 500)
        return urlResult;
    QMessageBox::information(nullptr, "Attention!", "Due to fanfiction.net still not fixing their favourites page displaying only 500 entries."
                                             "Your profile will have to be parsed from m.fanfiction.net page.\n"
                                             "Please grab a cup of coffee or smth after you press OK to close this window and wait a bit.");

    // first we need to create an m. link
    url = url.remove("https://www.");
    url = url.prepend("https://m.");
    url = url.append("/?a=fs");
    QString prototype = url;
    // we need to grab the initial page and figure out the exact amount of pages we need to parse

    WebPage page;
    TimedAction fetchAction("Author initial mobile page fetch", [&](){
        page = env::RequestPage(prototype.trimmed(),  ECacheMode::dont_use_cache);
    });
    fetchAction.run(false);

    QString content;
    QRegularExpression rx("p[=](\\d+)['][>]Last[<][/]a[>]");
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
    for(int i = startOfUnreachablePart; i <= amountOfPagesToGrab; i++)
        mobileUrls.push_back(prototype + "&s=0&cid=0&p=" + QString::number(i));

    QList<QString> mobileStories;
    QRegularExpression rxStoryId("/s/(\\d+)/1");
    emit requestProgressbar(amountOfPagesToGrab - 25);
    int counter = 1;
    for(auto mobileUrl : mobileUrls)
    {
        WebPage page;
        TimedAction fetchAction("Author mobile page fetch", [&](){
            page = env::RequestPage(mobileUrl.trimmed(),  ECacheMode::dont_use_cache);
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
            QThread::msleep(500);
        QCoreApplication::processEvents();
    }
    return urlResult;
}

bool CoreEnvironment::TestAuthorID(QString id)
{
    QString url = "https://www.fanfiction.net/u/" + id;
    WebPage page;
    TimedAction fetchAction("Author page fetch", [&](){
        page = env::RequestPage(url.trimmed(),  ECacheMode::use_cache);
    });
    fetchAction.run(false);
    if(!page.content.contains("Anime"))
        return false;
    return true;
}

QList<QSharedPointer<core::Fic> > CoreEnvironment::LoadAuthorFics(QString url)
{
    QStringList result;
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

    return parser.processedStuff;
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

void CoreEnvironment::FillDBIDsForTags()
{
    database::Transaction transaction(interfaces.tags->db);
    auto pack = interfaces.tags->GetAllFicsThatDontHaveDBID();
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    grpcSource->GetInternalIDsForFics(&pack);
    interfaces.tags->FillDBIDsForFics(pack);
    transaction.finalize();
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

void CoreEnvironment::LoadNewScoreValuesForFanfics(core::ReclistFilter filter, QVector<core::Fic>& fanfics)
{
    interfaces.recs->FetchRecommendationsBreakdown(&fanfics, filter.mainListId);
    if(fanfics.size() <= 100){
        interfaces.recs->LoadPlaceAndRecommendationsData(&fanfics, filter);
    }
}

void CoreEnvironment::RefreshSnoozes()
{
    auto snoozeInfo = interfaces.fanfics->GetUserSnoozeInfo();
    auto* grpcSource = dynamic_cast<FicSourceGRPC*>(ficSource.data());
    auto expiredSnoozes = grpcSource->GetExpiredSnoozes(snoozeInfo);
    interfaces.fanfics->WriteExpiredSnoozes(expiredSnoozes);
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



