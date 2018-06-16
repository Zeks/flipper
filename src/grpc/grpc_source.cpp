#include "include/grpc/grpc_source.h"
#include "include/url_utils.h"
#include "proto/filter.pb.h"
#include "proto/fanfic.pb.h"
#include "proto/feeder_service.pb.h"
#include "proto/feeder_service.grpc.pb.h"
#include <memory>
#include <QList>
#include <QVector>

#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>


FicSourceGRPC::FicSourceGRPC()
{

}

FicSourceGRPC::~FicSourceGRPC()
{

}


static inline int GetChapterForFic(int ficId){
    return 0;
}

static inline QList<int> GetTaggedIDs(){
    return QList<int>();
}

static ProtoSpace::Filter StoryFilterIntoProtoFlter(core::StoryFilter filter)
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

    return result;
}

static inline QString FS(const std::string& s)
{
    return QString::fromStdString(s);
}

static inline QDateTime DFS(const std::string& s)
{
    return QDateTime::fromString(QString::fromStdString(s), "yyyyMMdd");
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

    for(int i = 0; i < protoFic.fandoms_size(); i++)
        coreFic.fandoms.push_back(FS(protoFic.fandoms(i)));
    coreFic.isCrossover = coreFic.fandoms.size() > 1;

    for(int i = 0; i < protoFic.genres_size(); i++)
        coreFic.genres.push_back(FS(protoFic.genres(i)));
    coreFic.genreString = coreFic.genres.join(" ");
    coreFic.webId = protoFic.site_pack().ffn().id(); // temporary
    coreFic.ffn_id = coreFic.webId; // temporary
    coreFic.webSite = "ffn"; // temporary

    coreFic.urls["ffn"] = url_utils::GetStoryUrlFromWebId(coreFic.webId, "ffn"); // temporary
    coreFic.urlFFN = coreFic.urls["ffn"];
    return true;
}

void FicSourceGRPC::FetchData(core::StoryFilter filter, QList<core::Fic> *)
{

}

int FicSourceGRPC::GetFicCount(core::StoryFilter filter)
{
    return 0;
}


class FicSourceGRPCImpl{
    //    friend class GrpcServiceBase;
public:

    FicSourceGRPCImpl(QString connectionString, int deadline)
        :stub_(ProtoSpace::Feeder::NewStub(grpc::CreateChannel(connectionString.toStdString(), grpc::InsecureChannelCredentials()))),
          deadline(deadline)
    {
    }
    void ProcessStandardError(grpc::Status status);
    void FetchData(core::StoryFilter filter, std::vector<core::Fic> * fics)
    {
        grpc::ClientContext context;

        ProtoSpace::SearchTask task;

        ProtoSpace::Filter protoFilter = StoryFilterIntoProtoFlter(filter);
        task.set_allocated_filter(&protoFilter);

        QScopedPointer<ProtoSpace::SearchResponse> response (new ProtoSpace::SearchResponse);
        std::chrono::system_clock::time_point deadline =
                std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
        context.set_deadline(deadline);

        grpc::Status status = stub_->Search(&context, task, response.data());

        ProcessStandardError(status);

        fics->resize(static_cast<size_t>(response->fanfics_size()));
        for(int i = 0; i < response->fanfics_size(); i++)
        {
            ProtoFicToLocalFic(response->fanfics(i), (*fics)[static_cast<size_t>(i)]);
        }
    }

    std::unique_ptr<ProtoSpace::Feeder::Stub> stub_;
    QString error;
    bool hasErrors = false;
    int deadline = 60;
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
