/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
#include "servers/feed.h"
#include "servers/token_processing.h"
#include "servers/database_context.h"
#include "favholder.h"

#include "tokenkeeper.h"
#include "timeutils.h"
#include "core/section.h"
#include "core/fav_list_analysis.h"
#include "in_tag_accessor.h"
#include "logger/QsLog.h"
#include "loggers/usage_statistics.h"
#include "grpc/grpc_source.h"

#include "Interfaces/data_source.h"
#include "Interfaces/ffn/ffn_authors.h"
#include "Interfaces/ffn/ffn_fanfics.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/fanfics.h"
#include "Interfaces/genres.h"
#include "Interfaces/recommendation_lists.h"


#include <QSettings>
#include <QThread>
#include <QRegularExpression>


#define TO_STR2(x) #x
#define STRINGIFY(x) TO_STR2(x)

//template void core::DataHolder::LoadData<0>(QString);
//template void core::DataHolder::LoadData<1>(QString);


static QString GetDbNameFromCurrentThread(){
    std::stringstream ss;
    ss << std::this_thread::get_id();
    std::string id = ss.str();
    return QString("Crawler_") + QString::fromStdString(id);
}

FeederService::FeederService(QObject* parent): QObject(parent){
    startedAt = QDateTime::currentDateTimeUtc();
    allSearches = 0;
    genericSearches = 0;
    recommendationsSearches = 0;
    randomSearches = 0;

    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
    auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
    dbInterface->ReadDbFile("dbcode/dbinit.sql",GetDbNameFromCurrentThread());

    An<core::RecCalculator> calculator;
    auto authors = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authors->db = mainDb;
    calculator->holder.authorsInterface = authors;
    calculator->holder.settingsFile = "settings_server.ini";

    calculator->holder.LoadData<core::rdt_favourites>("ServerData");
    logTimer.reset(new QTimer());
    logTimer->start(3600000);
    connect(logTimer.data(), SIGNAL(timeout()), this, SLOT(OnPrintStatistics()), Qt::QueuedConnection);
}

FeederService::~FeederService()
{
    qDebug() << "Destroying server";
}

Status FeederService::GetStatus(ServerContext* context, const ProtoSpace::StatusRequest* task,
                 ProtoSpace::StatusResponse* response)
{
    Q_UNUSED(context);
    QString userToken = QString::fromStdString(task->controls().user_token());
    QLOG_INFO() << "Received status request from: " << userToken;
    QSettings settings("settings_server.ini", QSettings::IniFormat);
    auto motd = settings.value("Settings/motd", "Have fun searching.").toString();
    bool attached = settings.value("Settings/DBAttached", true).toBool();
    response->set_message_of_the_day(motd.toStdString());
    response->set_database_attached(attached);
    response->set_need_to_show_motd(settings.value("Settings/motdRequired", false).toBool());
    auto protocolVersion = QString(STRINGIFY(PROTOCOL_VERSION)).toInt();
    QLOG_INFO() << "Passing protocol version: " << protocolVersion;
    response->set_protocol_version(protocolVersion);
    return Status::OK;
}
Status FeederService::Search(ServerContext* context, const ProtoSpace::SearchTask* task,
              ProtoSpace::SearchResponse* response)
{
    Q_UNUSED(context);
    QString userToken = QString::fromStdString(task->controls().user_token());
    QLOG_INFO() << "////////////Received search task from: " << userToken;

    if(!VerifySearchInput(userToken,
                          task->filter(),
                          task->user_data(),
                          response->mutable_response_info()))
        return Status::OK;

    core::StoryFilter filter = FilterFromTask(task->filter(), task->user_data());
    auto ficSource = InitFicSource(userToken);
    if(filter.tagsAreUsedForAuthors)
    {
        FicSourceDirect* convertedFicSource = dynamic_cast<FicSourceDirect*>(ficSource.data());
        auto* userThreadData = ThreadData::GetUserData();
        auto authors = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
        authors->db = convertedFicSource->db->GetDatabase();
        userThreadData->usedAuthors = authors->GetAuthorsForFics(userThreadData->ficIDsForActivetags);
    }

    QVector<core::Fic> data;
    TimedAction action("Fetching data",[&](){
        ficSource->FetchData(filter, &data);
    });
    action.run();

    AddToStatistics(userToken, filter);

    QLOG_INFO() << "Fetch performed in: " << action.ms;
    TimedAction convertAction("Converting data",[&](){
        for(const auto& fic: data)
            proto_converters::LocalFicToProtoFic(fic, response->add_fanfics());
    });
    convertAction.run();
    QLOG_INFO() << "Convert performed in: " << convertAction.ms;

    QLOG_INFO() << "Fetched fics: " << data.size();
    QLOG_INFO() << " ";
    return Status::OK;
}

