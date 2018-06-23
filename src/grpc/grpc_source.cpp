#include "include/grpc/grpc_source.h"
#include "include/url_utils.h"
#include "include/Interfaces/fandoms.h"
#include "proto/filter.pb.h"
#include "proto/fanfic.pb.h"
#include "proto/fandom.pb.h"
#include "proto/feeder_service.pb.h"
#include "proto/feeder_service.grpc.pb.h"
#include <memory>
#include <QList>
#include <QUuid>
#include <QVector>

#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>





static inline int GetChapterForFic(int ficId){
    return 0;
}

static inline QList<int> GetTaggedIDs(){
    return QList<int>();
}

namespace proto_converters
{
QString FS(const std::string& s)
{
    return QString::fromStdString(s);
}

std::string TS(const QString& s)
{
    return s.toStdString();
}


QDateTime DFS(const std::string& s)
{
    return QDateTime::fromString(QString::fromStdString(s), "yyyyMMdd");
}

std::string DTS(const QDateTime & date)
{
    return date.toString("yyyyMMdd").toStdString();
}



ProtoSpace::Filter StoryFilterIntoProtoFilter(const core::StoryFilter& filter)
{
    // ignore fandoms intentionally not passed because likely use case can be done locally

    ProtoSpace::Filter result;
    result.set_randomize_results(filter.randomizeResults);
    auto* basicFilters = result.mutable_basic_filters();
    basicFilters->set_website(filter.website.toStdString());
    basicFilters->set_min_words(filter.minWords);
    basicFilters->set_max_words(filter.maxWords);

    basicFilters->set_min_favourites(filter.minFavourites);
    basicFilters->set_max_fics(filter.maxFics);

    basicFilters->set_allow_unfinished(filter.allowUnfinished);
    basicFilters->set_allow_no_genre(filter.allowNoGenre);

    basicFilters->set_ensure_active(filter.ensureActive);
    basicFilters->set_ensure_completed(filter.ensureCompleted);

    result.set_filtering_mode(static_cast<ProtoSpace::Filter::FilterMode>(filter.mode));
    result.set_sort_mode(static_cast<ProtoSpace::Filter::SortMode>(filter.sortMode));

    auto* sizeLimits = result.mutable_size_limits();
    sizeLimits->set_record_limit(filter.recordLimit);
    sizeLimits->set_record_page(filter.recordPage);

    auto* reviewBias = result.mutable_review_bias();
    reviewBias->set_bias_mode(static_cast<ProtoSpace::ReviewBias::ReviewBiasMode>(filter.reviewBias));
    reviewBias->set_bias_operator(static_cast<ProtoSpace::ReviewBias::BiasOperator>(filter.biasOperator));
    reviewBias->set_bias_ratio(filter.reviewBiasRatio);

    auto* recentPopular = result.mutable_recent_and_popular();
    recentPopular->set_fav_ratio(filter.recentAndPopularFavRatio);
    recentPopular->set_date_cutoff(filter.recentCutoff.toString("yyyyMMdd").toStdString());


    auto* contentFilter = result.mutable_content_filter();
    contentFilter->set_fandom(filter.fandom.toStdString());
    contentFilter->set_include_crossovers(filter.includeCrossovers);
    contentFilter->set_crossovers_only(filter.crossoversOnly);
    contentFilter->set_other_fandoms_mode(filter.otherFandomsMode);
    contentFilter->set_use_ignored_fandoms(filter.ignoreFandoms);


    for(auto genre : filter.genreExclusion)
        contentFilter->add_genre_exclusion(genre.toStdString());
    for(auto genre : filter.genreInclusion)
        contentFilter->add_genre_inclusion(genre.toStdString());
    for(auto word : filter.wordExclusion)
        contentFilter->add_word_exclusion(word.toStdString());
    for(auto word : filter.wordInclusion)
        contentFilter->add_word_inclusion(word.toStdString());



    auto* tagFilter = result.mutable_tag_filter();
    tagFilter->set_ignore_already_tagged(filter.ignoreAlreadyTagged);
    for(auto tag : filter.activeTags)
        tagFilter->add_active_tags(tag.toStdString());

    auto allTagged = GetTaggedIDs();

    for(auto id : allTagged)
        tagFilter->add_all_tagged(id);

    auto* slashFilter = result.mutable_slash_filter();
    slashFilter->set_use_slash_filter(filter.slashFilter.slashFilterEnabled);
    slashFilter->set_exclude_slash(filter.slashFilter.excludeSlash);
    slashFilter->set_include_slash(filter.slashFilter.includeSlash);
    slashFilter->set_enable_exceptions(filter.slashFilter.enableFandomExceptions);
    for(auto exception : filter.slashFilter.fandomExceptions)
        slashFilter->add_fandom_exceptions(exception);


    auto* recommendations = result.mutable_recommendations();
    recommendations->set_list_open_mode(filter.listOpenMode);
    recommendations->set_list_for_recommendations(filter.listForRecommendations);
    recommendations->set_use_this_recommender_only(filter.useThisRecommenderOnly);
    recommendations->set_min_recommendations(filter.minRecommendations);
    recommendations->set_show_origins_in_lists(filter.showOriginsInLists);
    for(auto fic : filter.recsHash.keys())
    {
        recommendations->add_list_of_fics(fic);
        recommendations->add_list_of_matches(filter.recsHash[fic]);
    }
    return result;
}


void LocalFandomToProtoFandom(const core::Fandom& coreFandom, ProtoSpace::Fandom* protoFandom)
{
    protoFandom->set_id(coreFandom.id);
    protoFandom->set_name(TS(coreFandom.GetName()));
    protoFandom->set_website("ffn");
}

void ProtoFandomToLocalFandom(const ProtoSpace::Fandom& protoFandom, core::Fandom& coreFandom)
{
    coreFandom.id = protoFandom.id();
    coreFandom.SetName(FS(protoFandom.name()));
}


core::StoryFilter ProtoFilterIntoStoryFilter(const ProtoSpace::Filter& filter)
{
    // ignore fandoms intentionally not passed because likely use case can be done locally

    core::StoryFilter result;
    result.randomizeResults = filter.randomize_results();
    result.website = FS(filter.basic_filters().website());
    result.minWords = filter.basic_filters().min_words();
    result.maxWords = filter.basic_filters().max_words();

    result.minFavourites = filter.basic_filters().min_favourites();
    result.maxFics = filter.basic_filters().max_fics();


    result.allowUnfinished = filter.basic_filters().allow_unfinished();
    result.allowNoGenre = filter.basic_filters().allow_no_genre();

    result.ensureActive = filter.basic_filters().ensure_active();
    result.ensureCompleted = filter.basic_filters().ensure_completed();

    result.mode = static_cast<core::StoryFilter::EFilterMode>(filter.filtering_mode());
    result.sortMode = static_cast<core::StoryFilter::ESortMode>(filter.sort_mode());

    result.recordLimit = filter.size_limits().record_limit();
    result.recordPage = filter.size_limits().record_page();

    result.reviewBias = static_cast<core::StoryFilter::EReviewBiasMode>(filter.review_bias().bias_mode());
    result.biasOperator = static_cast<core::StoryFilter::EBiasOperator>(filter.review_bias().bias_operator());
    result.reviewBiasRatio = filter.review_bias().bias_ratio();

    result.recentAndPopularFavRatio = filter.recent_and_popular().fav_ratio();
    result.recentCutoff = DFS(filter.recent_and_popular().date_cutoff());

    result.fandom = FS(filter.content_filter().fandom());
    result.includeCrossovers = filter.content_filter().include_crossovers();
    result.crossoversOnly = filter.content_filter().crossovers_only();
    result.otherFandomsMode = filter.content_filter().other_fandoms_mode();
    result.ignoreFandoms = filter.content_filter().use_ignored_fandoms();

    for(int i = 0; i < filter.content_filter().genre_exclusion_size(); i++)
        result.genreExclusion.push_back(FS(filter.content_filter().genre_exclusion(i)));
    for(int i = 0; i < filter.content_filter().genre_inclusion_size(); i++)
        result.genreInclusion.push_back(FS(filter.content_filter().genre_inclusion(i)));
    for(int i = 0; i < filter.content_filter().word_exclusion_size(); i++)
        result.wordExclusion.push_back(FS(filter.content_filter().word_exclusion(i)));
    for(int i = 0; i < filter.content_filter().word_inclusion_size(); i++)
        result.wordInclusion.push_back(FS(filter.content_filter().word_inclusion(i)));

    result.ignoreAlreadyTagged = filter.tag_filter().ignore_already_tagged();
    for(int i = 0; i < filter.tag_filter().active_tags_size(); i++)
        result.activeTags.push_back(FS(filter.tag_filter().active_tags(i)));

    for(int i = 0; i < filter.tag_filter().all_tagged_size(); i++)
        result.taggedIDs.push_back(filter.tag_filter().all_tagged(i));

    for(int i = 0; i < filter.recommendations().list_of_fics_size(); i++)
        result.recsHash[filter.recommendations().list_of_fics(i)] = filter.recommendations().list_of_matches(i);


    result.slashFilter.slashFilterEnabled = filter.slash_filter().use_slash_filter();
    result.slashFilter.excludeSlash = filter.slash_filter().exclude_slash();
    result.slashFilter.includeSlash = filter.slash_filter().include_slash();
    result.slashFilter.enableFandomExceptions = filter.slash_filter().enable_exceptions();

    for(int i = 0; i < filter.slash_filter().fandom_exceptions_size(); i++)
        result.slashFilter.fandomExceptions.push_back(filter.slash_filter().fandom_exceptions(i));

    result.listOpenMode = filter.recommendations().list_open_mode();
    result.listForRecommendations = filter.recommendations().list_open_mode();
    result.useThisRecommenderOnly = filter.recommendations().use_this_recommender_only();
    result.minRecommendations = filter.recommendations().min_recommendations();
    result.showOriginsInLists = filter.recommendations().show_origins_in_lists();

    return result;
}




bool ProtoFicToLocalFic(const ProtoSpace::Fanfic& protoFic, core::Fic& coreFic)
{
    coreFic.isValid = protoFic.is_valid();
    if(!coreFic.isValid)
        return false;

    coreFic.id = protoFic.id();

    // I will probably disable this for now in the ui
    coreFic.atChapter = GetChapterForFic(coreFic.id);

    coreFic.complete = protoFic.complete();
    coreFic.recommendations = protoFic.recommendations();
    coreFic.wordCount = FS(protoFic.word_count());
    coreFic.chapters = FS(protoFic.chapters());
    coreFic.reviews = FS(protoFic.reviews());
    coreFic.favourites = FS(protoFic.favourites());
    coreFic.follows = FS(protoFic.follows());
    coreFic.rated = FS(protoFic.rated());
    coreFic.fandom = FS(protoFic.fandom());
    coreFic.title = FS(protoFic.title());
    coreFic.summary = FS(protoFic.summary());
    coreFic.language = FS(protoFic.language());
    coreFic.published = DFS(protoFic.published());
    coreFic.updated = DFS(protoFic.updated());
    coreFic.charactersFull = FS(protoFic.characters());

    coreFic.author = core::Author::NewAuthor();
    coreFic.author->name = FS(protoFic.author());

    for(int i = 0; i < protoFic.fandoms_size(); i++)
        coreFic.fandoms.push_back(FS(protoFic.fandoms(i)));
    for(int i = 0; i < protoFic.fandom_ids_size(); i++)
        coreFic.fandomIds.push_back(protoFic.fandom_ids(i));
    coreFic.isCrossover = coreFic.fandoms.size() > 1;

    coreFic.genreString = FS(protoFic.genres());
    coreFic.webId = protoFic.site_pack().ffn().id(); // temporary
    coreFic.ffn_id = coreFic.webId; // temporary
    coreFic.webSite = "ffn"; // temporary

    coreFic.urls["ffn"] = QString::number(coreFic.webId); // temporary
    coreFic.urlFFN = coreFic.urls["ffn"];
    return true;
}

bool LocalFicToProtoFic(const core::Fic& coreFic, ProtoSpace::Fanfic* protoFic)
{
    protoFic->set_is_valid(true);
    protoFic->set_id(coreFic.id);

    protoFic->set_chapters(TS(coreFic.chapters));
    protoFic->set_complete(coreFic.complete);
    protoFic->set_recommendations(coreFic.recommendations);

    protoFic->set_word_count(TS(coreFic.wordCount));
    protoFic->set_reviews(TS(coreFic.reviews));
    protoFic->set_favourites(TS(coreFic.favourites));
    protoFic->set_follows(TS(coreFic.follows));
    protoFic->set_rated(TS(coreFic.rated));
    protoFic->set_fandom(TS(coreFic.fandom));
    protoFic->set_title(TS(coreFic.title));
    protoFic->set_summary(TS(coreFic.summary));
    protoFic->set_language(TS(coreFic.language));
    protoFic->set_genres(TS(coreFic.genreString));
    protoFic->set_author(TS(coreFic.author->name));

    protoFic->set_published(DTS(coreFic.published));
    protoFic->set_updated(DTS(coreFic.updated));
    protoFic->set_characters(TS(coreFic.charactersFull));


    for(auto fandom : coreFic.fandoms)
        protoFic->add_fandoms(TS(fandom));
    for(auto fandom : coreFic.fandomIds)
        protoFic->add_fandom_ids(fandom);


    protoFic->mutable_site_pack()->mutable_ffn()->set_id(coreFic.ffn_id);

    return true;
}
}

