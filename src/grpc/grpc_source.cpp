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
#include "include/grpc/grpc_source.h"
#include "include/url_utils.h"
#include "include/Interfaces/fandoms.h"
#include "include/tokenkeeper.h"
#include "include/servers/token_processing.h"
#include "proto/search/filter.pb.h"
#include "proto/search/fanfic.pb.h"
#include "proto/search/fandom.pb.h"
#include "proto/feeder_service.pb.h"
#include "proto/feeder_service.grpc.pb.h"
#include <memory>
#include <QList>
#include <QUuid>
#include <QVector>
#include <optional>

#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include "google/protobuf/util/time_util.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/empty.pb.h"

void DumpToLog(QString precede, const ::google::protobuf::Message* message)
{
    std::string asText;
    ::google::protobuf::TextFormat::Printer printer;
    printer.SetUseUtf8StringEscaping(true);
    printer.PrintToString(*message, &asText);
    QLOG_INFO() << precede << QString::fromStdString(asText);
}


static inline int GetChapterForFic(int ){
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
    return QDateTime::fromString(QString::fromStdString(s), QStringLiteral("yyyyMMdd"));
}

std::string DTS(const QDateTime & date)
{
    return date.toString(QStringLiteral("yyyyMMdd")).toStdString();
}



ProtoSpace::Filter StoryFilterIntoProto(const core::StoryFilter& filter,
                                        ProtoSpace::UserData* userData)
{
    ProtoSpace::Filter result;

    ProtoSpace::Filter::ESortDirection sortDirection = static_cast<ProtoSpace::Filter::ESortDirection>(static_cast<int>(filter.descendingDirection));
    result.set_sort_direction(sortDirection);

    auto* basicFilters = result.mutable_basic_filters();
    basicFilters->set_website(filter.website.toStdString());
    basicFilters->set_min_words(filter.minWords);
    basicFilters->set_max_words(filter.maxWords);

    basicFilters->set_min_favourites(filter.minFavourites);
    basicFilters->set_max_fics(filter.maxFics);

    basicFilters->set_allow_unfinished(filter.allowUnfinished);


    basicFilters->set_ensure_active(filter.ensureActive);
    basicFilters->set_last_active_days(filter.deadFicDaysRange);
    basicFilters->set_ensure_completed(filter.ensureCompleted);

    result.set_filtering_mode(static_cast<ProtoSpace::Filter::EFilterMode>(filter.mode));
    result.set_sort_mode(static_cast<ProtoSpace::Filter::ESortMode>(filter.sortMode));
    result.set_rating(static_cast<ProtoSpace::Filter::ERatingFilter>(filter.rating));

    auto* sourcesFilter = result.mutable_sources();
    sourcesFilter->add_authors(filter.useThisAuthor);
    for(auto author : filter.usedRecommenders)
        sourcesFilter->add_recommender_ids(author);

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

    auto* snoozeFilter = result.mutable_snoozes();
    snoozeFilter ->set_display_all(filter.displaySnoozedFics);

    auto* randomizer = result.mutable_randomizer();
    randomizer->set_randomize_results(filter.randomizeResults);
    randomizer->set_sequence_name(filter.rngDisambiguator.toStdString());
    randomizer->set_rebuild_random_sequence(filter.wipeRngSequence);

    auto* contentFilter = result.mutable_content_filter();
    contentFilter->set_fandom(filter.fandom);
    contentFilter->set_crossover_fandom(filter.secondFandom);
    contentFilter->set_include_crossovers(filter.includeCrossovers);
    contentFilter->set_crossovers_only(filter.crossoversOnly);
    contentFilter->set_other_fandoms_mode(filter.otherFandomsMode);
    contentFilter->set_use_ignored_fandoms(filter.ignoreFandoms);

    for(const auto& word : filter.wordExclusion)
        contentFilter->add_word_exclusion(word.toStdString());
    for(const auto& word : filter.wordInclusion)
        contentFilter->add_word_inclusion(word.toStdString());


    auto* genreFilter = result.mutable_genre_filter();

    for(const auto& genre : filter.genreExclusion)
        genreFilter->add_genre_exclusion(genre.toStdString());
    for(const auto& genre : filter.genreInclusion)
        genreFilter->add_genre_inclusion(genre.toStdString());
    genreFilter->set_use_implied_genre(filter.useRealGenres);

    genreFilter->set_allow_no_genre(filter.allowNoGenre);
    genreFilter->set_genre_presence_include(static_cast<ProtoSpace::GenreFilter::GenrePresence>(filter.genrePresenceForInclude));
    genreFilter->set_genre_presence_exclude(static_cast<ProtoSpace::GenreFilter::GenrePresence>(filter.genrePresenceForExclude));

    auto* tagFilter = result.mutable_tag_filter();
    tagFilter->set_ignore_already_tagged(filter.ignoreAlreadyTagged);
    tagFilter->set_tags_are_for_authors(filter.tagsAreUsedForAuthors);
    tagFilter->set_use_and_for_tags(filter.tagsAreANDed);

    auto* explicitFilter = result.mutable_explicit_filter();
    auto* slashFilter = explicitFilter->mutable_slash();

    slashFilter->set_use_filter(filter.slashFilter.slashFilterEnabled);
    slashFilter->set_exclude_content(filter.slashFilter.excludeSlash);
    slashFilter->set_include_content(filter.slashFilter.includeSlash);
    slashFilter->set_filter_level(filter.slashFilter.slashFilterLevel);
    slashFilter->set_enable_exceptions(filter.slashFilter.enableFandomExceptions);
    slashFilter->set_show_exact_level(filter.slashFilter.onlyExactLevel);
    slashFilter->set_filter_only_mature(filter.slashFilter.onlyMatureForSlash);

    for(auto exception : filter.slashFilter.fandomExceptions)
        slashFilter->add_fandom_exceptions(exception);

    auto* recommendations = result.mutable_recommendations();
    recommendations->set_list_open_mode(filter.listOpenMode);
    recommendations->set_list_for_recommendations(filter.listForRecommendations);
    recommendations->set_use_this_recommender_only(filter.useThisRecommenderOnly);
    recommendations->set_min_recommendations(filter.minRecommendations);
    recommendations->set_show_origins_in_lists(filter.showOriginsInLists);
    recommendations->set_display_purged_fics(filter.displayPurgedFics);
    {
        auto it= filter.recsHash.cbegin();
        auto itEnd = filter.recsHash.cend();
        while(it != itEnd)
        {
            userData->mutable_recommendation_list()->add_list_of_fics(it.key());
            userData->mutable_recommendation_list()->add_list_of_matches(it.value());
            it++;
        }
    }
    {
        auto it= filter.scoresHash.cbegin();
        auto itEnd = filter.scoresHash.cend();
        while(it != itEnd)
        {
            userData->mutable_scores_list()->add_list_of_fics(it.key());
            userData->mutable_scores_list()->add_list_of_scores(it.value());
            it++;
        }
    }
    //auto* fandomState = result.mutable_();
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




core::StoryFilter ProtoIntoStoryFilter(const ProtoSpace::Filter& filter, const ProtoSpace::UserData& userData)
{
    // ignore fandoms intentionally not passed because likely use case can be done locally

    core::StoryFilter result;
    result.randomizeResults = filter.randomizer().randomize_results();
    result.rngDisambiguator = QString::fromStdString(filter.randomizer().sequence_name());
    result.wipeRngSequence = filter.randomizer().rebuild_random_sequence();
    result.useRealGenres = filter.genre_filter().use_implied_genre();
    result.displayPurgedFics = filter.recommendations().display_purged_fics();
    if(filter.sort_direction() == ::ProtoSpace::Filter_ESortDirection::Filter_ESortDirection_sd_ascending)
    {
        QLOG_INFO() << "Is ascending direction";
        result.descendingDirection = false;
    }
    else
    {
        QLOG_INFO() << "Is descending direction";
        result.descendingDirection = true;
    }
    result.website = FS(filter.basic_filters().website());
    result.minWords = filter.basic_filters().min_words();
    result.maxWords = filter.basic_filters().max_words();

    result.minFavourites = filter.basic_filters().min_favourites();
    result.maxFics = filter.basic_filters().max_fics();


    result.allowUnfinished = filter.basic_filters().allow_unfinished();
    result.allowNoGenre = filter.genre_filter().allow_no_genre();

    result.ensureActive = filter.basic_filters().ensure_active();
    result.deadFicDaysRange= filter.basic_filters().last_active_days();
    if(result.deadFicDaysRange == 0)
        result.deadFicDaysRange = 365;
    result.ensureCompleted = filter.basic_filters().ensure_completed();

    result.mode = static_cast<core::StoryFilter::EFilterMode>(filter.filtering_mode());
    result.sortMode = static_cast<core::StoryFilter::ESortMode>(filter.sort_mode());
    result.rating = static_cast<core::StoryFilter::ERatingFilter>(filter.rating());
    result.genrePresenceForInclude = static_cast<core::StoryFilter::EGenrePresence>(filter.genre_filter().genre_presence_include());
    result.genrePresenceForExclude = static_cast<core::StoryFilter::EGenrePresence>(filter.genre_filter().genre_presence_exclude());

    result.recordLimit = filter.size_limits().record_limit();
    result.recordPage = filter.size_limits().record_page();

    result.reviewBias = static_cast<core::StoryFilter::EReviewBiasMode>(filter.review_bias().bias_mode());
    result.biasOperator = static_cast<core::StoryFilter::EBiasOperator>(filter.review_bias().bias_operator());
    result.reviewBiasRatio = filter.review_bias().bias_ratio();

    result.recentAndPopularFavRatio = filter.recent_and_popular().fav_ratio();
    result.recentCutoff = DFS(filter.recent_and_popular().date_cutoff());
    if(filter.sources().authors_size() > 0)
        result.useThisAuthor = filter.sources().authors(0);
    for(int i = 0; i < filter.sources().recommender_ids_size(); i++)
        result.usedRecommenders.push_back(filter.sources().recommender_ids(i));

    result.fandom = filter.content_filter().fandom();
    result.secondFandom = filter.content_filter().crossover_fandom();

    result.includeCrossovers = filter.content_filter().include_crossovers();
    result.crossoversOnly = filter.content_filter().crossovers_only();
    result.otherFandomsMode = filter.content_filter().other_fandoms_mode();
    result.ignoreFandoms = filter.content_filter().use_ignored_fandoms();




    for(int i = 0; i < filter.genre_filter().genre_exclusion_size(); i++)
        result.genreExclusion.push_back(FS(filter.genre_filter().genre_exclusion(i)));
    for(int i = 0; i < filter.genre_filter().genre_inclusion_size(); i++)
        result.genreInclusion.push_back(FS(filter.genre_filter().genre_inclusion(i)));
    for(int i = 0; i < filter.content_filter().word_exclusion_size(); i++)
        result.wordExclusion.push_back(FS(filter.content_filter().word_exclusion(i)));
    for(int i = 0; i < filter.content_filter().word_inclusion_size(); i++)
        result.wordInclusion.push_back(FS(filter.content_filter().word_inclusion(i)));

    result.ignoreAlreadyTagged = filter.tag_filter().ignore_already_tagged();

    auto* userThreadData = ThreadData::GetUserData();


    userThreadData->allTaggedFics.reserve(userData.user_tags().all_tags_size());
    for(int i = 0; i < userData.user_tags().all_tags_size(); i++)
        userThreadData->allTaggedFics.insert(userData.user_tags().all_tags(i));


    userThreadData->allSnoozedFics.reserve(userData.user_tags().all_tags_size());
    for(int i = 0; i < userData.snoozes_size(); i++)
        userThreadData->allSnoozedFics.insert(userData.snoozes(i));
    QLOG_INFO() << "passed snooze size:" << userData.snoozes_size();

    userThreadData->ficIDsForActivetags.reserve(userData.user_tags().searched_tags_size());
    for(int i = 0; i < userData.user_tags().searched_tags_size(); i++)
        userThreadData->ficIDsForActivetags.insert(userData.user_tags().searched_tags(i));
    result.activeTagsCount = userThreadData->ficIDsForActivetags.size();
    result.allTagsCount = userThreadData->allTaggedFics.size();
    result.allSnoozeCount = userThreadData->allSnoozedFics.size();


    result.slashFilter.slashFilterEnabled = filter.explicit_filter().slash().use_filter();
    result.slashFilter.excludeSlash = filter.explicit_filter().slash().exclude_content();
    result.slashFilter.includeSlash = filter.explicit_filter().slash().include_content();
    result.slashFilter.enableFandomExceptions = filter.explicit_filter().slash().enable_exceptions();
    result.slashFilter.slashFilterLevel = filter.explicit_filter().slash().filter_level();
    result.slashFilter.onlyMatureForSlash = filter.explicit_filter().slash().filter_only_mature();
    result.slashFilter.onlyExactLevel= filter.explicit_filter().slash().show_exact_level();

    for(int i = 0; i < filter.explicit_filter().slash().fandom_exceptions_size(); i++)
        result.slashFilter.fandomExceptions.push_back(filter.explicit_filter().slash().fandom_exceptions(i));

    result.listOpenMode = filter.recommendations().list_open_mode();
    result.listForRecommendations = filter.recommendations().list_for_recommendations();
    result.useThisRecommenderOnly = filter.recommendations().use_this_recommender_only();
    result.minRecommendations = filter.recommendations().min_recommendations();
    result.showOriginsInLists = filter.recommendations().show_origins_in_lists();
    result.tagsAreUsedForAuthors = filter.tag_filter().tags_are_for_authors();
    result.displaySnoozedFics= filter.snoozes().display_all();
    result.tagsAreANDed = filter.tag_filter().use_and_for_tags();
    for(int i =0; i < filter.tag_filter().active_tags_size(); i++)
        result.activeTags.push_back(QString::fromStdString(filter.tag_filter().active_tags(i)));

    result.ignoredFandomCount = userData.ignored_fandoms().fandom_ids_size();
    result.recommendationsCount = userData.recommendation_list().list_of_fics_size();
    for(int i = 0; i < userData.recommendation_list().list_of_fics_size(); i++)
        result.recsHash[userData.recommendation_list().list_of_fics(i)] = userData.recommendation_list().list_of_matches(i);
    for(int i = 0; i < userData.scores_list().list_of_fics_size(); i++)
        result.scoresHash[userData.scores_list().list_of_fics(i)] = userData.scores_list().list_of_scores(i);

    for(int i = 0; i < userData.ignored_fandoms().fandom_ids_size(); i++)
        userThreadData->ignoredFandoms[userData.ignored_fandoms().fandom_ids(i)] = userData.ignored_fandoms().ignore_crossovers(i);

    for(auto& item: userData.fandomstatetokens()){
        core::fandom_lists::FandomSearchStateToken token;
        token.id = item.id();
        token.inclusionMode = static_cast<core::fandom_lists::EInclusionMode>(item.inclusion_mode());
        token.crossoverInclusionMode = static_cast<core::fandom_lists::ECrossoverInclusionMode>(item.crossover_inclusion_mode());
        result.fandomStates.insert_or_assign(item.id(), std::move(token));
    }

    return result;
}

QString RelevanceToString(float value)
{
    if(value > 0.8f)
        return QStringLiteral("#c#");
    if(value > 0.5f)
        return QStringLiteral("#p#");
    if(value > 0.2f)
        return QStringLiteral("#b#");
    return QStringLiteral("#b#");
}

QString GenreDataToString(QList<genre_stats::GenreBit> data)
{
    QStringList resultList;
    float maxValue = 0;
    for(const auto& genre : data)
    {
        if(genre.relevance > maxValue)
            maxValue = genre.relevance;
    }

    for(const auto& genre : data)
    {
        for(const auto& genreBit : std::as_const(genre.genres))
        {
            const auto temp = genreBit.split(QStringLiteral(","));
            for(const auto& stringBit : temp)
            {
                if(std::abs(maxValue - genre.relevance) < 0.1f)
                    resultList+=QStringLiteral("#c#") + stringBit;
                else
                    resultList+=RelevanceToString(genre.relevance/maxValue) + stringBit;
            }
        }
    }

    QString result =  resultList.join(QStringLiteral(","));
    return result;
}


bool ProtoFicToLocalFic(const ProtoSpace::Fanfic& protoFic, core::Fanfic& coreFic)
{
    coreFic.isValid = protoFic.is_valid();
    if(!coreFic.isValid)
        return false;

    coreFic.identity.id = protoFic.id();

    // I will probably disable this for now in the ui
    coreFic.userData.atChapter = GetChapterForFic(coreFic.identity.id);

    coreFic.complete = protoFic.complete();
    coreFic.recommendationsData.recommendationsMainList = protoFic.recommendations();
    coreFic.wordCount = FS(protoFic.word_count());
    coreFic.chapters = FS(protoFic.chapters());
    //qDebug() << "received chapters: " << coreFic.chapters ;
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
    coreFic.author_id = protoFic.author_id();

    for(int i = 0; i < protoFic.fandoms_size(); i++)
        coreFic.fandoms.push_back(FS(protoFic.fandoms(i)));
    for(int i = 0; i < protoFic.fandom_ids_size(); i++)
        coreFic.fandomIds.push_back(protoFic.fandom_ids(i));
    coreFic.isCrossover = coreFic.fandoms.size() > 1;

    coreFic.genreString = FS(protoFic.genres());
    coreFic.identity.web.ffn = protoFic.site_pack().ffn().id();  // temporary
    coreFic.webSite = QStringLiteral("ffn"); // temporary

    coreFic.urls[QStringLiteral("ffn")] = QString::number(coreFic.identity.web.ffn); // temporary
    for(auto i =0; i < protoFic.real_genres_size(); i++)
        coreFic.statistics.realGenreData.push_back({{FS(protoFic.real_genres(i).genre())}, protoFic.real_genres(i).relevance()});

    std::sort(coreFic.statistics.realGenreData.begin(),coreFic.statistics.realGenreData.end(),[](const genre_stats::GenreBit& g1,const genre_stats::GenreBit& g2){
        if(g1.genres.size() != 0 && g2.genres.size() == 0)
            return true;
        if(g2.genres.size() != 0 && g1.genres.size() == 0)
            return false;
        if(std::abs(g1.relevance - g2.relevance) < 0.1f)
            return g1.genres < g2.genres;
        return g1.relevance > g2.relevance;
    });

    coreFic.statistics.realGenreString = GenreDataToString(coreFic.statistics.realGenreData);

    coreFic.urlFFN = coreFic.urls["ffn"];

    coreFic.slashData.keywords_no = protoFic.slash_data().keywords_no();
    coreFic.slashData.keywords_yes = protoFic.slash_data().keywords_yes();
    coreFic.slashData.keywords_result = protoFic.slash_data().keywords_result();
    coreFic.slashData.filter_pass_1= protoFic.slash_data().filter_pass_1();
    coreFic.slashData.filter_pass_2= protoFic.slash_data().filter_pass_2();
    if(coreFic.slashData.keywords_result)
        coreFic.statistics.minSlashPass = 1;
    else if(coreFic.slashData.filter_pass_1)
        coreFic.statistics.minSlashPass = 2;
    else if(coreFic.slashData.filter_pass_2)
        coreFic.statistics.minSlashPass = 3;
    else
        coreFic.statistics.minSlashPass = 0;

    return true;
}

bool LocalFicToProtoFic(const core::Fanfic& coreFic, ProtoSpace::Fanfic* protoFic)
{
    protoFic->set_is_valid(true);
    protoFic->set_id(coreFic.identity.id);

    protoFic->set_chapters(TS(coreFic.chapters));
    protoFic->set_complete(coreFic.complete);
    protoFic->set_recommendations(coreFic.recommendationsData.recommendationsMainList);

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
    protoFic->set_author_id(coreFic.author_id);

    protoFic->set_published(DTS(coreFic.published));
    protoFic->set_updated(DTS(coreFic.updated));
    protoFic->set_characters(TS(coreFic.charactersFull));


    for(const auto& fandom : std::as_const(coreFic.fandoms))
        protoFic->add_fandoms(TS(fandom));
    for(const auto& fandom : std::as_const(coreFic.fandomIds))
        protoFic->add_fandom_ids(fandom);

    for(const auto& realGenre : std::as_const(coreFic.statistics.realGenreData))
    {
        auto* genreData =  protoFic->add_real_genres();
        genreData->set_genre(TS(realGenre.genres.join(",")));
        genreData->set_relevance(realGenre.relevance);
    }


    protoFic->mutable_site_pack()->mutable_ffn()->set_id(coreFic.identity.web.ffn);

    auto slashData = protoFic->mutable_slash_data();
    slashData->set_keywords_no(coreFic.slashData.keywords_no);
    slashData->set_keywords_yes(coreFic.slashData.keywords_yes);
    slashData->set_keywords_result(coreFic.slashData.keywords_result);
    slashData->set_filter_pass_1(coreFic.slashData.filter_pass_1);
    slashData->set_filter_pass_2(coreFic.slashData.filter_pass_2);

    return true;
}

bool FavListProtoToLocal(const ProtoSpace::FavListDetails &protoStats, core::FavListDetails &stats)
{
    stats.isValid = protoStats.is_valid();
    if(!stats.isValid)
        return false;

    stats.favourites = protoStats.fic_count();
    stats.ficWordCount = protoStats.word_count();
    stats.averageWordsPerChapter = protoStats.average_words_per_chapter();
    stats.averageLength = protoStats.average_wordcount();
    stats.fandomsDiversity = protoStats.fandom_diversity();
    stats.explorerFactor = protoStats.explorer_rating();
    stats.megaExplorerFactor = protoStats.mega_explorer_rating();
    stats.crossoverFactor = protoStats.crossover_rating();
    stats.unfinishedFactor = protoStats.unfinished_rating();
    stats.genreDiversityFactor = protoStats.genre_diversity_rating();
    stats.moodUniformity = protoStats.mood_uniformity_rating();

    stats.crackRatio = protoStats.crack_rating();
    stats.slashRatio = protoStats.slash_rating();
    stats.smutRatio = protoStats.smut_rating();

    stats.firstPublished = QDate::fromString(FS(protoStats.published_first()), QStringLiteral("yyyyMMdd"));
    stats.lastPublished = QDate::fromString(FS(protoStats.published_last()), QStringLiteral("yyyyMMdd"));

    stats.moodSad = protoStats.mood_rating(0);
    stats.moodNeutral = protoStats.mood_rating(1);
    stats.moodHappy = protoStats.mood_rating(2);
    stats.noInfoCount = protoStats.no_info();

    for(auto i = 0; i< protoStats.size_rating_size(); i++)
        stats.sizeFactors[i] = protoStats.size_rating(i);
    for(auto i = 0; i< protoStats.genres_size(); i++)
        stats.genreFactors[FS(protoStats.genres(i))] = protoStats.genres_percentages(i);
    for(auto i = 0; i< protoStats.fandoms_size(); i++)
        stats.fandomsConverted[protoStats.fandoms(i)] = protoStats.fandoms_counts(i);
    return true;
}

bool FavListLocalToProto(const core::FavListDetails &stats, ProtoSpace::FavListDetails *protoStats)
{
    protoStats->set_is_valid(true);
    protoStats->set_fic_count(stats.favourites);
    protoStats->set_word_count(stats.ficWordCount);
    protoStats->set_average_wordcount(stats.averageLength);
    protoStats->set_average_words_per_chapter(stats.averageWordsPerChapter);
    protoStats->set_fandom_diversity(stats.fandomsDiversity);
    protoStats->set_explorer_rating(stats.explorerFactor);
    protoStats->set_mega_explorer_rating(stats.megaExplorerFactor);
    protoStats->set_crossover_rating(stats.crossoverFactor);
    protoStats->set_unfinished_rating(stats.unfinishedFactor);
    protoStats->set_genre_diversity_rating(stats.genreDiversityFactor);
    protoStats->set_mood_uniformity_rating(stats.moodUniformity);


    protoStats->set_crack_rating(stats.crackRatio);
    protoStats->set_slash_rating(stats.slashRatio);
    protoStats->set_smut_rating(stats.smutRatio);

    protoStats->set_published_first(stats.firstPublished.toString(QStringLiteral("yyyyMMdd")).toStdString());
    protoStats->set_published_last(stats.lastPublished.toString(QStringLiteral("yyyyMMdd")).toStdString());
    protoStats->add_mood_rating(stats.moodSad);
    protoStats->add_mood_rating(stats.moodNeutral);
    protoStats->add_mood_rating(stats.moodHappy);


    for(auto i = stats.sizeFactors.cbegin(); i !=stats.sizeFactors.cend(); i++)
        protoStats->add_size_rating(i.value());

    for(auto i = stats.genreFactors.cbegin(); i !=stats.genreFactors.cend(); i++)
    {
        protoStats->add_genres(TS(i.key()));
        protoStats->add_genres_percentages(i.value());
    }

    for(auto i = stats.fandomsConverted.cbegin(); i !=stats.fandomsConverted.cend(); i++)
    {
        protoStats->add_fandoms(i.key());
        protoStats->add_fandoms_counts(i.value());
    }
    return true;
}

bool AuthorListProtoToLocal(const ProtoSpace::AuthorsForFicsResponse &proto, QHash<uint32_t, uint32_t> &result)
{
    if(!proto.success())
        return false;

    for(auto i = 0; i < proto.fics_size(); i ++)
        result[static_cast<uint32_t>(proto.fics(i))] = static_cast<uint32_t>(proto.authors(i));

    return true;
}

}