Status FeederService::GetFicCount(ServerContext* context, const ProtoSpace::FicCountTask* task,
                   ProtoSpace::FicCountResponse* response)
{
    Q_UNUSED(context);
    QString userToken = QString::fromStdString(task->controls().user_token());
    QLOG_INFO() << "////////////Received fic count task from: " << userToken;
    if(!VerifySearchInput(userToken,
                          task->filter(),
                          task->user_data(),
                          response->mutable_response_info()))
        return Status::OK;

    core::StoryFilter filter = FilterFromTask(task->filter(), task->user_data());
    auto ficSource = InitFicSource(userToken);
    if(filter.tagsAreUsedForAuthors)
    {
        FicSourceDirect* convertedFicSource = dynamic_cast<FicSourceDirect*>(ficSource.data());
        auto* userThreadData = ThreadData::GetUserData();
        auto authors = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
        authors->db = convertedFicSource->db->GetDatabase();
        qDebug() << "getting authors";
        userThreadData->usedAuthors = authors->GetAuthorsForFics(userThreadData->ficIDsForActivetags);
    }

    QVector<core::Fic> data;
    int count = 0;
    TimedAction ("Getting fic count",[&](){
        count = ficSource->GetFicCount(filter);
    }).run();

    response->set_fic_count(count);
    QLOG_INFO() << " ";
    return Status::OK;
}