class FicSourceGRPCImpl{
    //    friend class GrpcServiceBase;
public:

    FicSourceGRPCImpl(QString connectionString, int deadline)
        :stub_(ProtoSpace::Feeder::NewStub(grpc::CreateChannel(connectionString.toStdString(), grpc::InsecureChannelCredentials()))),
          deadline(deadline)
    {
    }
    UserData userData;

    void ProcessStandardError(grpc::Status status);
    void FetchData(core::StoryFilter filter, QVector<core::Fic> * fics)
    {
        grpc::ClientContext context;

        ProtoSpace::SearchTask task;

        ProtoSpace::Filter protoFilter = proto_converters::StoryFilterIntoProtoFilter(filter);
        task.set_allocated_filter(&protoFilter);

        QScopedPointer<ProtoSpace::SearchResponse> response (new ProtoSpace::SearchResponse);
        std::chrono::system_clock::time_point deadline =
                std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
        context.set_deadline(deadline);
        auto* controls = task.mutable_controls();
        controls->set_user_token(proto_converters::TS(userToken));

        auto* userData = task.mutable_user_data();
        auto* tags = userData->mutable_user_tags();

        for(auto tag : this->userData.allTags)
            tags->add_all_tags(tag);
        for(auto tag : this->userData.activeTags)
            tags->add_searched_tags(tag);

        auto* ignoredFandoms = userData->mutable_ignored_fandoms();
        for(auto key: this->userData.ignoredFandoms.keys())
        {
            ignoredFandoms->add_fandom_ids(key);
            ignoredFandoms->add_ignore_crossovers(this->userData.ignoredFandoms[key]);
        }




        grpc::Status status = stub_->Search(&context, task, response.data());

        ProcessStandardError(status);

        fics->resize(static_cast<size_t>(response->fanfics_size()));
        for(int i = 0; i < response->fanfics_size(); i++)
        {
            proto_converters::ProtoFicToLocalFic(response->fanfics(i), (*fics)[static_cast<size_t>(i)]);
        }

        task.release_filter();
    }