class FicSourceGRPCImpl{
    //    friend class GrpcServiceBase;
public:
    FicSourceGRPCImpl(QString connectionString, int deadline)
        : deadline(deadline)
    {
            CreateStub(connectionString);
            this->connectionString = connectionString;
    }
    void CreateStub(QString);
    ServerStatus GetStatus();
    bool GetInternalIDsForFics(QVector<core::Identity> * ficList);
    bool GetFFNIDsForFics(QVector<core::Identity> * ficList);
    void FetchData(const core::StoryFilter& filter, QVector<core::Fanfic> * fics);
    void FetchFic(int ficId, QVector<core::Fanfic> * fics, core::StoryFilter::EUseThisFicType idType = core::StoryFilter::EUseThisFicType::utf_ffn_id);
    int GetFicCount(const core::StoryFilter& filter);
    bool GetFandomListFromServer(int lastFandomID, QVector<core::Fandom>* fandoms);
    bool GetRecommendationListFromServer(QSharedPointer<core::RecommendationList> recList);
    core::DiagnosticsForReclist GetDiagnosticsForRecommendationListFromServer(QSharedPointer<core::RecommendationList> recList);
    void ProcessStandardError(const grpc::Status &status);
    core::FavListDetails GetStatsForFicList(QVector<core::Identity> ficList);
    QHash<uint32_t, uint32_t> GetAuthorsForFicList(QSet<int> ficList);
    QSet<int> GetAuthorsForFicInRecList(int sourceFic, QString authors);
    QHash<int, core::FavouritesMatchResult > GetMatchesForUsers(int sourceUser, QList<int> users);
    QHash<int, core::FavouritesMatchResult> GetMatchesForUsers(InputsForMatches data, QList<int> users);
    QSet<int> GetExpiredSnoozes(QHash<int, core::FanficSnoozeStatus> data);
    void FillControlStruct(ProtoSpace::ControlInfo *controls);