Status FeederService::SyncFandomList(ServerContext* context, const ProtoSpace::SyncFandomListTask* task,
                      ProtoSpace::SyncFandomListResponse* response)
{
    Q_UNUSED(context);
    QString userToken = QString::fromStdString(task->controls().user_token());
    QLOG_INFO() << "////////////Received sync fandoms task from: " << userToken;

    if(!VerifyUserToken(userToken))
    {
        SetTokenError(response->mutable_response_info());
        return Status::OK;
    }
    AddToStatistics(userToken);

    DatabaseContext dbContext;
    QSharedPointer<interfaces::Fandoms> fandomInterface (new interfaces::Fandoms());
    fandomInterface->db = dbContext.dbInterface->GetDatabase();
    auto lastServerFandomID = fandomInterface->GetLastFandomID();
    QLOG_INFO() << "Client last fandom ID: " << task->last_fandom_id();
    QLOG_INFO() << "Server last fandom ID: " << lastServerFandomID;
    if(lastServerFandomID == task->last_fandom_id())
        response->set_needs_update(false);
    else
    {
        TimedAction ("Processing fandoms",[&](){
            response->set_needs_update(true);
            auto fandoms = fandomInterface->LoadAllFandomsAfter(task->last_fandom_id());
            QLOG_INFO() << "Sending new fandoms to the client:" << fandoms.size();
            for(auto coreFandom: fandoms)
            {
                auto* protoFandom = response->add_fandoms();
                proto_converters::LocalFandomToProtoFandom(*coreFandom, protoFandom);
            }
        }).run();
    }
    QLOG_INFO() << "Finished fandom sync task";
    QLOG_INFO() << " ";
    QLOG_INFO() << " ";
    QLOG_INFO() << " ";
    return Status::OK;
}
Status FeederService::RecommendationListCreation(ServerContext* context, const ProtoSpace::RecommendationListCreationRequest* task,
                                  ProtoSpace::RecommendationListCreationResponse* response)
{

    Q_UNUSED(context);
    QString userToken = QString::fromStdString(task->controls().user_token());
    QLOG_INFO() << "////////////Received reclists task from: " << userToken;
    QLOG_INFO() << "Verifying user token";
    if(!VerifyUserToken(userToken))
    {
        SetTokenError(response->mutable_response_info());
        QLOG_INFO() << "token error, exiting";
        return Status::OK;
    }
    QLOG_INFO() << "Verifying request params";
    if(!VerifyRecommendationsRequest(task))
    {
        SetRecommedationDataError(response->mutable_response_info());
        QLOG_INFO() << "data size error, exiting";
        return Status::OK;
    }
    An<UserTokenizer> keeper;

    auto safetyToken = keeper->GetToken(userToken);
    if(!safetyToken)
    {
        SetTokenMatchError(response->mutable_response_info());
        return Status::OK;
    }
    AddToRecStatistics(userToken);
    DatabaseContext dbContext;

    QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
    params->name =  proto_converters::FS(task->list_name());
    params->minimumMatch = task->min_fics_to_match();
    params->pickRatio = task->max_unmatched_to_one_matched();
    params->alwaysPickAt = task->always_pick_at();
    params->useWeighting = task->use_weighting();

    QSharedPointer<interfaces::Fanfics> fanficsInterface (new interfaces::FFNFanfics());
    fanficsInterface->db = dbContext.dbInterface->GetDatabase();

    auto* recs = ThreadData::GetRecommendationData();

    QHash<uint32_t, core::FicWeightPtr> fetchedFics;
    QSet<int> sourceFics;
    for(int i = 0; i < task->id_packs().ffn_ids_size(); i++)
    {
        sourceFics.insert(task->id_packs().ffn_ids(i));
        //recs->recommendationList[task->id_packs().ffn_ids(i)]
    }
    if(sourceFics.size() == 0)
        return Status::OK;
    recs->sourceFics = sourceFics;
    TimedAction action("Fic ids conversion",[&](){
        fetchedFics = fanficsInterface->GetFicsForRecCreation();
    });
    action.run();

    An<core::RecCalculator> holder;
    auto list = holder->GetMatchedFicsForFavList(fetchedFics, params);

    auto* targetList = response->mutable_list();
    targetList->set_list_name(proto_converters::TS(params->name));
    targetList->set_list_ready(true);
    using core::AuthorWeightingResult;
    typedef core::AuthorWeightingResult::EAuthorType EAuthorType;
    for(int key: list.recommendations.keys())
    {
        //QLOG_INFO() << " n_fic_id: " << key << " n_matches: " << list[key];
        if(sourceFics.contains(key) && !task->return_sources())
            continue;
        targetList->add_fic_ids(key);
        targetList->add_fic_matches(list.recommendations[key]);
        auto* target = targetList->add_breakdowns();
        target->set_id(key);
        target->set_votes_common(list.breakdowns[key].authorTypeVotes[EAuthorType::common]);
        target->set_votes_uncommon(list.breakdowns[key].authorTypeVotes[EAuthorType::uncommon]);
        target->set_votes_rare(list.breakdowns[key].authorTypeVotes[EAuthorType::rare]);
        target->set_votes_unique(list.breakdowns[key].authorTypeVotes[EAuthorType::unique]);

        target->set_counts_common(list.breakdowns[key].authorTypes[EAuthorType::common]);
        target->set_counts_uncommon(list.breakdowns[key].authorTypes[EAuthorType::uncommon]);
        target->set_counts_rare(list.breakdowns[key].authorTypes[EAuthorType::rare]);
        target->set_counts_unique(list.breakdowns[key].authorTypes[EAuthorType::unique]);
    }
    for(int key: list.matchReport.keys())
    {
        if(sourceFics.contains(key) && !task->return_sources())
            continue;
        (*targetList->mutable_match_report())[key] = list.matchReport[key];
    }
    return Status::OK;
}

