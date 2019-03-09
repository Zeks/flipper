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
#include "tasks/author_genre_iteration_processor.h"


#include <QSettings>
#include <QThread>
#include <QRegularExpression>


#define TO_STR2(x) #x
#define STRINGIFY(x) TO_STR2(x)

//template void core::DataHolder::LoadData<0>(QString);
//template void core::DataHolder::LoadData<1>(QString);
void AccumulatorIntoSectionStats(core::FicSectionStats& result, const core::FicListDataAccumulator& dataResult);

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
    rngData.reset(new core::RNGData);

    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
    auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
    dbInterface->ReadDbFile("dbcode/dbinit.sql",GetDbNameFromCurrentThread());

    An<core::RecCalculator> calculator;
    auto authors = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authors->db = mainDb;
    auto fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
    fanfics->db = mainDb;
    auto genres = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    genres->db = mainDb;
    fanfics->authorInterface = authors;
    calculator->holder.authorsInterface = authors;
    calculator->holder.fanficsInterface = fanfics;
    calculator->holder.genresInterface = genres;
    calculator->holder.settingsFile = "settings/settings_server.ini";

    qDebug() << "loading fics";
    calculator->holder.LoadData<core::rdt_fics>("ServerData");
    qDebug() << "loading favourites";
    calculator->holder.LoadData<core::rdt_favourites>("ServerData");
    qDebug() << "loading genres composite";
    //genres->loadOriginalGenresOnly = true;
    calculator->holder.LoadData<core::rdt_fic_genres_composite>("ServerData");
    //genres->loadOriginalGenresOnly = false;
    qDebug() << "loading moods";
    calculator->holder.LoadData<core::rdt_author_mood_distribution>("ServerData");

    if(calculator->holder.authorMoodDistributions.size() == 0)
    {
        qDebug() << "calculating moods";
        AuthorGenreIterationProcessor iteratorProcessor;
        calculator->holder.LoadData<core::rdt_author_genre_distribution>("ServerData");
        iteratorProcessor.ReprocessGenreStats(calculator->holder.genreComposites, calculator->holder.faves);
        auto testedAuthor = iteratorProcessor.resultingMoodAuthorData[94186];
        QStringList moodList;
        moodList << "Neutral" << "Funny"  << "Shocky" << "Flirty" << "Dramatic" << "Hurty" << "Bondy";
        for(int i= 0; i < moodList.size(); i++)
        {
            auto userValue =  interfaces::Genres::ReadMoodValue(moodList[i], testedAuthor);
            qDebug() << moodList[i] << ": " << userValue;
        }
        qDebug() << "saving moods";
        thread_boost::SaveData("ServerData","amd",iteratorProcessor.resultingMoodAuthorData);
        qDebug() << "finished saving moods";
    }

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
    QSettings settings("settings/settings_server.ini", QSettings::IniFormat);
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



UsedInSearch FeederService::PrepareSearch(::ProtoSpace::ResponseInfo* response,
                                          const ::ProtoSpace::Filter& protoFilter,
                                          const ::ProtoSpace::UserData& userData,
                                          RequestContext& reqContext)
{
    UsedInSearch result;
    if(!reqContext.Process(response))
        return result;


    if(!VerifySearchInput(reqContext.userToken,
                          protoFilter,
                          userData,
                          response))
        return result;




    core::StoryFilter filter = FilterFromTask(protoFilter, userData);
    auto ficSource = InitFicSource(reqContext.userToken, reqContext.dbContext.dbInterface);
    reqContext.dbContext.InitAuthors();

    if(filter.tagsAreUsedForAuthors)
    {
        auto* userThreadData = ThreadData::GetUserData();
        userThreadData->usedAuthors = reqContext.dbContext.authors->GetAuthorsForFics(userThreadData->ficIDsForActivetags);
    }

    for(auto recommender: filter.usedRecommenders)
    {
        auto* userThreadData = ThreadData::GetUserData();
        userThreadData->ficsForAuthorSearch += QSet<int>::fromList(reqContext.dbContext.authors->GetAllAuthorRecommendationIDs(recommender));
    }

    result.filter = filter;
    result.ficSource = ficSource;
    result.isValid = true;
    return result;
}