    std::unique_ptr<ProtoSpace::Feeder::Stub> stub_;
    QString error;
    bool hasErrors = false;
    int deadline = 60;
    QString userToken;
    QString applicationToken;
    UserData userData;
    QString connectionString;
};
#define TO_STR2(x) #x
#define STRINGIFY(x) TO_STR2(x)
void FicSourceGRPCImpl::CreateStub(QString connectionString)
{
    grpc::ChannelArguments args;
    args.SetMaxReceiveMessageSize(100000000);
    auto customChannel = grpc::CreateCustomChannel(connectionString.toStdString(), grpc::InsecureChannelCredentials(), args);
    auto newStub = ProtoSpace::Feeder::NewStub(customChannel);
    stub_.reset(newStub.release()) ;
}

ServerStatus FicSourceGRPCImpl::GetStatus()
{
    ServerStatus serverStatus;
    grpc::ClientContext context;
    ProtoSpace::StatusRequest task;

    QScopedPointer<ProtoSpace::StatusResponse> response (new ProtoSpace::StatusResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    FillControlStruct(task.mutable_controls());


    grpc::Status status = stub_->GetStatus(&context, task, response.data());

    ProcessStandardError(status);
    if(hasErrors)
    {
        serverStatus.error = error;
        return serverStatus;
    }
    serverStatus.isValid = true;
    serverStatus.dbAttached = response->database_attached();
    //QString dbUpdate = proto_converters::FS(response->last_database_update());
    serverStatus.lastDBUpdate = QString::fromStdString(response->last_database_update());
    serverStatus.motd = proto_converters::FS(response->message_of_the_day());
    serverStatus.messageRequired = response->need_to_show_motd();
    int ownProtocolVersion = QStringLiteral(STRINGIFY(MAJOR_PROTOCOL_VERSION)).toInt();
    serverStatus.protocolVersionMismatch = ownProtocolVersion != response->protocol_version();
    return serverStatus;
}

bool FicSourceGRPCImpl::GetInternalIDsForFics(QVector<core::Identity> * ficList){
    grpc::ClientContext context;

    ProtoSpace::FicIdRequest task;

    if(!ficList->size())
        return true;


    QScopedPointer<ProtoSpace::FicIdResponse> response (new ProtoSpace::FicIdResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    FillControlStruct(task.mutable_controls());

    for(core::Identity& fic : *ficList)
    {
        task.mutable_ids()->add_db_ids(fic.id);
        task.mutable_ids()->add_ffn_ids(fic.web.ffn);
    }

    grpc::Status status = stub_->GetDBFicIDS(&context, task, response.data());

    ProcessStandardError(status);

    for(int i = 0; i < response->ids().db_ids_size(); i++)
    {
        (*ficList)[i].id = response->ids().db_ids(i);
        (*ficList)[i].web.ffn = response->ids().ffn_ids(i);
    }
    return true;
}

bool FicSourceGRPCImpl::GetFFNIDsForFics(QVector<core::Identity> *ficList)
{
    grpc::ClientContext context;

    ProtoSpace::FicIdRequest task;

    if(!ficList->size())
        return true;

    QScopedPointer<ProtoSpace::FicIdResponse> response (new ProtoSpace::FicIdResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    FillControlStruct(task.mutable_controls());
    for(core::Identity& fic : *ficList)
    {
        task.mutable_ids()->add_db_ids(fic.id);
        task.mutable_ids()->add_ffn_ids(fic.web.ffn);
    }

    grpc::Status status = stub_->GetFFNFicIDS(&context, task, response.data());

    ProcessStandardError(status);

    for(int i = 0; i < response->ids().db_ids_size(); i++)
    {
        (*ficList)[i].id = response->ids().db_ids(i);
        (*ficList)[i].web.ffn = response->ids().ffn_ids(i);
    }
    return true;
}

void FicSourceGRPCImpl::FetchData(const core::StoryFilter &filter, QVector<core::Fanfic> * fics)
{
    QLOG_INFO() << "Fetching fics data: ";
    grpc::ClientContext context;

    ProtoSpace::SearchTask task;

    ProtoSpace::Filter protoFilter;
    auto* userData = task.mutable_user_data();
    protoFilter = proto_converters::StoryFilterIntoProto(filter, userData);
    task.set_allocated_filter(&protoFilter);

    QScopedPointer<ProtoSpace::SearchResponse> response (new ProtoSpace::SearchResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    auto* controls = task.mutable_controls();
    FillControlStruct(controls);
    controls->mutable_protocol_version()->set_major_version(filter.protocolMajorVersion);
    controls->mutable_protocol_version()->set_minor_version(filter.protocolMinorVersion);


    auto* tags = userData->mutable_user_tags();

    for(auto fic : std::as_const(this->userData.allTaggedFics))
    {
        //QLOG_INFO() << "adding fic ignore: " << fic;
        tags->add_all_tags(fic);
    }

    for(auto tag : std::as_const(this->userData.ficIDsForActivetags))
        tags->add_searched_tags(tag);
    for(auto snooze : std::as_const(this->userData.allSnoozedFics))
        userData->add_snoozes(snooze);


    auto* ignoredFandoms = userData->mutable_ignored_fandoms();

    for(auto i = this->userData.ignoredFandoms.cbegin(); i != this->userData.ignoredFandoms.cend(); i++)
    {
        auto key = i.key();
        //QLOG_INFO() << "adding fandom ignore: " << key << " " << i.value();
        if(key==-1)
            continue;
        ignoredFandoms->add_fandom_ids(key);
        ignoredFandoms->add_ignore_crossovers(i.value());
    }

    for(auto it = this->userData.fandomStates.cbegin(); it != this->userData.fandomStates.cend(); it++){
        auto token = userData->add_fandomstatetokens();
        token->set_id(it->second.id);
        token->set_inclusion_mode(static_cast<::ProtoSpace::EInclusionMode>((it->second.inclusionMode)));
        token->set_crossover_inclusion_mode(static_cast<::ProtoSpace::ECrossoverInclusionMode>((it->second.crossoverInclusionMode)));
    }

    auto tagData = task.mutable_filter()->mutable_tag_filter();
    for(const auto& tag: std::as_const(filter.activeTags))
        tagData->add_active_tags(tag.toStdString());

    grpc::Status status = stub_->Search(&context, task, response.data());

    ProcessStandardError(status);

    fics->resize(static_cast<int>(response->fanfics_size()));
    for(int i = 0; i < response->fanfics_size(); i++)
    {
        proto_converters::ProtoFicToLocalFic(response->fanfics(i), (*fics)[static_cast<int>(i)]);
    }

    task.release_filter();
}

void FicSourceGRPCImpl::FetchFic(int ficId,  QVector<core::Fanfic> *fics, core::StoryFilter::EUseThisFicType idType)
{
    grpc::ClientContext context;

    ProtoSpace::SearchByFFNIDTask task;

    ProtoSpace::Filter protoFilter;
    QScopedPointer<ProtoSpace::SearchByFFNIDResponse> response (new ProtoSpace::SearchByFFNIDResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    FillControlStruct(task.mutable_controls());
    task.set_id(ficId);
    if(idType == core::StoryFilter::EUseThisFicType::utf_ffn_id)
        task.set_id_type(ProtoSpace::SearchByFFNIDTask::ffn);
    else
        task.set_id_type(ProtoSpace::SearchByFFNIDTask::db);


    grpc::Status status = stub_->SearchByFFNID(&context, task, response.data());

    ProcessStandardError(status);

    fics->resize(1);
    proto_converters::ProtoFicToLocalFic(response->fanfic(), (*fics)[static_cast<size_t>(0)]);
}

int FicSourceGRPCImpl::GetFicCount(const core::StoryFilter& filter)
{
    grpc::ClientContext context;

    ProtoSpace::FicCountTask task;

    ProtoSpace::Filter protoFilter;
    auto* userData = task.mutable_user_data();
    protoFilter = proto_converters::StoryFilterIntoProto(filter, userData);
    task.set_allocated_filter(&protoFilter);

    auto* controls = task.mutable_controls();
    FillControlStruct(controls);
    controls->mutable_protocol_version()->set_major_version(filter.protocolMajorVersion);
    controls->mutable_protocol_version()->set_minor_version(filter.protocolMinorVersion);

    QScopedPointer<ProtoSpace::FicCountResponse> response (new ProtoSpace::FicCountResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    auto* tags = userData->mutable_user_tags();
    for(const auto& tag : std::as_const(this->userData.allTaggedFics))
        tags->add_all_tags(tag);
    for(const auto& tag : std::as_const(this->userData.ficIDsForActivetags))
        tags->add_searched_tags(tag);
    for(const auto& snooze : std::as_const(this->userData.allSnoozedFics))
        userData->add_snoozes(snooze);

    auto* ignoredFandoms = userData->mutable_ignored_fandoms();

    for(auto i = this->userData.ignoredFandoms.cbegin(); i != this->userData.ignoredFandoms.cend(); i++)
    {
        ignoredFandoms->add_fandom_ids(i.key());
        ignoredFandoms->add_ignore_crossovers(i.value());
    }
    for(auto it = this->userData.fandomStates.cbegin(); it != this->userData.fandomStates.cend(); it++){
        auto token = userData->add_fandomstatetokens();
        token->set_id(it->second.id);
        token->set_inclusion_mode(static_cast<::ProtoSpace::EInclusionMode>((it->second.inclusionMode)));
        token->set_crossover_inclusion_mode(static_cast<::ProtoSpace::ECrossoverInclusionMode>((it->second.crossoverInclusionMode)));
    }

    grpc::Status status = stub_->GetFicCount(&context, task, response.data());

    ProcessStandardError(status);
    task.release_filter();
    int result = response->fic_count();
    return result;
}

bool FicSourceGRPCImpl::GetFandomListFromServer(int lastFandomID, QVector<core::Fandom>* fandoms)
{
    grpc::ClientContext context;

    ProtoSpace::SyncFandomListTask task;
    task.set_last_fandom_id(lastFandomID);

    QScopedPointer<ProtoSpace::SyncFandomListResponse> response (new ProtoSpace::SyncFandomListResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    task.mutable_controls()->set_user_token(proto_converters::TS(userToken));
    task.mutable_controls()->set_application_token(proto_converters::TS(applicationToken));

    grpc::Status status = stub_->SyncFandomList(&context, task, response.data());

    if(!status.ok())
        QLOG_INFO() << "error calling grpc: " <<  status.error_code() << " " << QString::fromStdString(status.error_message());

    ProcessStandardError(status);
    if(!response->needs_update())
        return false;

    fandoms->resize(response->fandoms_size());
    for(int i = 0; i < response->fandoms_size(); i++)
    {
        proto_converters::ProtoFandomToLocalFandom(response->fandoms(i), (*fandoms)[static_cast<int>(i)]);
    }
    return true;
}

static const auto paramToTaskFiller = [](auto& task, QSharedPointer<core::RecommendationList> recList){
    auto* data = task.mutable_data();
    auto* ffn = data->mutable_id_packs();
    auto* params = data->mutable_general_params();

    //std::sort(std::begin(recList->ficData.fics), std::end(recList->ficData.fics));
    //qDebug() << "Source fics  for list: ";
    for(auto fic: std::as_const(recList->ficData->fics))
    {
        //qDebug() << fic;
        ffn->add_ffn_ids(fic);
    }
    data->mutable_response_data_controls()->set_ignore_breakdowns(recList->ignoreBreakdowns);
    data->set_list_name(proto_converters::TS(recList->name));
    params->set_always_pick_at(recList->alwaysPickAt);
    params->set_is_automatic(recList->isAutomatic);
    params->set_use_dislikes(recList->useDislikes);
    params->set_use_dead_fic_ignore(recList->useDeadFicIgnore);
    params->set_min_fics_to_match(recList->minimumMatch);
    params->set_list_size_multiplier(recList->listSizeMultiplier);
    params->set_source_favourites_cutoff(recList->ficFavouritesCutoff);
    params->set_max_unmatched_to_one_matched(static_cast<int>(recList->maxUnmatchedPerMatch));
    params->set_use_weighting(recList->useWeighting);
    params->set_use_mood_filtering(recList->useMoodAdjustment);
    params->set_users_ffn_profile_id(recList->userFFNId);
    QLOG_INFO() << "passing user id to server: " << recList->userFFNId;

    auto userData = data->mutable_user_data();
    auto ignores = userData->mutable_ignored_fandoms();
    for(auto ignore: std::as_const(recList->ignoredFandoms))
        ignores->add_fandom_ids(ignore);
    //auto likedAuthors = userData->mutable_liked_authors();
    for(auto authorId: std::as_const(recList->likedAuthors))
        userData->add_liked_authors(authorId);

    for(auto vote: std::as_const(recList->majorNegativeVotes))
        userData->mutable_negative_feedback()->add_strongnegatives(vote);
    for(auto fic: std::as_const(recList->ignoredDeadFics))
        userData->add_ignored_fics(fic);

};


static const auto basicRecListFiller = [](const ::ProtoSpace::RecommendationListData& response, QSharedPointer<core::RecommendationList> recList){

   recList->ficData->fics.clear();
   recList->ficData->fics.reserve(response.fic_ids_size());
   recList->ficData->purges.reserve(response.fic_ids_size());
   recList->ficData->matchCounts.reserve(response.fic_ids_size());
   recList->ficData->noTrashScores.reserve(response.fic_ids_size());

    for(int i = 0; i < response.fic_ids_size(); i++)
       recList->ficData->fics.push_back(response.fic_ids(i));
    for(int i = 0; i < response.fic_matches_size(); i++)
        recList->ficData->matchCounts.push_back(response.fic_matches(i));
    for(int i = 0; i < response.purged_size(); i++)
        recList->ficData->purges.push_back(response.purged(i));
    for(int i = 0; i < response.no_trash_score_size(); i++)
        recList->ficData->noTrashScores.push_back(response.no_trash_score(i));


    for(int i = 0; i < response.author_ids_size(); i++)
       recList->ficData->authorIds.push_back(response.author_ids(i));

    auto it = response.match_report().begin();
    while(it != response.match_report().end())
    {
       recList->ficData->matchReport[it->first] = it->second;
        ++it;
    }
    using core::AuthorWeightingResult;
    if(!recList->ignoreBreakdowns){
        for(int i = 0; i < response.breakdowns_size(); i++)
        {
            auto ficid= response.breakdowns(i).id();
            recList->ficData->breakdowns[ficid].ficId = static_cast<uint32_t>(ficid);
            auto&  breakdown =recList->ficData->breakdowns[response.breakdowns(i).id()];
            breakdown.AddAuthorResult(AuthorWeightingResult::EAuthorType::common,
                                      response.breakdowns(i).counts_common(),
                                      response.breakdowns(i).votes_common());
            breakdown.AddAuthorResult(AuthorWeightingResult::EAuthorType::uncommon,
                                      response.breakdowns(i).counts_uncommon(),
                                      response.breakdowns(i).votes_uncommon());
            breakdown.AddAuthorResult(AuthorWeightingResult::EAuthorType::rare,
                                      response.breakdowns(i).counts_rare(),
                                      response.breakdowns(i).votes_rare());
            breakdown.AddAuthorResult(AuthorWeightingResult::EAuthorType::unique,
                                      response.breakdowns(i).counts_unique(),
                                      response.breakdowns(i).votes_unique());
        }
    }
    // need to fill the params for the list as they were adjusted on the server
   recList->isAutomatic = response.used_params().is_automatic();
   recList->useDislikes = response.used_params().use_dislikes();
   recList->useDeadFicIgnore= response.used_params().use_dead_fic_ignore();
   recList->minimumMatch = response.used_params().min_fics_to_match();
   recList->maxUnmatchedPerMatch = response.used_params().max_unmatched_to_one_matched();
   recList->alwaysPickAt = response.used_params().always_pick_at();
   recList->useWeighting = response.used_params().use_weighting();
   recList->useMoodAdjustment = response.used_params().use_mood_filtering();
   recList->userFFNId = response.used_params().users_ffn_profile_id();
   recList->success = response.success();

};

bool FicSourceGRPCImpl::GetRecommendationListFromServer(QSharedPointer<core::RecommendationList> recList)
{
    grpc::ClientContext context;
    ProtoSpace::RecommendationListCreationRequest task;
    QScopedPointer<ProtoSpace::RecommendationListCreationResponse> response (new ProtoSpace::RecommendationListCreationResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    paramToTaskFiller(task, recList);

    FillControlStruct(task.mutable_controls());

    grpc::Status status = stub_->RecommendationListCreation(&context, task, response.data());
    if(!status.ok())
        QLOG_INFO() << "error calling grpc: " <<  status.error_code() << " " << QString::fromStdString(status.error_message());

    ProcessStandardError(status);
    //DumpToLog("Test Dump", response.data());
    if(!response->list().list_ready())
        return false;

    basicRecListFiller(response->list(), recList);
    return true;
}


core::DiagnosticsForReclist FicSourceGRPCImpl::GetDiagnosticsForRecommendationListFromServer(QSharedPointer<core::RecommendationList> recList)
{
    core::DiagnosticsForReclist result;

    grpc::ClientContext context;
    ProtoSpace::DiagnosticRecommendationListCreationRequest task;
    QScopedPointer<ProtoSpace::DiagnosticRecommendationListCreationResponse> response (new ProtoSpace::DiagnosticRecommendationListCreationResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);

    context.set_deadline(deadline);
    paramToTaskFiller(task, recList);

    FillControlStruct(task.mutable_controls());

    grpc::Status status = stub_->DiagnosticRecommendationListCreation(&context, task, response.data());

    ProcessStandardError(status);
    //DumpToLog("Test Dump", response.data());

    for(int ficCounter = 0; ficCounter < response->list().matches_size(); ficCounter++)
        for(int authorCounter = 0; authorCounter < response->list().matches(ficCounter).author_id_size(); authorCounter++)
            result.authorsForFics[response->list().matches(ficCounter).fic_id()].push_back(response->list().matches(ficCounter).author_id(authorCounter));

    for(int authorCounter = 0; authorCounter < response->list().author_params_size(); authorCounter++){
        core::AuthorResult author;
        author.id = response->list().author_params(authorCounter).author_id();
        author.fullListSize = response->list().author_params(authorCounter).full_list_size();
        author.matches = response->list().author_params(authorCounter).total_matches();
        author.negativeMatches = response->list().author_params(authorCounter).negative_matches();
        author.authorMatchCloseness = static_cast<core::AuthorWeightingResult::EAuthorType>( response->list().author_params(authorCounter).match_category());
        author.sizeAfterIgnore = response->list().author_params(authorCounter).list_size_without_ignores();
        author.listDiff.touchyDifference = response->list().author_params(authorCounter).ratio_difference_on_touchy_mood();
        author.listDiff.neutralDifference = response->list().author_params(authorCounter).ratio_difference_on_neutral_mood();
        result.authorData.push_back(author);
    }
    result.quad = response->list().quadratic_deviation();
    result.ratioMedian = response->list().ratio_median();
    result.sigma2Dist = response->list().distance_to_double_sigma();
    result.isValid = true;

    return result;
}


void FicSourceGRPCImpl::ProcessStandardError(const grpc::Status& status)
{
    error.clear();
    if(status.ok())
    {
        hasErrors = false;
        return;
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
    error+= "GRPC:" + QString::fromStdString(status.error_message());
    if(!error.isEmpty())
        QLOG_INFO() << "GRPC error: " << error;
}

core::FavListDetails FicSourceGRPCImpl::GetStatsForFicList(QVector<core::Identity> ficList)
{
    core::FavListDetails result;

    grpc::ClientContext context;

    ProtoSpace::FavListDetailsRequest task;

    if(!ficList.size())
        return result;

    QScopedPointer<ProtoSpace::FavListDetailsResponse> response (new ProtoSpace::FavListDetailsResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    FillControlStruct(task.mutable_controls());

    for(core::Identity& fic : ficList)
    {
        auto idPacks = task.mutable_id_packs();
        idPacks->add_ffn_ids(fic.web.ffn);
    }

    grpc::Status status = stub_->GetFavListDetails(&context, task, response.data());

    ProcessStandardError(status);

    if(!response->success())
        return result;

    proto_converters::FavListProtoToLocal(response->details(), result);

    return result;
}

QHash<uint32_t, uint32_t> FicSourceGRPCImpl::GetAuthorsForFicList(QSet<int> ficList)
{
    QHash<uint32_t, uint32_t> result;

    grpc::ClientContext context;

    ProtoSpace::AuthorsForFicsRequest task;

    if(!ficList.size())
        return result;

    QScopedPointer<ProtoSpace::AuthorsForFicsResponse> response (new ProtoSpace::AuthorsForFicsResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    FillControlStruct(task.mutable_controls());

    for(auto fic : ficList)
    {
        auto idPacks = task.mutable_id_packs();
        idPacks->add_db_ids(fic);
    }

    grpc::Status status = stub_->GetAuthorsForFicList(&context, task, response.data());

    ProcessStandardError(status);

    if(!response->success())
        return result;

    proto_converters::AuthorListProtoToLocal(*response, result);

    return result;
}

QSet<int> FicSourceGRPCImpl::GetAuthorsForFicInRecList(int sourceFic, QString authors)
{
    QSet<int> result;

    grpc::ClientContext context;

    ProtoSpace::AuthorsForFicInReclistRequest task;

    QScopedPointer<ProtoSpace::AuthorsForFicInReclistResponse> response (new ProtoSpace::AuthorsForFicInReclistResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    FillControlStruct(task.mutable_controls());
    task.set_fic_id(sourceFic);
    task.set_author_list(authors.toStdString());

    grpc::Status status = stub_->GetAuthorsFromRecListContainingFic(&context, task, response.data());

    ProcessStandardError(status);

    if(!response->success())
        return result;
    for(auto i = 0; i < response->filtered_authors_size(); i++)
        result.insert(response->filtered_authors(i));

    return result;
}

QHash<int, core::FavouritesMatchResult > FicSourceGRPCImpl::GetMatchesForUsers(int sourceUser, QList<int> users)
{
    QHash<int, core::FavouritesMatchResult> result;

    grpc::ClientContext context;

    ProtoSpace::UserMatchRequest task;

    QScopedPointer<ProtoSpace::UserMatchResponse> response (new ProtoSpace::UserMatchResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
//    auto* controls = task.mutable_controls();
//    controls->set_user_token(proto_converters::TS(userToken));
    task.set_source_user(sourceUser);
    for(auto user: users)
        task.add_test_users(user);

    grpc::Status status = stub_->GetUserMatches(&context, task, response.data());

    ProcessStandardError(status);

    if(!response->success())
        return result;
    for(auto i = 0; i < response->matches_size(); i++)
    {
        auto match = response->matches(i);
        result[match.user_id()].ratio = match.ratio();
        result[match.user_id()].ratioWithoutIgnores = match.ratio_without_ignores();
        for(auto j = 0; j < match.fic_id_size(); j++)
            result[match.user_id()].matches.push_back(match.fic_id(j));
    }

    return result;
}

QHash<int, core::FavouritesMatchResult > FicSourceGRPCImpl::GetMatchesForUsers(InputsForMatches data, QList<int> users)
{
    QHash<int, core::FavouritesMatchResult> result;

    grpc::ClientContext context;

    ProtoSpace::UserMatchRequest task;

    QScopedPointer<ProtoSpace::UserMatchResponse> response (new ProtoSpace::UserMatchResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
//    auto* controls = task.mutable_controls();
//    controls->set_user_token(proto_converters::TS(userToken));
    for(const auto& fic: std::as_const(data.userFics))
        task.add_user_fics(fic.toInt());
    for(const auto& fic: std::as_const(data.userIgnores))
        task.add_fandom_ignores(fic.toInt());
    for(auto user: users)
        task.add_test_users(user);

    grpc::Status status = stub_->GetUserMatches(&context, task, response.data());

    ProcessStandardError(status);

    if(!response->success())
        return result;
    for(auto i = 0; i < response->matches_size(); i++)
    {
        auto match = response->matches(i);
        result[match.user_id()].ratio = match.ratio();
        result[match.user_id()].ratioWithoutIgnores = match.ratio_without_ignores();
        for(auto j = 0; j < match.fic_id_size(); j++)
            result[match.user_id()].matches.push_back(match.fic_id(j));
    }

    return result;
}

QSet<int> FicSourceGRPCImpl::GetExpiredSnoozes(QHash<int, core::FanficSnoozeStatus> data)
{
    QSet<int> result;

    grpc::ClientContext context;

    ProtoSpace::SnoozeInfoRequest task;

    QScopedPointer<ProtoSpace::SnoozeInfoResponse> response (new ProtoSpace::SnoozeInfoResponse);
    std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(this->deadline);
    context.set_deadline(deadline);
    FillControlStruct(task.mutable_controls());
    for(const auto& snoozeInfo : data)
    {
        auto snooze = task.add_snoozes();
        snooze->set_fic_id(snoozeInfo.ficId);
        snooze->set_chapter_added(snoozeInfo.snoozedAtChapter);
        snooze->set_until_chapter(snoozeInfo.snoozedTillChapter);
        snooze->set_until_finished(snoozeInfo.untilFinished);
    }

    grpc::Status status = stub_->GetExpiredSnoozes(&context, task, response.data());

    ProcessStandardError(status);

    if(!response->success())
        return result;
    for(auto i = 0; i < response->expired_snoozes_size(); i++)
        result.insert(response->expired_snoozes(i));

    return result;

}

void FicSourceGRPCImpl::FillControlStruct(ProtoSpace::ControlInfo *controls)
{
    controls->set_user_token(proto_converters::TS(userToken));
    controls->set_application_token(proto_converters::TS(applicationToken));
}

FicSourceGRPC::FicSourceGRPC(QString connectionString,
                             QString userToken,
                             int deadline): impl(new FicSourceGRPCImpl(connectionString, deadline))
{
    impl->userToken = userToken;
    impl->applicationToken = userToken;
}

FicSourceGRPC::~FicSourceGRPC()
{

}
void FicSourceGRPC::FetchData(const core::StoryFilter &filter, QVector<core::Fanfic> *fics)
{
    if(!impl)
        return;
    impl->userData = userData;
    impl->FetchData(filter, fics);
}

void FicSourceGRPC::FetchFic(int ficId, QVector<core::Fanfic> *fics, core::StoryFilter::EUseThisFicType idType)
{
    if(!impl)
        return;
    impl->FetchFic(ficId, fics, idType);
}

int FicSourceGRPC::GetFicCount(const core::StoryFilter &filter)
{
    if(!impl)
        return 0;
    impl->userData = userData;
    return impl->GetFicCount(filter);
}

bool FicSourceGRPC::GetFandomListFromServer(int lastFandomID, QVector<core::Fandom> *fandoms)
{
    if(!impl)
        return false;
    return impl->GetFandomListFromServer(lastFandomID, fandoms);
}

core::DiagnosticsForReclist FicSourceGRPC::GetDiagnosticsForRecListFromServer(QSharedPointer<core::RecommendationList> recList)
{
    if(!impl)
        return {};
    return impl->GetDiagnosticsForRecommendationListFromServer(recList);
}

bool FicSourceGRPC::GetRecommendationListFromServer(QSharedPointer<core::RecommendationList> recList)
{
    if(!impl)
        return false;
    return impl->GetRecommendationListFromServer(recList);
}

bool FicSourceGRPC::GetInternalIDsForFics(QVector<core::Identity> * ficList)
{
    if(!impl)
        return false;
    return impl->GetInternalIDsForFics(ficList);
}

bool FicSourceGRPC::GetFFNIDsForFics(QVector<core::Identity> * ficList)
{
    if(!impl)
        return false;
    return impl->GetFFNIDsForFics(ficList);
}

std::optional<core::FavListDetails> FicSourceGRPC::GetStatsForFicList(QVector<core::Identity> ficList)
{
    if(!impl)
        return {};
    return impl->GetStatsForFicList(ficList);
}

QHash<uint32_t, uint32_t> FicSourceGRPC::GetAuthorsForFicList(QSet<int> ficList)
{
    if(!impl)
        return {};
    return impl->GetAuthorsForFicList(ficList);
}

QSet<int> FicSourceGRPC::GetAuthorsForFicInRecList(int sourceFic, QString authors)
{
    if(!impl)
        return {};
    return impl->GetAuthorsForFicInRecList(sourceFic, authors);
}

QHash<int, core::FavouritesMatchResult > FicSourceGRPC::GetMatchesForUsers(int sourceUser, QList<int> users)
{
    if(!impl)
        return {};
    return impl->GetMatchesForUsers(sourceUser, users);
}
QHash<int, core::FavouritesMatchResult> FicSourceGRPC::GetMatchesForUsers(InputsForMatches data, QList<int> users)
{
    if(!impl)
        return {};
    return impl->GetMatchesForUsers(data, users);
}

QSet<int> FicSourceGRPC::GetExpiredSnoozes(QHash<int, core::FanficSnoozeStatus> data)
{
    if(!impl)
        return {};
    return impl->GetExpiredSnoozes(data);
}
ServerStatus FicSourceGRPC::GetStatus()
{
    if(!impl)
        return ServerStatus();
    return impl->GetStatus();
}

void FicSourceGRPC::SetUserToken(QString token)
{
    impl->userToken = token;
}

void FicSourceGRPC::ClearUserData()
{
    impl->userData.Clear();
}



bool VerifyString(const std::string& s, uint maxSize = 200){
    if(s.length() > maxSize)
        return false;
    return true;
}

bool VerifyInt(const int& val, int maxValue = 100000){
    if(val > maxValue)
        return false;
    return true;
}
template<typename T>
bool VerifyVectorSize(const T& vector, int maxSize = 10000){
    if(vector.size() > maxSize)
        return false;
    return true;
}

bool VerifyNotEmpty(const int& val){
    if(val == 0)
        return false;
    return true;
}

bool VerifyIDPack(const ::ProtoSpace::SiteIDPack& idPack, ProtoSpace::ResponseInfo* info)
{
    bool isValid = true;
    if(idPack.ffn_ids().size() == 0 && idPack.ao3_ids().size() == 0 && idPack.sb_ids().size() == 0 && idPack.sv_ids().size() == 0 && idPack.db_ids().size() == 0 )
        isValid = false;
    if(idPack.ffn_ids().size() > 10000)
        isValid = false;
    if(idPack.ao3_ids().size() > 10000)
        isValid = false;
    if(idPack.sb_ids().size() > 10000)
        isValid = false;
    if(idPack.sv_ids().size() > 10000)
        isValid = false;
    if(!isValid)
    {
        SetFicIDSyncDataError(info);
        return false;
    }
    return true;
}


bool VerifyRecommendationsRequest(const ProtoSpace::RecommendationListCreationRequest* request,  ProtoSpace::ResponseInfo* info)
{
    if(request->data().list_name().size() > 100)
        return false;
    if(!request->data().general_params().is_automatic() &&
            (request->data().general_params().min_fics_to_match() <= 0 || request->data().general_params().min_fics_to_match() > 10000))
        return false;
    if(!request->data().general_params().is_automatic()
            && request->data().general_params().max_unmatched_to_one_matched() <= 0)
        return false;
//    if(request->data().general_params().always_pick_at() < 1)
//        return false;
    if(!VerifyIDPack(request->data().id_packs(), info))
        return false;
    return true;
}

bool VerifyFilterData(const ProtoSpace::Filter& filter, const ProtoSpace::UserData& user)
{
    if(!VerifyString(filter.basic_filters().website(), 10))
        return false;
    if(!VerifyInt(filter.size_limits().record_page()))
        return false;
    if(!VerifyInt(filter.genre_filter().genre_exclusion_size(), 20))
        return false;
    if(!VerifyInt(filter.genre_filter().genre_inclusion_size(), 20))
        return false;
    if(!VerifyInt(filter.content_filter().word_exclusion_size(), 50))
        return false;
    if(!VerifyInt(filter.content_filter().word_inclusion_size(), 50))
        return false;
    //    if(!VerifyInt(filter.tag_filter().all_tagged_size(), 50000))
    //        return false;
    //    if(!VerifyInt(filter.tag_filter().active_tags_size(), 50000))
    //        return false;
    if(!VerifyInt(user.recommendation_list().list_of_fics_size(), 1000000))
        return false;
    if(!VerifyInt(user.recommendation_list().list_of_matches_size(), 1000000))
        return false;
    if(!VerifyInt(filter.explicit_filter().slash().fandom_exceptions_size(), 20000))
        return false;

    //if(filter.tag_filter())

    return true;
}