Status FeederService::GetDBFicIDS(ServerContext* context, const ProtoSpace::FicIdRequest* task,
                   ProtoSpace::FicIdResponse* response)
{
    Q_UNUSED(context);
    QString userToken = QString::fromStdString(task->controls().user_token());
    QLOG_INFO() << "////////////Received sync fandoms task from: " << userToken;

    if(!VerifyUserToken(userToken))
    {
        SetTokenError(response->mutable_response_info());
        return Status::OK;
    }
    if(!VerifyIDPack(task->ids()))
    {
        SetFicIDSyncDataError(response->mutable_response_info());
        return Status::OK;
    }
    An<UserTokenizer> keeper;
    auto safetyToken = keeper->GetToken(userToken);
    if(!safetyToken)
    {
        SetTokenMatchError(response->mutable_response_info());
        return Status::OK;
    }
    DatabaseContext dbContext;

    QHash<int, int> idsToFill;
    for(int i = 0; i < task->ids().ffn_ids_size(); i++)
        idsToFill[task->ids().ffn_ids(i)] = -1;
    QSharedPointer<interfaces::Fanfics> fanficsInterface (new interfaces::FFNFanfics());
    fanficsInterface->db = dbContext.dbInterface->GetDatabase();
    bool result = fanficsInterface->ConvertFFNTaggedFicsToDB(idsToFill);
    if(!result)
    {
        response->set_success(false);
        return Status::OK;
    }
    response->set_success(true);
    for(int fic: idsToFill.keys())
    {
        QLOG_INFO() << "Returning fic ids: " << "DB: " << idsToFill[fic] << " FFN: " << fic;
        response->mutable_ids()->add_ffn_ids(fic);
        response->mutable_ids()->add_db_ids(idsToFill[fic]);
    }
    return Status::OK;
}

Status FeederService::GetFFNFicIDS(ServerContext* context, const ProtoSpace::FicIdRequest* task,
                   ProtoSpace::FicIdResponse* response)
{
    Q_UNUSED(context);
    QString userToken = QString::fromStdString(task->controls().user_token());
    QLOG_INFO() << "////////////Received sync fandoms task from: " << userToken;

    if(!VerifyUserToken(userToken))
    {
        SetTokenError(response->mutable_response_info());
        return Status::OK;
    }
    if(!VerifyIDPack(task->ids()))
    {
        SetFicIDSyncDataError(response->mutable_response_info());
        return Status::OK;
    }
    An<UserTokenizer> keeper;
    auto safetyToken = keeper->GetToken(userToken);
    if(!safetyToken)
    {
        SetTokenMatchError(response->mutable_response_info());
        return Status::OK;
    }
    DatabaseContext dbContext;

    QHash<int, int> idsToFill;
    for(int i = 0; i < task->ids().db_ids_size(); i++)
        idsToFill[task->ids().db_ids(i)] = -1;
    QSharedPointer<interfaces::Fanfics> fanficsInterface (new interfaces::FFNFanfics());
    fanficsInterface->db = dbContext.dbInterface->GetDatabase();
    bool result = fanficsInterface->ConvertDBFicsToFFN(idsToFill);
    if(!result)
    {
        response->set_success(false);
        return Status::OK;
    }
    response->set_success(true);
    for(int fic: idsToFill.keys())
    {
        QLOG_INFO() << "Returning fic ids: " << "FFN: " << idsToFill[fic] << " DB: " << fic;
        response->mutable_ids()->add_ffn_ids(idsToFill[fic]);
        response->mutable_ids()->add_db_ids(fic);
    }
    return Status::OK;
}

void AccumulatorIntoSectionStats(core::FicSectionStats& result, const core::FicListDataAccumulator& dataResult)
{
    result.isValid = true;
    result.favourites = dataResult.ficCount;
    result.ficWordCount = dataResult.wordcount;
    result.averageWordsPerChapter = dataResult.result.averageWordsPerChapter;
    result.averageLength = dataResult.result.averageWordsPerFic;
    result.firstPublished = dataResult.firstPublished;
    result.lastPublished = dataResult.lastPublished;

    result.fandomsDiversity = dataResult.result.fandomDiversityRatio;
    result.explorerFactor = dataResult.result.explorerRatio;
    result.megaExplorerFactor = dataResult.result.megaExplorerRatio;
    result.crossoverFactor = dataResult.result.crossoverRatio;
    result.unfinishedFactor = dataResult.result.unfinishedRatio;
    result.genreDiversityFactor = dataResult.result.genreDiversityRatio;
    result.moodUniformity = dataResult.result.moodUniformityRatio;
    result.esrbMature = dataResult.result.matureRatio;

    result.crackRatio = dataResult.result.crackRatio;
    result.slashRatio = dataResult.result.slashRatio;
    result.smutRatio = dataResult.result.smutRatio;
    result.mostFavouritedSize = dataResult.result.mostFavouritedSize;
    result.sectionRelativeSize = dataResult.result.sectionRelativeSize;

    result.prevalentMood = dataResult.result.prevalentMood;
    result.moodSad = dataResult.result.moodRatios[1];
    result.moodNeutral = dataResult.result.moodRatios[2];
    result.moodHappy = dataResult.result.moodRatios[3];

    An<interfaces::GenreIndex> genreIndex;
    for(size_t i = 0; i < dataResult.result.genreRatios.size(); i++)
    {
        const auto& genre =  genreIndex->genresByIndex[i];
        result.genreFactors[genre.name] = dataResult.result.genreRatios[i];
    }

    for(auto fandom : dataResult.result.fandomRatios.keys())
        result.fandomFactorsConverted[fandom] = dataResult.result.fandomRatios[fandom];
    for(auto fandom : dataResult.fandomCounters.keys())
        result.fandomsConverted[fandom] = dataResult.fandomCounters[fandom];

    for(size_t i = 1; i < dataResult.result.sizeRatios.size(); i++)
        result.sizeFactors[i] = dataResult.result.sizeRatios[i];
}