    int GetFicCount(core::StoryFilter filter)
    {
        grpc::ClientContext context;

        ProtoSpace::FicCountTask task;

        ProtoSpace::Filter protoFilter = proto_converters::StoryFilterIntoProtoFilter(filter);
        task.set_allocated_filter(&protoFilter);
        auto* controls = task.mutable_controls();
        controls->set_user_token(proto_converters::TS(userToken));

        QScopedPointer<ProtoSpace::FicCountResponse> response (new ProtoSpace::FicCountResponse);
        std::chrono::system_clock::time_point deadline =
                std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
        context.set_deadline(deadline);

        grpc::Status status = stub_->GetFicCount(&context, task, response.data());

        ProcessStandardError(status);
        task.release_filter();
        return response->fic_count();
    }

    bool GetFandomListFromServer(int lastFandomID, QVector<core::Fandom>* fandoms)
    {
        grpc::ClientContext context;

        ProtoSpace::SyncFandomListTask task;
        task.set_last_fandom_id(lastFandomID);

        QScopedPointer<ProtoSpace::SyncFandomListResponse> response (new ProtoSpace::SyncFandomListResponse);
        std::chrono::system_clock::time_point deadline =
                std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
        context.set_deadline(deadline);

        grpc::Status status = stub_->SyncFandomList(&context, task, response.data());

        ProcessStandardError(status);
        if(!response->needs_update())
            return false;

        fandoms->resize(response->fandoms_size());
        for(int i = 0; i < response->fandoms_size(); i++)
        {
            proto_converters::ProtoFandomToLocalFandom(response->fandoms(i), (*fandoms)[static_cast<size_t>(i)]);
        }
        return true;
    }