Status FeederService::Search(ServerContext* context, const ProtoSpace::SearchTask* task,
                             ProtoSpace::SearchResponse* response)
{
    Q_UNUSED(context);
    QLOG_INFO() << "///Searching";
    RequestContext reqContext("Searching",task->controls(), this);
    auto prepared = PrepareSearch(response->mutable_response_info(),task->filter(),
                                  task->user_data(),reqContext);

    if(!prepared.isValid)
        return Status::OK;

    QVector<core::Fic> data;
    TimedAction action("Fetching data",[&](){
        prepared.ficSource->FetchData(prepared.filter, &data);
    });
    action.run();

    AddToStatistics(reqContext.userToken, prepared.filter);

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
grpc::Status FeederService::SearchByFFNID(grpc::ServerContext *, const ProtoSpace::SearchByFFNIDTask *task, ProtoSpace::SearchByFFNIDResponse *response)
{
    RequestContext reqContext("Search by FFN id", task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;

    QLOG_INFO() << "Verifying request params";
    if(task->id() < 1)
        return Status::OK;

    auto ficSource = InitFicSource(reqContext.userToken, reqContext.dbContext.dbInterface);
    core::StoryFilter filter;
    filter.mode = core::StoryFilter::filtering_in_fics;
    filter.sortMode = core::StoryFilter::sm_wordcount;
    filter.slashFilter.slashFilterEnabled = false;

    filter.useThisFic = task->id();
    filter.useThisFicType = task->id_type() == ProtoSpace::SearchByFFNIDTask::ffn ? core::StoryFilter::EUseThisFicType::utf_ffn_id : core::StoryFilter::EUseThisFicType::utf_db_id;
    QLOG_INFO() << "Fetching data for fic ID: " << task->id();
    QLOG_INFO() << "ID type: " << task->id_type();

    QVector<core::Fic> data;
    TimedAction action("Fetching data",[&](){
        ficSource->FetchData(filter, &data);
    });
    action.run();
    for(const auto& fic: data)
        proto_converters::LocalFicToProtoFic(fic, response->mutable_fanfic());
    response->set_success(true);
    return Status::OK;
}

grpc::Status FeederService::GetUserMatches(grpc::ServerContext *context, const ProtoSpace::UserMatchRequest *task, ProtoSpace::UserMatchResponse *response)
{
    Q_UNUSED(context);
    QLOG_INFO() << "Starting user matches";
    An<core::RecCalculator> holder;
    QHash<int, core::MatchedFics> fics;
    QLOG_INFO() << "received user task of size: " << task->test_users_size();
    Roaring r;
    Roaring ignoredFandoms;
    if(task->user_fics_size() > 0)
    {
        for(int i = 0; i < task->user_fics_size(); i++)
            r.add(task->user_fics(i));
        QLOG_INFO() << "reading fandom ignores";
        for(int i = 0; i < task->fandom_ignores_size(); i++)
        {
            if(task->fandom_ignores(i) >= 0)
                ignoredFandoms.add(task->fandom_ignores(i));
        }
    }
    else
        r = holder->holder.faves[task->source_user()];
    core::UserMatchesInput input;
    input.userFavourites = r;
    input.userIgnoredFandoms = ignoredFandoms;
    for(int i = 0; i < task->test_users_size(); i++)
    {
        QLOG_INFO() << "Processing user: " << i;
        fics[task->test_users(i)] = holder->GetMatchedFics(input, task->test_users(i));
        QLOG_INFO() << "Ratio for user: " << task->test_users(i) << " " << fics[task->test_users(i)].ratio;
        QLOG_INFO() << "Ratio without ignores for user: " << task->test_users(i) << " " << fics[task->test_users(i)].ratioWithoutIgnores;
        QLOG_INFO() << "Matches for user: " << task->test_users(i) << " " << fics[task->test_users(i)].matches;
    }
    QLOG_INFO() << "Responding";
    response->set_success(true);
    for(auto user : fics.keys())
    {
        auto match = response->add_matches();
        match->set_user_id(user);
        match->set_ratio(fics[user].ratio);
        match->set_ratio_without_ignores(fics[user].ratioWithoutIgnores);
        for(auto fic: fics[user].matches)
            match->add_fic_id(fic);
    }
    return Status::OK;
}
Status FeederService::GetFicCount(ServerContext* context, const ProtoSpace::FicCountTask* task,
                                  ProtoSpace::FicCountResponse* response)
{
    Q_UNUSED(context);
    RequestContext reqContext("Getting fic count",task->controls(), this);
    auto prepared = PrepareSearch(response->mutable_response_info(),task->filter(),
                                  task->user_data(),reqContext);

    if(!prepared.isValid)
        return Status::OK;

    QVector<core::Fic> data;
    int count = 0;
    TimedAction ("Getting fic count",[&](){
        count = prepared.ficSource->GetFicCount(prepared.filter);
    }).run();

    response->set_fic_count(count);
    QLOG_INFO() << " ";
    return Status::OK;
}

Status FeederService::SyncFandomList(ServerContext* context, const ProtoSpace::SyncFandomListTask* task,
                                     ProtoSpace::SyncFandomListResponse* response)
{
    Q_UNUSED(context);
    RequestContext reqContext("Fandom synch",task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;

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

genre_stats::GenreMoodData CalcMoodDistributionForFicList(QList<uint32_t> ficList, core::FicGenreCompositeType ficGenres){

    genre_stats::GenreMoodData result;
    Roaring r;
    for(auto fic : ficList)
        r.add(fic);

    QHash<int, Roaring> favourites;
    AuthorGenreIterationProcessor iteratorProcessor;
    favourites[0] = r;
    iteratorProcessor.ReprocessGenreStats(ficGenres, favourites);
    result.isValid = true;
    result.listMoodData = iteratorProcessor.resultingMoodAuthorData[0];
    result.listGenreData = iteratorProcessor.resultingGenreAuthorData[0];
    qDebug() << "Logging reference list";
    result.listMoodData.Log();

    QStringList moodList;
    moodList << "Neutral" << "Funny"  << "Shocky" << "Flirty" << "Dramatic" << "Hurty" << "Bondy";
    for(int i= 0; i < moodList.size(); i++)
    {
        auto userValue =  interfaces::Genres::ReadMoodValue(moodList[i], result.listMoodData);
        if(userValue >= 0.5)
        {
            qDebug() << "detected axis mood:" << moodList[i];
            result.moodAxis.push_back(moodList[i]);
        }
    }

    return result;
}

Status FeederService::RecommendationListCreation(ServerContext* context, const ProtoSpace::RecommendationListCreationRequest* task,
                                                 ProtoSpace::RecommendationListCreationResponse* response)
{

    Q_UNUSED(context);
    RequestContext reqContext("Reclist Creation",task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;


    QLOG_INFO() << "Verifying request params";
    if(!VerifyRecommendationsRequest(task, response->mutable_response_info()))
    {
        SetRecommedationDataError(response->mutable_response_info());
        QLOG_INFO() << "data size error, exiting";
        return Status::OK;
    }

    reqContext.dbContext.InitFanfics();

    //reqContext.dbContext.InitAuthors();

    QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
    params->name =  proto_converters::FS(task->list_name());
    params->minimumMatch = task->min_fics_to_match();
    params->userFFNId = task->users_ffn_profile_id();
    if(params->userFFNId != -1)
        params->userFFNId  = reqContext.dbContext.authors->GetRecommenderIDByFFNId(params->userFFNId);
    params->pickRatio = task->max_unmatched_to_one_matched();
    params->alwaysPickAt = task->always_pick_at();
    params->useWeighting = task->use_weighting();
    params->useMoodAdjustment = task->use_mood_filtering();
    for(auto i = 0; i< task->user_data().ignored_fandoms().fandom_ids_size(); i++)
        params->ignoredFandoms.insert(task->user_data().ignored_fandoms().fandom_ids(i));
    for(auto i = 0; i< task->user_data().liked_authors_size(); i++)
        params->likedAuthors.insert(task->user_data().liked_authors(i));
    for(auto i = 0; i< task->user_data().negative_feedback().basicnegatives_size(); i++)
        params->minorNegativeVotes.insert(task->user_data().negative_feedback().basicnegatives(i));
    for(auto i = 0; i< task->user_data().negative_feedback().basicnegatives_size(); i++)
        params->majorNegativeVotes.insert(task->user_data().negative_feedback().strongnegatives(i));


    params->Log();

    QHash<uint32_t, core::FicWeightPtr> fetchedFics;
    QSet<int> sourceFics;
    for(int i = 0; i < task->id_packs().ffn_ids_size(); i++)
    {
        sourceFics.insert(task->id_packs().ffn_ids(i));
        //recs->recommendationList[task->id_packs().ffn_ids(i)]
    }
    if(sourceFics.size() == 0)
        return Status::OK;
    QLOG_INFO() << "source contains fics: " << sourceFics.size();
    reqContext.recsData->sourceFics = sourceFics;
    TimedAction action("Fic ids conversion",[&](){
        fetchedFics = reqContext.dbContext.fanfics->GetFicsForRecCreation();
    });
    action.run();


    An<core::RecCalculator> holder;
    auto moodData = CalcMoodDistributionForFicList(fetchedFics.keys(), holder->holder.genreComposites);

    auto list = holder->GetMatchedFicsForFavList(fetchedFics, params, moodData);
    int baseVotes = params->useMoodAdjustment ? 100 : 1;
    TimedAction dataPassAction("Passing data: ",[&](){
        auto* targetList = response->mutable_list();
        targetList->set_list_name(proto_converters::TS(params->name));
        targetList->set_list_ready(true);
        QLOG_INFO() << "setting list ready to true";
        using core::AuthorWeightingResult;
        typedef core::AuthorWeightingResult::EAuthorType EAuthorType;
        for(int key: list.recommendations.keys())
        {
            //QLOG_INFO() << " n_fic_id: " << key << " n_matches: " << list[key];
            if(!holder->holder.fics.contains(key))
            {
                qDebug() << "probably an older database, skipping key: " << key;
                continue;
            }
            if(sourceFics.contains(key) && !task->return_sources())
                continue;
            int adjustedVotes = list.recommendations[key]/(baseVotes);
            if(adjustedVotes < 1)
                adjustedVotes = 1;
            if((params->useMoodAdjustment && (list.recommendations[key]/(baseVotes*list.pureMatches[key])) < 1) &&
                    list.decentMatches[key] == 0 &&
                    !params->likedAuthors.contains(holder->holder.fics[key]->authorId))
            {
                bool axisGenre = false;;
                qDebug() << "attempting to purge fic: " << key;
                QHash<int, QList<genre_stats::GenreBit>>& ref = holder->holder.genreComposites;
                QList<genre_stats::GenreBit>& refList = ref[key];
                double maxValue = 0.;
                // shit code, but I really don't want to refactor rn
                for(auto genreBit: refList)
                {
                    if(genreBit.relevance > maxValue)
                        maxValue = genreBit.relevance;
                }

                for(auto genreBit: refList)
                {
                    qDebug() << "genres: " << genreBit.genres << " relevance: " << genreBit.relevance;
                    if(genreBit.relevance/maxValue > 0.45)
                    {
                        for(auto actualGenre : genreBit.genres)
                        {
                            auto mood = interfaces::Genres::MoodForGenre(actualGenre);
                            if(moodData.moodAxis.contains(mood))
                                axisGenre = true;
                        }
                    }
                }

                if(!axisGenre)
                    targetList->add_purged(1);
                else
                    targetList->add_purged(0);
            }
            else {
                targetList->add_purged(0);
            }
            targetList->add_fic_matches(adjustedVotes);

            targetList->add_fic_ids(key);
            //targetList->add_fic_matches(list.recommendations[key]/100);
            //targetList->add_fic_matches(list.recommendations[key]);
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
        qDebug() << "Match report will contain: " << list.matchReport.keys().size() << " fics";
        for(auto author: list.authors)
            response->mutable_list()->add_author_ids(author);

        for(int key: list.matchReport.keys())
        {
            if(sourceFics.contains(key) && !task->return_sources())
                continue;
            (*targetList->mutable_match_report())[key] = list.matchReport[key];
        }
    });
    dataPassAction.run();
    QLOG_INFO() << "Byte size will be: " << response->ByteSize();
    return Status::OK;
}

Status FeederService::GetDBFicIDS(ServerContext* context, const ProtoSpace::FicIdRequest* task,
                                  ProtoSpace::FicIdResponse* response)
{
    Q_UNUSED(context);
    RequestContext reqContext("FFN fic IDS",task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;

    QLOG_INFO() << "Verifying request params";
    if(!VerifyIDPack(task->ids(), response->mutable_response_info()))
        return Status::OK;

    QHash<int, int> idsToFill;
    for(int i = 0; i < task->ids().ffn_ids_size(); i++)
        idsToFill[task->ids().ffn_ids(i)] = -1;
    reqContext.dbContext.InitFanfics();
    bool result = reqContext.dbContext.fanfics->ConvertFFNTaggedFicsToDB(idsToFill);
    if(!result)
    {
        response->set_success(false);
        return Status::OK;
    }
    response->set_success(true);
    for(int fic: idsToFill.keys())
    {
        //QLOG_INFO() << "Returning fic ids: " << "DB: " << idsToFill[fic] << " FFN: " << fic;
        response->mutable_ids()->add_ffn_ids(fic);
        response->mutable_ids()->add_db_ids(idsToFill[fic]);
    }
    return Status::OK;
}

Status FeederService::GetFFNFicIDS(ServerContext* context, const ProtoSpace::FicIdRequest* task,
                                   ProtoSpace::FicIdResponse* response)
{
    Q_UNUSED(context);
    RequestContext reqContext("FFN fic IDS",task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;

    QLOG_INFO() << "Verifying request params";
    if(!VerifyIDPack(task->ids(), response->mutable_response_info()))
        return Status::OK;

    reqContext.dbContext.InitFanfics();
    QHash<int, int> idsToFill;
    for(int i = 0; i < task->ids().db_ids_size(); i++)
        idsToFill[task->ids().db_ids(i)] = -1;

    bool result = reqContext.dbContext.fanfics->ConvertDBFicsToFFN(idsToFill);
    if(!result)
    {
        response->set_success(false);
        return Status::OK;
    }
    response->set_success(true);
    for(int fic: idsToFill.keys())
    {
        //QLOG_INFO() << "Returning fic ids: " << "FFN: " << idsToFill[fic] << " DB: " << fic;
        response->mutable_ids()->add_ffn_ids(idsToFill[fic]);
        response->mutable_ids()->add_db_ids(fic);
    }

    return Status::OK;
}



grpc::Status FeederService::GetFavListDetails(grpc::ServerContext *context,
                                              const ProtoSpace::FavListDetailsRequest *task,
                                              ProtoSpace::FavListDetailsResponse *response)
{
    Q_UNUSED(context);
    RequestContext reqContext("Favlist details",task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;

    QLOG_INFO() << "Verifying request params";
    if(!VerifyIDPack(task->id_packs(), response->mutable_response_info()))
        return Status::OK;

    reqContext.dbContext.InitFanfics();

    QHash<uint32_t, core::FicWeightPtr> fetchedFics;
    auto  sourceFics = ProcessFFNIDPackIntoFfnFicSet(task->id_packs());

    if(sourceFics.size() == 0)
        return Status::OK;

    reqContext.recsData->sourceFics = sourceFics;
    TimedAction action("Fic ids conversion",[&](){
        fetchedFics = reqContext.dbContext.fanfics->GetFicsForRecCreation();
    });
    action.run();
    core::FicSectionStats result;

    core::FicListDataAccumulator dataAccumulator;
    auto genresInterface  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    genresInterface->db = reqContext.dbContext.dbInterface->GetDatabase();;
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

grpc::Status FeederService::GetAuthorsForFicList(grpc::ServerContext *context, const ProtoSpace::AuthorsForFicsRequest *task, ProtoSpace::AuthorsForFicsResponse *response)
{
    Q_UNUSED(context);
    RequestContext reqContext("Authors for fics",task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;

    QLOG_INFO() << "Verifying request params";
    if(!VerifyIDPack(task->id_packs(), response->mutable_response_info()))
        return Status::OK;

    reqContext.dbContext.InitAuthors();

    QHash<uint32_t, core::FicWeightPtr> fetchedFics;
    auto  sourceFics = ProcessIDPackIntoFfnFicSet(task->id_packs());

    if(sourceFics.size() == 0)
        return Status::OK;

    auto* data = ThreadData::GetUserData();
    data->ficsForAuthorSearch = sourceFics;
    QLOG_INFO() << "Fetching authors";
    auto result = reqContext.dbContext.authors->GetHashAuthorsForFics(sourceFics);
    for(auto fic : result.keys())
    {
        response->add_fics(fic);
        response->add_authors(result[fic]);
    }
    response->set_success(true);
    return Status::OK;
}

grpc::Status FeederService::GetAuthorsFromRecListContainingFic(grpc::ServerContext *context, const ProtoSpace::AuthorsForFicInReclistRequest *task, ProtoSpace::AuthorsForFicInReclistResponse *response)
{
    Q_UNUSED(context);
    RequestContext reqContext("Authors for fic in reclist", task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;

    QLOG_INFO() << "Verifying request params";
    if(task->fic_id() < 1)
        return Status::OK;

    reqContext.dbContext.InitAuthors();
    auto allAuthors = reqContext.dbContext.authors->GetRecommendersForFics({task->fic_id()});
    auto recsAuthorsList = QString::fromStdString(task->author_list()).split(",", QString::SkipEmptyParts);
    QSet<int> result;
    for(auto author: recsAuthorsList)
    {
        if(allAuthors.contains(author.toInt()))
            result.insert(author.toInt());
    }
    for(auto author : result)
    {
        response->add_filtered_authors(author);
    }
    response->set_success(true);
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
    if(!ProcessUserToken(userData, userToken, responseInfo))
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
    QLOG_INFO() << "Using rec list of size: " << filter.recsHash.size();
    recs->recommendationList = filter.recsHash;

    return filter;
}

QSharedPointer<FicSource> FeederService::InitFicSource(QString userToken,
                                                       QSharedPointer<database::IDBWrapper> dbInterface)
{
    //DatabaseContext dbContext;
    QSharedPointer<FicSource> ficSource(new FicSourceDirect(dbInterface,rngData));
    FicSourceDirect* convertedFicSource = dynamic_cast<FicSourceDirect*>(ficSource.data());
    QLOG_INFO() << "Initializing fic source mode";
    convertedFicSource->InitQueryType(true, userToken);
    //QLOG_INFO() << "Initialized fic source mode";
    return ficSource;
}

QSet<int> FeederService::ProcessIDPackIntoFfnFicSet(const ProtoSpace::SiteIDPack & pack)
{
    QSet<int> fics;
    QLOG_INFO() << "passing fic set of size: " << pack.db_ids_size();
    for(int i = 0; i < pack.db_ids_size(); i++)
        fics.insert(pack.db_ids(i));
    return fics;
}
QSet<int> FeederService::ProcessFFNIDPackIntoFfnFicSet(const ProtoSpace::SiteIDPack & pack)
{
    QSet<int> fics;
    QLOG_INFO() << "passing fic set of size: " << pack.ffn_ids_size();
    for(int i = 0; i < pack.ffn_ids_size(); i++)
        fics.insert(pack.ffn_ids(i));
    return fics;
}

void FeederService::OnPrintStatistics()
{
    PrintStatistics();
}


RequestContext::RequestContext(QString requestName, const ProtoSpace::ControlInfo & control, FeederService *server)
{
    userToken = QString::fromStdString(control.user_token());
    QLOG_INFO() << "Received requst: " << requestName << " from: " << userToken;
    this->server = server;
    recsData = ThreadData::GetRecommendationData();
}

bool RequestContext::Process(ProtoSpace::ResponseInfo * info)
{
    if(!VerifyUserToken(userToken,info))
        return false;

    An<UserTokenizer> keeper;
    auto safetyToken = keeper->GetToken(userToken);
    if(!safetyToken)
    {
        SetTokenMatchError(info);
        return false;
    }
    server->AddToRecStatistics(userToken);
    return true;
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