grpc::Status FeederService::GetFavListDetails(grpc::ServerContext *context,
                                              const ProtoSpace::FavListDetailsRequest *task,
                                              ProtoSpace::FavListDetailsResponse *response)
{
    Q_UNUSED(context);
    QString userToken = QString::fromStdString(task->controls().user_token());
    QLOG_INFO() << "////////////Received reclists task from: " << userToken;
    QLOG_INFO() << "Verifying user token";
    if(!VerifyUserToken(userToken))
    {
        SetTokenError(response->mutable_response_info());
        QLOG_INFO() << "token error, exiting";
        return Status::OK;
    }
    QLOG_INFO() << "Verifying request params";
    if(!VerifyIDPack(task->id_packs()))
    {
        SetFicIDSyncDataError(response->mutable_response_info());
        return Status::OK;
    }
    An<UserTokenizer> keeper;
    auto safetyToken = keeper->GetToken(userToken);
    if(!safetyToken)
    {
        SetTokenMatchError(response->mutable_response_info());
        return Status::OK;
    }
    AddToRecStatistics(userToken);
    DatabaseContext dbContext;

    QSharedPointer<interfaces::Fanfics> fanficsInterface (new interfaces::FFNFanfics());
    fanficsInterface->db = dbContext.dbInterface->GetDatabase();

    auto* recs = ThreadData::GetRecommendationData();

    QHash<uint32_t, core::FicWeightPtr> fetchedFics;
    QSet<int> sourceFics;
    for(int i = 0; i < task->id_packs().ffn_ids_size(); i++)
        sourceFics.insert(task->id_packs().ffn_ids(i));

    if(sourceFics.size() == 0)
        return Status::OK;

    recs->sourceFics = sourceFics;
    TimedAction action("Fic ids conversion",[&](){
        fetchedFics = fanficsInterface->GetFicsForRecCreation();
    });
    action.run();
    core::FicSectionStats result;

    core::FicListDataAccumulator dataAccumulator;
    auto genresInterface  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    genresInterface->db = dbContext.dbInterface->GetDatabase();;
    interfaces::GenreConverter conv;
    An<interfaces::GenreIndex> genreIndex;
    for(auto fic: fetchedFics)
    {
        fic->genres = conv.GetFFNGenreList(fic->genreString);
        for(auto genre: fic->genres)
        {
            if(auto genreObject = genreIndex->GenreByName(genre); genreObject.isValid)
            {
                auto index = genreObject.indexInDatabase;
                dataAccumulator.genreCounters[index]++;
            }
        }
        dataAccumulator.AddFandoms(fic->fandoms);
        dataAccumulator.AddFavourites(fic->favCount);
        dataAccumulator.AddPublishDate(fic->published);
        dataAccumulator.AddWordcount(fic->wordCount, fic->chapterCount);
        dataAccumulator.slashCounter += fic->slash;
        dataAccumulator.unfinishedCounter += !fic->complete;
        dataAccumulator.matureCounter += fic->adult;
    }
    for(auto i = 0; i < 22; i++)
    {
        qDebug() << genreIndex->genresByIndex[i].name << " : " << dataAccumulator.genreCounters[i];
    }
    dataAccumulator.ProcessIntoResult();
    AccumulatorIntoSectionStats(result, dataAccumulator);
    response->set_success(true);
    auto details = response->mutable_details();
    proto_converters::FavListLocalToProto(result, details);
    details->set_no_info(task->id_packs().ffn_ids_size() - fetchedFics.size());
    return Status::OK;
}