    bool GetRecommendationListFromServer(RecommendationListGRPC& recList)
    {
        grpc::ClientContext context;

        ProtoSpace::RecommendationListCreationRequest task;


        QScopedPointer<ProtoSpace::RecommendationListCreationResponse> response (new ProtoSpace::RecommendationListCreationResponse);
        std::chrono::system_clock::time_point deadline =
                std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
        context.set_deadline(deadline);
        auto* ffn = task.mutable_id_packs();
        for(auto fic: recList.fics)
            ffn->add_ffn_ids(fic);
        task.set_list_name(proto_converters::TS(recList.listParams.name));
        task.set_always_pick_at(recList.listParams.alwaysPickAt);
        task.set_min_fics_to_match(recList.listParams.minimumMatch);
        task.set_max_unmatched_to_one_matched(recList.listParams.pickRatio);
        auto* controls = task.mutable_controls();
        controls->set_user_token(proto_converters::TS(userToken));

        grpc::Status status = stub_->RecommendationListCreation(&context, task, response.data());

        ProcessStandardError(status);
        if(!response->list().list_ready())
            return false;

        recList.fics.clear();
        recList.fics.reserve(response->list().fic_ids_size());
        recList.matchCounts.reserve(response->list().fic_ids_size());
        for(int i = 0; i < response->list().fic_ids_size(); i++)
        {
            recList.fics.push_back(response->list().fic_ids(i));
            recList.matchCounts.push_back(response->list().fic_matches(i));
        }
        return true;
    }
    std::unique_ptr<ProtoSpace::Feeder::Stub> stub_;
    QString error;
    bool hasErrors = false;
    int deadline = 60;
    QString userToken;
};

