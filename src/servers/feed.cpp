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
#include "third_party/nanobench/nanobench.h"


#include <QSettings>
#include <QThread>
#include <QRegularExpression>


#define TO_STR2(x) #x
#define STRINGIFY(x) TO_STR2(x)

//template void core::DataHolder::LoadData<0>(QString);
//template void core::DataHolder::LoadData<1>(QString);
void AccumulatorIntoSectionStats(core::FavListDetails& result, const core::FicListDataAccumulator& dataResult);

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
        calculator->holder.LoadData<core::rdt_author_mood_distribution>("ServerData");
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
    response->set_last_database_update(settings.value("Settings/lastDBUpdate", "").toString().toStdString());
    response->set_need_to_show_motd(settings.value("Settings/motdRequired", false).toBool());
    auto majorProtocolVersion = QStringLiteral(STRINGIFY(MAJOR_PROTOCOL_VERSION)).toInt();
    auto minorProtocolVersion = QStringLiteral(STRINGIFY(MINOR_PROTOCOL_VERSION)).toInt();
    QLOG_INFO() << "Passing protocol version: " << majorProtocolVersion;
    response->set_protocol_version(majorProtocolVersion);
    auto protocol = response->mutable_current_protocol();
    protocol->set_major_version(majorProtocolVersion);
    protocol->set_minor_version(minorProtocolVersion);
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

    for(auto recommender: std::as_const(filter.usedRecommenders))
    {
        auto* userThreadData = ThreadData::GetUserData();
        const auto& list = reqContext.dbContext.authors->GetAllAuthorRecommendationIDs(recommender);
        userThreadData->ficsForAuthorSearch += QSet<int>(list.begin(), list.end());
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

    if(task->controls().protocol_version().major_version() < 2)
        return Status::CANCELLED;

    if(!prepared.isValid)
        return Status::OK;

    QVector<core::Fanfic> data;
    TimedAction action("Fetching data",[&](){
        prepared.ficSource->FetchData(prepared.filter, &data);
    });
    action.run();

    AddToStatistics(reqContext.userToken, prepared.filter);

    QLOG_INFO() << "Fetch performed in: " << action.ms;
    TimedAction convertAction("Converting data",[&](){
        for(const auto& fic: std::as_const(data))
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

    QLOG_TRACE() << "Verifying request params";
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

    QVector<core::Fanfic> data;
    TimedAction action("Fetching data",[&](){
        ficSource->FetchData(filter, &data);
    });
    action.run();
    for(const auto& fic: std::as_const(data))
        proto_converters::LocalFicToProtoFic(fic, response->mutable_fanfic());
    response->set_success(true);
    return Status::OK;
}

grpc::Status FeederService::GetUserMatches(grpc::ServerContext *context, const ProtoSpace::UserMatchRequest *task, ProtoSpace::UserMatchResponse *response)
{
    Q_UNUSED(context);
    QLOG_INFO() << "Starting user matches";
    An<core::RecCalculator> holder;
    QHash<int, core::FavouritesMatchResult> fics;
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

    for(auto i = fics.begin(); i != fics.end(); i++)
    {
        auto match = response->add_matches();
        const auto& value = i.value();
        match->set_user_id(i.key());
        match->set_ratio(value.ratio);
        match->set_ratio_without_ignores(value.ratioWithoutIgnores);
        for(auto fic: std::as_const(value.matches))
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

    if(task->controls().protocol_version().major_version() < 2)
        return Status::CANCELLED;

    if(!prepared.isValid)
        return Status::OK;

    //QVector<core::Fanfic> data;
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
            for(const auto& coreFandom: fandoms)
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
std::once_flag moodsFlag;
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

    static QStringList moodList;
    std::call_once(moodsFlag, [&](){
        moodList << "Neutral" << "Funny"  << "Shocky" << "Flirty" << "Dramatic" << "Hurty" << "Bondy";
    });

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


const static auto basicRecommendationsParamReader = [](RequestContext& reqContext, const auto& task) -> QSharedPointer<core::RecommendationList> {
    QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
    params->isAutomatic = task->data().general_params().is_automatic();
    params->useDislikes = task->data().general_params().use_dislikes();
    params->useDeadFicIgnore= task->data().general_params().use_dead_fic_ignore();
    params->name =  proto_converters::FS(task->data().list_name());
    params->minimumMatch = task->data().general_params().min_fics_to_match();
    params->userFFNId = task->data().general_params().users_ffn_profile_id();
    QLOG_INFO() << "Read user's FFN id: " << params->userFFNId;
    if(params->userFFNId != -1)
        params->userFFNId  = reqContext.dbContext.authors->GetRecommenderIDByFFNId(params->userFFNId);
    params->maxUnmatchedPerMatch = task->data().general_params().max_unmatched_to_one_matched();
    params->alwaysPickAt = task->data().general_params().always_pick_at();
    params->useWeighting = task->data().general_params().use_weighting();
    params->useMoodAdjustment = task->data().general_params().use_mood_filtering();
    for(auto i = 0; i< task->data().user_data().ignored_fandoms().fandom_ids_size(); i++)
        params->ignoredFandoms.insert(task->data().user_data().ignored_fandoms().fandom_ids(i));
    for(auto i = 0; i< task->data().user_data().ignored_fics_size(); i++)
        params->ignoredDeadFics.insert(task->data().user_data().ignored_fics(i));
    for(auto i = 0; i< task->data().user_data().liked_authors_size(); i++)
        params->likedAuthors.insert(task->data().user_data().liked_authors(i));
    for(auto i = 0; i< task->data().user_data().negative_feedback().basicnegatives_size(); i++)
        params->minorNegativeVotes.insert(task->data().user_data().negative_feedback().basicnegatives(i));
    for(auto i = 0; i< task->data().user_data().negative_feedback().strongnegatives_size(); i++)
        params->majorNegativeVotes.insert(task->data().user_data().negative_feedback().strongnegatives(i));


    QLOG_INFO() << "Dumping received list creation params:";
    params->Log();
    return params;
};


struct RecommendationsSourceFics{
    QSet<int> sourceFics;
    QHash<uint32_t, core::FicWeightPtr> fetchedFics;
};

const static auto ficPackReader = [](RequestContext& reqContext, const auto& task) -> auto {
    RecommendationsSourceFics result;

    for(int i = 0; i < task->data().id_packs().ffn_ids_size(); i++)
    {
        result.sourceFics.insert(task->data().id_packs().ffn_ids(i));
        //recs->recommendationList[task->id_packs().ffn_ids(i)]
    }
    QLOG_INFO() << "source contains fics: " << result.sourceFics.size();
    reqContext.recsData->sourceFics = result.sourceFics;

    TimedAction action("Fic ids conversion",[&](){
        result.fetchedFics = reqContext.dbContext.fanfics->GetFicsForRecCreation();
    });
    action.run();
    return result;
};

grpc::Status FeederService::DiagnosticRecommendationListCreation(grpc::ServerContext *context,
                                                                 const ProtoSpace::DiagnosticRecommendationListCreationRequest *task,
                                                                 ProtoSpace::DiagnosticRecommendationListCreationResponse *response)
{
    Q_UNUSED(context);
    RequestContext reqContext("Diagnostic Reclist Creation",task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;

    reqContext.dbContext.InitFanfics();

    if(task->data().id_packs().ffn_ids_size() == 0)
        return Status::OK;

    An<core::RecCalculator> recCalculator;

    auto recommendationsCreationParams = basicRecommendationsParamReader(reqContext, task);
    auto ficResult = ficPackReader(reqContext, task);
    auto moodData = CalcMoodDistributionForFicList(ficResult.fetchedFics.keys(), recCalculator->holder.genreComposites);

    auto list = recCalculator->GetDiagnosticRecommendationList(ficResult.fetchedFics, recommendationsCreationParams, moodData);
    TimedAction dataPassAction("Passing data: ",[&](){
        auto* targetList = response->mutable_list();

        targetList->set_quadratic_deviation(list.quad);
        targetList->set_ratio_median(list.ratioMedian);
        targetList->set_distance_to_double_sigma(list.sigma2Dist);
        QLOG_INFO() << "passing authors for fics into data structures: " << list.authorsForFics.size();
        auto keys = list.authorsForFics.keys();
        for(auto fic : keys){
            auto* newMatch = targetList->add_matches();
            newMatch->set_fic_id(fic);
            for(auto author : list.authorsForFics.value(fic))
                newMatch->add_author_id(author);
        }
        QLOG_INFO() << "passing author stats into data: " << list.authorData.size();
        for(const auto& author : std::as_const(list.authorData))
        {
            auto* authorData  = targetList->add_author_params();
            authorData->set_author_id(author.id);
            authorData->set_total_matches(author.matches);
            authorData->set_full_list_size(author.fullListSize);
            authorData->set_match_category(static_cast<int>(author.authorMatchCloseness));
            authorData->set_negative_matches(author.negativeMatches);
            authorData->set_list_size_without_ignores(author.sizeAfterIgnore);
            authorData->set_ratio_difference_on_neutral_mood(author.listDiff.neutralDifference.value_or(0));
            authorData->set_ratio_difference_on_touchy_mood(author.listDiff.touchyDifference.value_or(0));
        }
    });
    dataPassAction.run();


    return Status::OK;
}

Status FeederService::RecommendationListCreation(ServerContext* context, const ProtoSpace::RecommendationListCreationRequest* task,
                                                 ProtoSpace::RecommendationListCreationResponse* response)
{

    Q_UNUSED(context);
    RequestContext reqContext("Reclist Creation",task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;


    QLOG_TRACE() << "Verifying request params";
    if(!VerifyRecommendationsRequest(task, response->mutable_response_info()))
    {
        SetRecommedationDataError(response->mutable_response_info());
        QLOG_INFO() << "data size error, exiting";
        return Status::OK;
    }

    reqContext.dbContext.InitFanfics();

    if(task->data().id_packs().ffn_ids_size() == 0)
        return Status::OK;

    auto recommendationsCreationParams = basicRecommendationsParamReader(reqContext, task);


    auto ficResult = ficPackReader(reqContext, task);
    ankerl::nanobench::Bench().minEpochIterations(6).run([&](){
    //recommendationsCreationParams->ficData.sourceFics = ficResult.sourceFics;
    //QLOG_INFO() << "Received source fics: " << ficResult.sourceFics.toList();
    //recommendationsCreationParams->Log();

    An<core::RecCalculator> recCalculator;
    auto moodData = CalcMoodDistributionForFicList(ficResult.fetchedFics.keys(), recCalculator->holder.genreComposites);


    auto list = recCalculator->GetMatchedFicsForFavList(ficResult.fetchedFics, recommendationsCreationParams, moodData);
    int baseVotes = recommendationsCreationParams->useMoodAdjustment ? 100 : 1;

    //TimedAction dataPassAction("Passing data: ",[&](){


        response->Clear();
        auto* targetList = response->mutable_list();
        targetList->set_success(list.success);
        QLOG_INFO() << "Notrash is of size: " <<  list.noTrashScore.size();
        targetList->set_list_name(proto_converters::TS(recommendationsCreationParams->name));
        targetList->set_list_ready(true);
        QLOG_INFO() << "setting list ready to true";
        using core::AuthorWeightingResult;
        typedef core::AuthorWeightingResult::EAuthorType EAuthorType;

        auto dataSize = list.recommendations.size();
        targetList->mutable_fic_matches()->Reserve(dataSize);
        targetList->mutable_breakdowns()->Reserve(dataSize);
        if(!task->data().response_data_controls().ignore_breakdowns())
            targetList->mutable_no_trash_score()->Reserve(dataSize);

        for(auto i = list.recommendations.cbegin(); i != list.recommendations.cend(); i++)
        {
            const auto& key = i.key();
            const auto& value = i.value();
            //QLOG_INFO() << " n_fic_id: " << key << " n_matches: " << list[key];
            if(!recCalculator->holder.fics.contains(key))
            {
                qDebug() << "probably an older database, skipping key: " << key;
                continue;
            }

            int adjustedVotes = value/(baseVotes);
            if(adjustedVotes < 1)
                adjustedVotes = 1;
            // purging based on mood
            if((recommendationsCreationParams->useMoodAdjustment && (value/(baseVotes*list.pureMatches.value(key))) < 1) &&
                    list.decentMatches.value(key) == 0 &&
                    !recommendationsCreationParams->likedAuthors.contains(recCalculator->holder.fics.value(key)->authorId))
            {
                bool axisGenre = false;;
                //qDebug() << "attempting to purge fic: " << key;
                const QHash<int, QList<genre_stats::GenreBit>>& ref = recCalculator->holder.genreComposites;
                const QList<genre_stats::GenreBit>& refList = ref[key];
                double maxValue = 0.;
                // shit code, but I really don't want to refactor rn
                for(const auto& genreBit: refList)
                {
                    if(genreBit.relevance > maxValue)
                        maxValue = genreBit.relevance;
                }

                for(const auto& genreBit: refList)
                {
                    //qDebug() << "genres: " << genreBit.genres << " relevance: " << genreBit.relevance;
                    if(genreBit.relevance/maxValue > 0.45)
                    {
                        for(const auto& actualGenre : std::as_const(genreBit.genres))
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
            if(!task->data().response_data_controls().ignore_breakdowns())
                targetList->add_no_trash_score(list.sumNegativeVotesForFic[key]);

            targetList->add_fic_ids(key);
            //targetList->add_fic_matches(list.recommendations[key]/100);
            //targetList->add_fic_matches(list.recommendations[key]);
            auto* target = targetList->add_breakdowns();
            target->set_id(key);
            if(!task->data().response_data_controls().ignore_breakdowns()){
                target->set_votes_common(list.breakdowns[key].authorTypeVotes[EAuthorType::common]);
                target->set_votes_uncommon(list.breakdowns[key].authorTypeVotes[EAuthorType::uncommon]);
                target->set_votes_rare(list.breakdowns[key].authorTypeVotes[EAuthorType::rare]);
                target->set_votes_unique(list.breakdowns[key].authorTypeVotes[EAuthorType::unique]);

                target->set_counts_common(list.breakdowns[key].authorTypes[EAuthorType::common]);
                target->set_counts_uncommon(list.breakdowns[key].authorTypes[EAuthorType::uncommon]);
                target->set_counts_rare(list.breakdowns[key].authorTypes[EAuthorType::rare]);
                target->set_counts_unique(list.breakdowns[key].authorTypes[EAuthorType::unique]);
            }
        }
        qDebug() << "Match report will contain: " << list.matchReport.size() << " fics";
        for(const auto& author: std::as_const(list.authors))
            response->mutable_list()->add_author_ids(author);

        if(!task->data().response_data_controls().ignore_breakdowns())
            for(auto i = list.matchReport.cbegin(); i != list.matchReport.cend(); i++)
                (*targetList->mutable_match_report())[i.key()] = i.value();

        response->mutable_list()->mutable_used_params()->set_is_automatic(recommendationsCreationParams->isAutomatic);
        response->mutable_list()->mutable_used_params()->set_min_fics_to_match(recommendationsCreationParams->minimumMatch);
        response->mutable_list()->mutable_used_params()->set_max_unmatched_to_one_matched(recommendationsCreationParams->maxUnmatchedPerMatch);
        response->mutable_list()->mutable_used_params()->set_always_pick_at(recommendationsCreationParams->alwaysPickAt);
        response->mutable_list()->mutable_used_params()->set_use_weighting(recommendationsCreationParams->useWeighting);
        response->mutable_list()->mutable_used_params()->set_use_mood_filtering(recommendationsCreationParams->useMoodAdjustment);
        response->mutable_list()->mutable_used_params()->set_use_dislikes(recommendationsCreationParams->useDislikes);
        response->mutable_list()->mutable_used_params()->set_use_dead_fic_ignore(recommendationsCreationParams->useDeadFicIgnore);
        });

//    });
//    dataPassAction.run();
    QLOG_INFO() << "Byte size will be: " << response->ByteSizeLong();
    return Status::OK;
}



Status FeederService::GetDBFicIDS(ServerContext* context, const ProtoSpace::FicIdRequest* task,
                                  ProtoSpace::FicIdResponse* response)
{
    Q_UNUSED(context);
    RequestContext reqContext("FFN fic IDS",task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;

    QLOG_TRACE() << "Verifying request params";
    if(!VerifyIDPack(task->ids(), response->mutable_response_info()))
    {
        QLOG_ERROR() << "failed verifying id pack";
        return Status::OK;
    }

    QHash<int, int> idsToFill;
    for(int i = 0; i < task->ids().ffn_ids_size(); i++)
        idsToFill[task->ids().ffn_ids(i)] = -1;
    reqContext.dbContext.InitFanfics();
    bool result = reqContext.dbContext.fanfics->ConvertFFNTaggedFicsToDB(idsToFill);
    if(!result)
    {
        QLOG_ERROR() << "failed to convert";
        response->set_success(false);
        return Status::OK;
    }
    response->set_success(true);
    QLOG_INFO() << "Succesfully converted ids:" << idsToFill.size();

    for(auto i = idsToFill.begin(); i != idsToFill.end(); i ++)
    {
        //QLOG_INFO() << "Returning fic ids: " << "DB: " << idsToFill[fic] << " FFN: " << fic;
        response->mutable_ids()->add_ffn_ids(i.key());
        response->mutable_ids()->add_db_ids(i.value());
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

    QLOG_TRACE() << "Verifying request params";
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

    for(auto i = idsToFill.begin(); i != idsToFill.end(); i ++)
    {
        //QLOG_INFO() << "Returning fic ids: " << "FFN: " << idsToFill[fic] << " DB: " << fic;
        response->mutable_ids()->add_db_ids(i.key());
        response->mutable_ids()->add_ffn_ids(i.value());
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

    QLOG_TRACE() << "Verifying request params";
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
    core::FavListDetails result;

    core::FicListDataAccumulator dataAccumulator;
    auto genresInterface  = QSharedPointer<interfaces::Genres> (new interfaces::Genres());
    genresInterface->db = reqContext.dbContext.dbInterface->GetDatabase();;
    interfaces::GenreConverter conv;
    An<interfaces::GenreIndex> genreIndex;
    for(const auto& fic: std::as_const(fetchedFics))
    {
        fic->genres = conv.GetFFNGenreList(fic->genreString);
        for(const auto& genre: std::as_const(fic->genres))
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
//    for(auto i = 0; i < 22; i++)
//    {
//        qDebug() << genreIndex->genresByIndex[i].name << " : " << dataAccumulator.genreCounters[i];
//    }
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

    QLOG_TRACE() << "Verifying request params";
    if(!VerifyIDPack(task->id_packs(), response->mutable_response_info()))
        return Status::OK;

    reqContext.dbContext.InitAuthors();

    //QHash<uint32_t, core::FicWeightPtr> fetchedFics;
    auto  sourceFics = ProcessIDPackIntoFfnFicSet(task->id_packs());

    if(sourceFics.size() == 0)
        return Status::OK;

    auto* data = ThreadData::GetUserData();
    data->ficsForAuthorSearch = sourceFics;
    QLOG_INFO() << "Fetching authors";
    auto result = reqContext.dbContext.authors->GetHashAuthorsForFics(sourceFics);

    for(auto i = result.begin(); i != result.end(); i++)
    {
        response->add_fics(i.key());
        response->add_authors(i.value());
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

    QLOG_TRACE() << "Verifying request params";
    if(task->fic_id() < 1)
        return Status::OK;

    reqContext.dbContext.InitAuthors();
    auto allAuthors = reqContext.dbContext.authors->GetRecommendersForFics({task->fic_id()});
    auto recsAuthorsList = QString::fromStdString(task->author_list()).split(",", Qt::SkipEmptyParts);
    QSet<int> result;
    for(const auto& author: std::as_const(recsAuthorsList))
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

void PassTaskIntoSnoozes(const ProtoSpace::SnoozeInfoRequest *task,
                         QHash<int, core::FanficSnoozeStatus>& snoozes){

    for(int i = 0; i < task->snoozes_size(); i++)
    {
        auto ficId = task->snoozes(i).fic_id();
        snoozes[ficId].ficId = ficId;
        snoozes[ficId].untilFinished= task->snoozes(i).until_finished();
        snoozes[ficId].snoozedAtChapter= task->snoozes(i).chapter_added();
        snoozes[ficId].snoozedTillChapter = task->snoozes(i).until_chapter();
    }

}

grpc::Status FeederService::GetExpiredSnoozes(grpc::ServerContext *, const ProtoSpace::SnoozeInfoRequest *task, ProtoSpace::SnoozeInfoResponse *response)
{
    if(task->snoozes_size() > 10000)
    {
        response->mutable_response_info()->set_data_size_limit_reached(true);
        response->mutable_response_info()->set_error("You can only have 10000 active snoozes");
        return Status::OK;
    }

    RequestContext reqContext("Snooze refresh", task->controls(), this);
    if(!reqContext.Process(response->mutable_response_info()))
        return Status::OK;

    reqContext.dbContext.InitFanfics();
    QHash<int, core::FanficSnoozeStatus> snoozes;
    PassTaskIntoSnoozes(task, snoozes);



    auto* userThreadData = ThreadData::GetUserData();

    for(auto i = snoozes.begin();i != snoozes.end(); i++)
        userThreadData->ficsForSelection.insert(i.key());

    qDebug() << "Selection is: " << userThreadData->ficsForSelection;

    auto snoozeInfo = reqContext.dbContext.fanfics->GetSnoozeInfo();
    for(auto snoozeData : snoozeInfo)
    {
        if(snoozeData.finished || (snoozeData.atChapter >= snoozes[snoozeData.ficId].snoozedTillChapter))
        {
            qDebug() << "is finished: " << snoozeData.finished << " at chapter: " << snoozeData.atChapter << " testing against user: " << snoozes[snoozeData.ficId].snoozedTillChapter;
            qDebug() << "adding expired snooze: " << snoozeData.ficId;
            response->add_expired_snoozes(snoozeData.ficId);
        }
    }
    response->set_success(true);
    return Status::OK;
}

void FeederService::AddToStatistics(QString uuid, const core::StoryFilter& filter){
    StatisticsToken token;
    {
        QWriteLocker locker(&lock);
        //StatisticsToken token = tokenData.value(uuid);
        searchedTokens.insert(uuid);
        allTokens.insert(uuid);
        allSearches++;
    }
    if(filter.sortMode == core::StoryFilter::sm_metascore ||
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
    //QList<QString> toErase;
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
    recs->scoresList = filter.scoresHash;

    return filter;
}

QSharedPointer<FicSource> FeederService::InitFicSource(QString userToken,
                                                       QSharedPointer<database::IDBWrapper> dbInterface)
{
    //DatabaseContext dbContext;
    QSharedPointer<FicSource> ficSource(new FicSourceDirect(dbInterface,rngData));
    auto* convertedFicSource = dynamic_cast<FicSourceDirect*>(ficSource.data());
    QLOG_TRACE() << "Initializing fic source mode";
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
    applicationToken = QString::fromStdString(control.application_token());
    if(applicationToken.isEmpty())
        QLOG_INFO() << "Received request: " << requestName << " from: " << applicationToken;
    else
        QLOG_INFO() << "Received request: " << requestName << " from: " << userToken;
    if(applicationToken != userToken && !applicationToken.isEmpty())
        QLOG_INFO() << "Discord user: " << userToken;

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

void AccumulatorIntoSectionStats(core::FavListDetails& result, const core::FicListDataAccumulator& dataResult)
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
        const auto& genre =  qAsConst(genreIndex->genresByIndex)[i];
        result.genreFactors[genre.name] = dataResult.result.genreRatios[i];
    }

    for(auto i = dataResult.result.fandomRatios.begin(); i != dataResult.result.fandomRatios.end(); i++)
        result.fandomFactorsConverted[i.key()] = i.value();

    for(auto i = dataResult.fandomCounters.begin(); i != dataResult.fandomCounters.end(); i++)
        result.fandomsConverted[i.key()] = i.value();

    for(size_t i = 1; i < dataResult.result.sizeRatios.size(); i++)
        result.sizeFactors[i] = dataResult.result.sizeRatios[i];
}