void FeederService::AddToStatistics(QString uuid, const core::StoryFilter& filter){
    StatisticsToken token;
    {
        QWriteLocker locker(&lock);
        StatisticsToken token = tokenData[uuid];
        searchedTokens.insert(uuid);
        allTokens.insert(uuid);
        allSearches++;
    }
    if(filter.sortMode == core::StoryFilter::sm_reccount ||
            filter.minRecommendations > 0)
    {
        token.recommendationsSearches++;
        recommendationsSearches++;
    }
    else
    {
        token.genericSearches++;
        genericSearches++;
    }
    if(filter.randomizeResults)
    {
        token.randomSearches++;
        randomSearches++;
    }
    token.lastUsed = QDateTime::currentDateTimeUtc();
    token.usedAt.push_back(token.lastUsed);
    {
        QWriteLocker locker(&lock);
        tokenData[uuid] = token;
    }
}

void FeederService::AddToStatistics(QString uuid)
{
    QWriteLocker locker(&lock);
    allTokens.insert(uuid);
    allSearches++;
}

void FeederService::AddToRecStatistics(QString uuid)
{
    QWriteLocker locker(&lock);
    allTokens.insert(uuid);
    recommendationTokens.insert(uuid);

}
void FeederService::CleaupOldTokens()
{
    QList<QString> toErase;
}
void FeederService::PrintStatistics(){
    // creating statistics message
    STAT_INFO() << "//////////////////////////////////////////////////////////////////////";
    STAT_INFO() << "Current time is: " << QDateTime::currentDateTimeUtc();
    STAT_INFO() << "Server started at: " << startedAt;
    STAT_INFO() << "Unique tokens in use: " << allTokens.size();
    STAT_INFO() << "Tokens that searched: " << searchedTokens.size();
    STAT_INFO() << "Tokens that created lists: " << recommendationTokens.size();
    STAT_INFO() << "Searches: " << allSearches;
    STAT_INFO() << "Generic: " << genericSearches;
    STAT_INFO() << "Recommendations: " << recommendationsSearches;
    STAT_INFO() << "Random: " << randomSearches;
}

bool FeederService::VerifySearchInput(QString userToken,
                                      const ProtoSpace::Filter & filter,
                                      const ProtoSpace::UserData & userData,
                                      ProtoSpace::ResponseInfo * responseInfo)
{
    if(!ProcessUserToken(userData, userToken))
    {
        SetTokenError(responseInfo);
        return false;
    }
    if(!VerifyFilterData(filter, userData))
    {
        SetFilterDataError(responseInfo);
        return false;
    }
    An<UserTokenizer> keeper;
    auto safetyToken = keeper->GetToken(userToken);
    if(!safetyToken)
    {
        SetTokenMatchError(responseInfo);
        return false;
    }
    return true;
}

core::StoryFilter FeederService::FilterFromTask(const ProtoSpace::Filter & grpcfilter, const ProtoSpace::UserData & grpcUserData)
{
    core::StoryFilter filter;
    TimedAction ("Converting filter",[&](){
        filter = proto_converters::ProtoIntoStoryFilter(grpcfilter, grpcUserData);

    }).run();

    auto* recs = ThreadData::GetRecommendationData();
    recs->recommendationList = filter.recsHash;

    return filter;
}

QSharedPointer<FicSource> FeederService::InitFicSource(QString userToken)
{
    DatabaseContext dbContext;
    QSharedPointer<FicSource> ficSource(new FicSourceDirect(dbContext.dbInterface));
    FicSourceDirect* convertedFicSource = dynamic_cast<FicSourceDirect*>(ficSource.data());
    QLOG_INFO() << "Initializing fic source mode";
    convertedFicSource->InitQueryType(true, userToken);
    //QLOG_INFO() << "Initialized fic source mode";
    return ficSource;
}

void FeederService::OnPrintStatistics()
{
    PrintStatistics();
}