void FicSourceGRPCImpl::ProcessStandardError(grpc::Status status)
{
    error.clear();
    if(status.ok())
    {
        hasErrors = false;
    }
    hasErrors = true;
    switch(status.error_code())
    {
    case 1:
        error+="Client application cancelled the request\n";
        break;
    case 4:
        error+="Deadline for request Exceeded\n";
        break;
    case 12:
        error+="Requested method is not implemented on the server\n";
        break;
    case 14:
        error+="Server is shutting down or is unavailable\n";
        break;
    case 2:
        error+="Server has thrown an unknown exception\n";
        break;
    case 8:
        error+="Server out of resources, or no memory on client\n";
        break;
    case 13:
        error+="Flow control violation\n";
        break;
    default:
        //intentionally empty
        break;
    }
    error+=QString::fromStdString(status.error_message());
}

FicSourceGRPC::FicSourceGRPC(QString connectionString,
                             QString userToken,
                             int deadline): impl(new FicSourceGRPCImpl(connectionString, deadline))
{
    impl->userToken = userToken;
}

FicSourceGRPC::~FicSourceGRPC()
{

}
void FicSourceGRPC::FetchData(core::StoryFilter filter, QVector<core::Fic> *fics)
{
    if(!impl)
        return;
    impl->userData = userData;
    impl->FetchData(filter, fics);
}

int FicSourceGRPC::GetFicCount(core::StoryFilter filter)
{
    if(!impl)
        return 0;
    return impl->GetFicCount(filter);
}

bool FicSourceGRPC::GetFandomListFromServer(int lastFandomID, QVector<core::Fandom> *fandoms)
{
    if(!impl)
        return false;
    return impl->GetFandomListFromServer(lastFandomID, fandoms);
}

bool FicSourceGRPC::GetRecommendationListFromServer(RecommendationListGRPC &recList)
{
    if(!impl)
        return false;
    return impl->GetRecommendationListFromServer(recList);
}
