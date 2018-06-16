#include "grpc/grpc_source.h"
#include "proto/filter.pb.h"
#include <memory>

FicSourceGRPC::FicSourceGRPC()
{

}

FicSourceGRPC::~FicSourceGRPC()
{

}

static Filter StoryFilterIntoSlashFlter(core::StoryFilter filter)
{
    // ignore fandoms intentionally not passed because likely use case can be done locally

    Filter result;
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

    result.set_filtering_mode(static_cast<Filter::FilterMode>(filter.mode));
    result.set_sort_mode(static_cast<Filter::SortMode>(filter.sortMode));

    auto* sizeLimits = result.mutable_size_limits();
    sizeLimits->set_record_limit(filter.recordLimit);
    sizeLimits->set_record_page(filter.recordPage);

    auto* reviewBias = result.mutable_review_bias();
    reviewBias->set_bias_mode(static_cast<ReviewBias::ReviewBiasMode>(filter.reviewBias));
    reviewBias->set_bias_operator(static_cast<ReviewBias::BiasOperator>(filter.biasOperator));
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


void FicSourceGRPC::FetchData(core::StoryFilter filter, QList<core::Fic> *)
{

}

int FicSourceGRPC::GetFicCount(core::StoryFilter filter)
{
    return 0;
}

class FicSourceGRPCImpl{
    //    friend class GrpcServiceBase;
    //public:
    //    FicSourceGRPCImpl(QString connectionString, int deadline)
    //        {
    //        stub_.reset(Services::Feeder::NewStub(wrapper.channel));
    //        timeout = deadline;
    //    }
    //    bool HasErrors() const { return hasErrors; }
    //    void SetServiceName(QString);
    //    std::unique_ptr<Services::Feeder::Stub> stub_;
    //    QString requestString;
    //    QString responseString;
    //    int timeout = 3000;

    //private:
    //    QString connectionString;
    //   bool hasErrors = false;
    //    QString error;
    //    bool isResponsive = true;
};
