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
#include "querybuilder.h"
#include "pure_sql.h"
#include "Interfaces/db_interface.h"
#include "filters/date_filter.h"
#include "GlobalHeaders/snippets_templates.h"
#include <QDebug>

namespace  core{
QString WrapTag(QString tag)
{
    tag= "(.{0,}" + tag + ".{0,})";
    return tag;
}

DefaultQueryBuilder::DefaultQueryBuilder(bool client, QString userToken)
{
    query = NewQuery();
    InitTagFilterBuilder(client, userToken);
}

QSharedPointer<Query> DefaultQueryBuilder::Build(StoryFilter filter,
                                                 bool createLimits)
{
    query = NewQuery();

    queryString.clear();
    bool scoreSorting = filter.sortMode *in(StoryFilter::sm_metascore, StoryFilter::sm_minimize_dislikes, StoryFilter::sm_gems);

    bool useRecommendationFiltering = scoreSorting || filter.listOpenMode;
    useRecommendationFiltering = useRecommendationFiltering && filter.recommendationsCount > 0;
    bool useRecommendationOrdering = useRecommendationFiltering && !filter.listOpenMode;

    bool useScoresOrdering = filter.sortMode == StoryFilter::sm_userscores;

    if(createLimits)
    {
        queryString = "ID, ";
        queryString+= CreateCustomFields(filter);
        queryString+=  + " f.* ";
    }
    else
    {
        queryString = " f.ID as id ";
        if(useRecommendationOrdering)
        {
            queryString += " , cfRecommendationsMetascore(f.id) as sumrecs ";
            if(filter.sortMode == StoryFilter::sm_gems)
                queryString += " , cfRecommendationsPureVotes(f.id) as sumvotes ";
            queryString = queryString.arg(userToken);
        }
        if(useScoresOrdering)
        {
            queryString += " , cfScoresMatchCount(f.id) as scores ";
            queryString = queryString.arg(userToken);
        }
    }

    queryString+=" from vFanfics f " ;

    QString where = CreateWhere(filter);

    ProcessBindings(filter, query);


    //where+= CreateLimitQueryPart(filter);

    if(!where.trimmed().isEmpty() || useRecommendationFiltering || useScoresOrdering)
    {
        if(useRecommendationFiltering)
        {

            QString temp = " and cfInRecommendations(f.id) > 0 ";
            where = temp + where;
        }

        QString randomizer = ProcessRandomization(filter, where);

        QRegularExpression rx("^\\s{0,10}(and|AND)");
        auto match = rx.match(where);
        if(match.hasMatch())
        {
            //qDebug() << "FOUND MATCH";
            where = where.mid(match.captured().length());
        }

        if(!filter.randomizeResults)
            queryString += " where " + where;
        else
        {
            auto match = rx.match(randomizer);
            if(match.hasMatch())
            {
                //qDebug() << "FOUND MATCH";
                randomizer = randomizer.mid(match.captured().length());
            }
            queryString +=  " where " + randomizer;
        }

        if(createLimits)
        {
            queryString += BuildSortMode(filter);
            queryString += CreateLimitQueryPart(filter, false);
        }
    }
    else
    {
        QString randomizer = ProcessRandomization(filter, where);
        queryString += where + randomizer;
        if(createLimits)
            queryString += BuildSortMode(filter) + CreateLimitQueryPart(filter);
    }

    query->str = "select " + queryString.toStdString();
    queryString.replace(" fid ", " f.id ");
    queryString.replace(" fid)", " f.id)");
    queryString.replace("(fid", "(f.id");
    qDebug().noquote() << "Created query is:" << QString::fromStdString(query->str);


    return query;
}

QString DefaultQueryBuilder::CreateCustomFields(StoryFilter filter)
{
    QString queryString;
    //queryString+=ProcessSumFaves(filter);
    queryString+=ProcessFandoms(filter);
    queryString+=ProcessSumRecs(filter);
    if(filter.sortMode == StoryFilter::sm_gems)
        queryString+=ProcessSumVotes(filter);
    queryString+=ProcessScores(filter);
    queryString+=ProcessTags(filter);
    queryString+=ProcessUrl(filter);
    queryString+=ProcessGenreValues(filter);
    return queryString;
}

QString DefaultQueryBuilder::CreateWhere(StoryFilter filter,
                                         bool usePageLimiter)
{
    Q_UNUSED(usePageLimiter)
    QString queryString;

    queryString+= ProcessWordcount(filter);
    queryString+= ProcessRating(filter);
    queryString+= ProcessOtherFandomsMode(filter);
    queryString+= ProcessSlashMode(filter);
    queryString+= ProcessGenreIncluson(filter);
    queryString+= ProcessWordInclusion(filter);
    queryString+= ProcessDateRange(filter);
    queryString+= ProcessBias(filter);
    queryString+= ProcessWhereSortMode(filter);
    queryString+= ProcessActiveRecommendationsPart(filter);

    if(filter.minFavourites > 0)
        queryString += " and favourites > :favourites ";

    queryString+= ProcessStatusFilters(filter);
    queryString+= ProcessNormalOrCrossover(filter);
    queryString+= ProcessAuthor(filter);
    queryString+= ProcessFicID(filter);
    queryString+= ProcessRecommenders(filter);
    queryString+= ProcessSnoozes(filter);
    queryString+= tagFilterBuilder->GetString(filter);
    queryString+= ignoredFandomsBuilder->GetString(filter);
    //queryString+= ProcessFandomIgnore(filter);
    queryString+= ProcessCrossovers(filter);


    return queryString;
}

QString DefaultQueryBuilder::ProcessBias(StoryFilter filter)
{
    QString result;
    if(filter.reviewBias == StoryFilter::bias_none)
        return result;
    if(filter.reviewBias == StoryFilter::bias_favor)
        result += QString(" and ");
    else
        result += QString(" and not ");

    if(filter.biasOperator == StoryFilter::bias_more)
        result += QString(" reviewstofavourites > ");
    else
        result += QString(" reviewstofavourites < ");
    result += QString::number(filter.reviewBiasRatio);
    return result;
}

QString DefaultQueryBuilder::ProcessExpiration(StoryFilter)
{
    return "";
}

QString DefaultQueryBuilder::ProcessSumFaves(StoryFilter)
{
    QString sumOfAuthorFavourites = " (SELECT sumfaves FROM recommenders where name = f.author) as sumfaves, \n";
    return sumOfAuthorFavourites;
}

QString DefaultQueryBuilder::ProcessFandoms(StoryFilter)
{
    //return QString();
    QString fandoms = " "
            //"( select group_concat(name, ' & ') from fandomindex where id in (select fandom_id  from ficfandoms where fic_id = f.id)) as fandom, \n"
            "cast(fandom1 as text)||'::::'||cast(fandom2 as text)  as fandomids, \n"
            "";
    return fandoms;
}

QString DefaultQueryBuilder::ProcessOtherFandomsMode(StoryFilter filter, bool renameToFID)
{
    QString queryString;
    if(filter.otherFandomsMode)
    {
        queryString += QString(" and cfInIgnoredFandoms(f.fandom1,f.fandom2) > 0");
    }
    //        queryString += " and not exists ("
    //                       "select fandom_id from ficfandoms where fic_id = f.id and fandom_id in "
    //                       "(select id from fandomindex where name in (select distinct fandom from recent_fandoms)"
    //                       "))" ;

    if(!renameToFID)
        queryString.replace(" fid ", " ff.id ");

    return queryString;
}

// not exactly what its supposed to do but okay query to save
//QString currentRecTagValue = " ((SELECT match_count FROM RecommendationListData rfs where rfs.fic_id = f.id and rfs.list_id = 1)* "
//        " ( case when (select (select max(average_faves_top_3) from fandoms)/(select max(average_faves_top_3) from fandoms fs where fs.fandom in (f.fandom1, f.fandom2) )) > 3  then 1.5 "
//        " when (select (select max(average_faves_top_3) from fandoms)/(select max(average_faves_top_3) from fandoms fs where fs.fandom in (f.fandom1, f.fandom2) )) > 15 then 2 "
//        " else 1 end))  as sumrecs, ";

QString DefaultQueryBuilder::ProcessSumRecs(StoryFilter , bool )
{

    QString result;
    result = QString(" cfRecommendationsMetascore(f.id) as sumrecs, ");

    return result;
}

QString DefaultQueryBuilder::ProcessSumVotes(StoryFilter , bool )
{

    QString result;
    result = QString(" cfRecommendationsPureVotes(f.id) as sumvotes, ");

    return result;
}

QString DefaultQueryBuilder::ProcessScores(StoryFilter, bool )
{
    QString result;
    result = QString(" cfScoresMatchCount(f.id) as scores, ");

    return result;
}

QString DefaultQueryBuilder::ProcessTags(StoryFilter)
{
    QString result;
    result = " '' as tags , \n";
    return result;
}

QString DefaultQueryBuilder::ProcessAuthor(StoryFilter filter)
{
    QString result;
    if(filter.useThisAuthor == -1)
        return result;
    QStringList authorList;
    //    for(auto author : filter.usedAuthors)
    //        authorList.push_back(QString::number(author));

    result = QString(" and f.author_id = %1 ").arg(QString::number(filter.useThisAuthor ));
    return result;
}

QString DefaultQueryBuilder::ProcessFicID(StoryFilter filter)
{
    QString result;
    // need to split into db and ffn ids
    QStringList dbIds;
    QStringList ffnIds;
    std::for_each(filter.exactFicIds.begin(),filter.exactFicIds.end(), [&](const auto& fic){
        if(fic.idType == core::StoryFilter::EUseThisFicType::utf_db_id)
            dbIds.push_back(QString::number(fic.id));
        else
            ffnIds.push_back(QString::number(fic.id));
    });

    if(ffnIds.size() == 0 && dbIds.size() == 0)
        return result;

    QString ffnIdPart = ffnIds.size() > 0 ? QString(" f.ffn_id in ('%1') ").arg(ffnIds.join("','")) : QString("");
    QString dbIdPart = dbIds.size() > 0 ? QString(" f.id in ('%1') ").arg(dbIds.join("','")) : QString("");;
    QStringList sum;
    if(!ffnIdPart.isEmpty())
        sum.push_back(ffnIdPart);
    if(!dbIdPart.isEmpty())
        sum.push_back(dbIdPart);
    return " and ( " + sum.join(" OR ") + " ) " ;

    return result;
}
QString DefaultQueryBuilder::ProcessRecommenders(StoryFilter filter)
{
    QString result;
    if(filter.usedRecommenders.size() == 0)
        return result;

    result = " and cfInFicsForAuthors(f.id) > 0 ";
    return result;

}

QString DefaultQueryBuilder::ProcessSnoozes(StoryFilter filter)
{
    QString result;
    if(filter.displaySnoozedFics)
        return result;

    result = " and cfInSnoozes(f.id) < 1 ";
    return result;
}

QString DefaultQueryBuilder::ProcessUrl(StoryFilter)
{
    QString currentTagValue = " f.ffn_id as url, ";
    return currentTagValue;
}

QString DefaultQueryBuilder::ProcessGenreValues(StoryFilter filter)
{
    if(filter.sortMode != StoryFilter::sm_genrevalues)
        return QString();
    QString result = " (SELECT  %1  FROM FicGenreStatistics where fic_id = f.id) as genrevalue, \n";
    result = result.arg(filter.genreSortField);
    return result;
}

QString DefaultQueryBuilder::ProcessWordcount(StoryFilter filter)
{
    QString queryString;
    if(filter.minWords > 0)
        queryString += " and wordcount >= :minwordcount ";
    if(filter.maxWords > 0)
        queryString += " and wordcount <= :maxwordcount ";
    return queryString;
}

QString DefaultQueryBuilder::ProcessRating(StoryFilter filter)
{
    QString queryString;
    if(filter.rating == StoryFilter::rt_t_m)
        queryString += "";
    if(filter.rating == StoryFilter::rt_t)
        queryString += " and (rated <> 'M') ";
    if(filter.rating == StoryFilter::rt_m)
        queryString += " and (rated = 'M') ";
    return queryString;
}

QString DefaultQueryBuilder::ProcessSlashMode(StoryFilter filter, bool renameToFID)
{
    QString queryString;
    QString slashField;
    QStringList excludeFields;
    {
        if(!filter.slashFilter.slashFilterEnabled)
            return queryString;
        if(!filter.slashFilter.excludeSlash && !filter.slashFilter.includeSlash)
            return queryString;
        QString start = " and "  ;
        if(filter.slashFilter.includeSlash)
            start = start.arg("  ");
        else
            start = start.arg(" not  ");
        if(filter.slashFilter.slashFilterLevel == 0)
        {
            excludeFields = QStringList{"f.filter_pass_1","f.filter_pass_2"};
            slashField = "f.keywords_result";
        }
        else if(filter.slashFilter.slashFilterLevel == 1)
        {
            excludeFields = QStringList{"f.keywords_result","f.filter_pass_2"};
            slashField = "f.filter_pass_1";
        }
        else
        {
            excludeFields = QStringList{"f.keywords_result","f.filter_pass_1"};
            slashField = "f.filter_pass_2";
        }

        if(filter.slashFilter.excludeSlash)
        {
            if(filter.slashFilter.slashFilterLevel == 2 && filter.slashFilter.onlyMatureForSlash)
                queryString += "  not ( f.filter_pass_1 == 1 or ( %1 == 1 and f.rated = 'M')) ";
            else
                queryString += "  (%1 = 0) ";
        }
        if(filter.slashFilter.includeSlash)
        {
            if(!filter.slashFilter.onlyExactLevel)
            {
                if(filter.slashFilter.slashFilterLevel == 2 && filter.slashFilter.onlyMatureForSlash)
                    queryString += "  ( f.filter_pass_1 == 1 or ( %1 == 1 and f.rated = 'M')) ";
                else
                    queryString += " %1 = 1 ";
            }
            else
                queryString += " %1 = 1 and %2 = 0 and %3 = 0 ";
        }

        if(filter.slashFilter.enableFandomExceptions && filter.slashFilter.excludeSlash)
        {
            queryString += " and not exists (select fandom_id from ignored_fandoms_slash_filter where fandom_id in (select fandom_id from ficfandoms ffd where ffd.fic_id = f.id )) ";
        }
        //queryString+= ")";
        queryString = start + queryString;
    }
    if(!filter.slashFilter.onlyExactLevel)
        queryString = queryString.arg(slashField);
    else
        queryString = queryString.arg(slashField,excludeFields.first(),excludeFields.last());
    if(!renameToFID)
    {
        queryString.replace(" fid ", " ff.id ");
    }
    return queryString;
}

QString DefaultQueryBuilder::ProcessGenreIncluson(StoryFilter filter)
{
    QString queryString;
    if(!filter.useRealGenres)
    {
        if(filter.genreInclusion.size() > 0)
        {
            int counter = 0;
            for(const auto& genre : std::as_const(filter.genreInclusion))
            {
                Q_UNUSED(genre);
                auto counter1 = ++counter;
                queryString += QString(" AND f.genres like '%'||:genreinc%1||'%'").arg(QString::number(counter1));
            }
        }

        if(filter.genreExclusion.size() > 0)
        {
            int counter = 0;
            for(const auto& genre : std::as_const(filter.genreExclusion))
            {
                Q_UNUSED(genre);
                auto counter1 = ++counter;
                queryString += QString(" AND f.genres not like '%'||:genreexc%1||'%'").arg(QString::number(counter1));
            }
        }
    }
    else
    {


        if(filter.genreInclusion.size() > 0)
        {
            double limiter = 0;
            if(filter.genrePresenceForInclude == StoryFilter::gp_minimal)
                limiter = 0.2;
            else if(filter.genrePresenceForInclude == StoryFilter::gp_medium)
                limiter = 0.5;
            else if(filter.genrePresenceForInclude == StoryFilter::gp_considerable)
                limiter = 0.8;

            int counter = 0;
            QStringList genreResult;
            for(const auto& genre : std::as_const(filter.genreInclusion))
            {
                Q_UNUSED(genre);
                auto counter1 = ++counter;
                genreResult.push_back(QString(" true_genre1  like '%'||:genreinc%1||'%' and  true_genre1_percent/max_genre_percent > %2  ")
                                      .arg(QString::number(counter1++),QString::number(limiter)));
                genreResult.push_back(QString(" true_genre2  like '%'||:genreinc%1||'%' and  true_genre2_percent/max_genre_percent > %2  ")
                                      .arg(QString::number(counter1++),QString::number(limiter)));
                genreResult.push_back(QString(" true_genre3  like '%'||:genreinc%1||'%' and  true_genre3_percent/max_genre_percent > %2 ")
                                      .arg(QString::number(counter1++),QString::number(limiter)));

            }
            if(genreResult.size() > 0)
                queryString += QString(" AND ( ") + genreResult.join(" OR ") + QString(" ) ");
        }

        if(filter.genreExclusion.size() > 0)
        {
            double limiter = 0;
            if(filter.genrePresenceForExclude == StoryFilter::gp_minimal)
                limiter = 0.5;
            else if(filter.genrePresenceForExclude == StoryFilter::gp_medium)
                limiter = 0.8;
            else if(filter.genrePresenceForExclude == StoryFilter::gp_none)
                limiter = 0.05;

            int counter = 0;
            QStringList genreResult;
            for(const auto& genre : std::as_const(filter.genreExclusion))
            {
                Q_UNUSED(genre);
                auto counter1 = ++counter;
                genreResult.push_back(QString(" true_genre1  like '%'||:genreexc%1||'%' and  true_genre1_percent/max_genre_percent > %2  ")
                                      .arg(QString::number(counter1++),QString::number(limiter)));
                genreResult.push_back(QString(" true_genre2  like '%'||:genreexc%1||'%' and  true_genre2_percent/max_genre_percent > %2  ")
                                      .arg(QString::number(counter1++),QString::number(limiter)));
                genreResult.push_back(QString(" true_genre3  like '%'||:genreexc%1||'%' and  true_genre3_percent/max_genre_percent > %2 ")
                                      .arg(QString::number(counter1++),QString::number(limiter)));
            }
            if(genreResult.size() > 0)
                queryString += QString(" AND not ( ") + genreResult.join(" OR ") + QString(" ) ");

        }
    }
    return queryString;
}

QString DefaultQueryBuilder::ProcessWordInclusion(StoryFilter filter)
{
    QString queryString;
    if(filter.wordInclusion.size() > 0)
    {
        int counter = 0;
        for(const auto& word: std::as_const(filter.wordInclusion))
        {
            if(word.trimmed().isEmpty())
                continue;
            auto counter1 = ++counter;
            auto counter2 = ++counter;
            queryString += QString(" AND (summary like '%'||:incword%1||'%' "
                                   "or title like '%'||:incword%2||'%') ")

                    .arg(QString::number(counter1),QString::number(counter2));
        }
    }
    if(filter.wordExclusion.size() > 0)
    {
        int counter = 0;
        for(const auto& word: std::as_const(filter.wordExclusion))
        {
            if(word.trimmed().isEmpty())
                continue;
            auto counter1 = ++counter;
            auto counter2 = ++counter;
            queryString += QString(" AND summary not like '%'||:excword%1||'%' and title not like '%'||:excword%2||'%'")
                    .arg(QString::number(counter1),QString::number(counter2));
        }
    }
    return queryString;
}

QString DefaultQueryBuilder::ProcessDateRange(StoryFilter filter)
{
    if(filter.ficDateFilter.mode == filters::dft_none)
        return "";
    QString queryString = " %1 between '%2' and '%3' ";
    if(filter.ficDateFilter.mode == filters::dft_published){
        queryString = queryString.arg("published");
        queryString = queryString.arg(QString::fromStdString(filter.ficDateFilter.dateStart),QString::fromStdString(filter.ficDateFilter.dateEnd));
        queryString = " and " + queryString;
    }
    else {
        queryString = queryString.arg("updated");
        queryString += " and complete = 1 ";
        queryString = queryString.arg(QString::fromStdString(filter.ficDateFilter.dateStart),QString::fromStdString(filter.ficDateFilter.dateEnd));
        queryString = " and ( " + queryString + " ) ";
    }
    return queryString;
}

QString DefaultQueryBuilder::ProcessActiveRecommendationsPart(StoryFilter filter)
{
    QString queryString;
    if(filter.mode == core::StoryFilter::filtering_in_recommendations && filter.useThisRecommenderOnly != -1)
    {
        QString qsl = " and id in (select fic_id from recommendations %1)";
        qsl=qsl.arg(QString(" where recommender_id = %1 ").arg(QString::number(filter.useThisRecommenderOnly)));
        queryString+=qsl;
    }
    else if(filter.mode == core::StoryFilter::filtering_in_recommendations)
    {
        QString qsl = " and id in (select fic_id from RecommendationListData where list_id = %1)";
        qsl=qsl.arg(QString::number(filter.listForRecommendations));
        queryString+=qsl;
    }
    return queryString;
}

QString DefaultQueryBuilder::ProcessWhereSortMode(StoryFilter filter)
{
    QString queryString;
    if(filter.sortMode == StoryFilter::sm_trending)
        queryString += " and ( favourites/(julianday(CURRENT_TIMESTAMP) - julianday(Published)) > " + QString::number(filter.recentAndPopularFavRatio) + " OR  favourites > 1000) ";

    if(filter.sortMode == StoryFilter::sm_trending)
        queryString+= " and published <> updated "
                      " and published > date('now', '-" + QString::number(filter.recentCutoff.date().daysTo(QDate::currentDate())) + " days') "
                                                                                                                                     " and published < date('now', '-" + QString::number(45) + " days') "
                                                                                                                                                                                               " and updated > date('now', '-60 days') ";

    //    if(filter.sortMode == StoryFilter::reccount)
    //        queryString += QString(" AND sumrecs > " + QString::number(filter.minRecommendations));
    //    else if(filter.minRecommendations > 0)
    //        queryString += QString(" AND sumrecs > " + QString::number(filter.minRecommendations));

    return queryString;
}

QString DefaultQueryBuilder::ProcessDiffField(StoryFilter filter)
{
    QString diffField;
    bool scoreSorting = filter.sortMode == StoryFilter::sm_metascore  || filter.sortMode == StoryFilter::sm_minimize_dislikes;

    if(filter.sortMode == StoryFilter::sm_wordcount)
        diffField = " WORDCOUNT";
    if(filter.sortMode == StoryFilter::sm_wcrcr)
        diffField = " wcr";
    else if(filter.sortMode == StoryFilter::sm_favourites)
        diffField = " FAVOURITES";
    else if(filter.sortMode == StoryFilter::sm_updatedate)
        diffField = " updated";
    else if(filter.sortMode == StoryFilter::sm_publisdate)
        diffField = " published";
    else if(scoreSorting)
        diffField = " sumrecs";
    else if(filter.sortMode == StoryFilter::sm_trending)
        diffField = " favourites/(julianday(CURRENT_TIMESTAMP) - julianday(Published))";
    else if(filter.sortMode == StoryFilter::sm_revtofav)
        diffField = " favourites /(reviews + 1)";
    else if(filter.sortMode == StoryFilter::sm_genrevalues)
        diffField = " genrevalue";
    else if(filter.sortMode == StoryFilter::sm_userscores)
        diffField = " scores ";
    else if(filter.sortMode == StoryFilter::sm_gems)
        diffField = " (cast(sumvotes as float)/cast(favourites + 15 as float)) * (cast(sumrecs as float)/cast(sumvotes as float)) ";
    diffField += filter.descendingDirection ? " DESC" : " ASC";

    return diffField;
}

QString DefaultQueryBuilder::ProcessStatusFilters(StoryFilter filter)
{
    QString queryString;
    QString activeString = "( cast("
                           "("
                           " strftime('%s',f.updated)-strftime('%s',CURRENT_TIMESTAMP) "
                           " ) AS real "
                           " )/60/60/24 >-%1 or f.complete = 1 )";
    activeString=activeString.arg(QString::number(filter.deadFicDaysRange));

    if(filter.ensureCompleted)
        queryString+=QString(" and  f.complete = 1");

    if(!filter.allowUnfinished)
        queryString+=QString(" and  ( f.complete = 1 or " + activeString + " )");

    if(!filter.allowNoGenre)
        queryString+=QString(" and  ( genres != 'not found' )");

    if(filter.ensureActive)
        queryString+=QString(" and " + activeString);
    return queryString;
}

QString DefaultQueryBuilder::ProcessNormalOrCrossover(StoryFilter filter, bool renameToFID)
{
    QString queryString;
    if(filter.fandom == -1)
        return queryString;
    if(filter.secondFandom == -1)
    {
        //todo this can be optimized if I pass fandom id directly
        queryString = " and ("
                      "f.fandom1 = :fandom_id1 "
                      "or f.fandom2 = :fandom_id2"
                      ")";
        if(!renameToFID)
            queryString.replace(" fid ", " ff.id ");
    }
    else {
        queryString += " and ("
                      "( f.fandom1 = :fandom_id1 "
                      "and f.fandom2 = :fandom_id2) "
                       " or "
                       "( f.fandom2 = :fandom_id1_ "
                       "and f.fandom1 = :fandom_id2_) "
                      ")";
        if(!renameToFID)
            queryString.replace(" fid ", " ff.id ");

    }

    return queryString;

}

QString DefaultQueryBuilder::ProcessFilteringMode(StoryFilter filter)
{
    QString queryString;
    QString part =  "'" + filter.activeTags.join("','") + "'";
    if(filter.mode == core::StoryFilter::filtering_in_fics && !filter.activeTags.isEmpty())
        queryString += QString(" and exists (select fic_id from fictags where tag in (%1) and fic_id = f.id) ").arg(part);
    else
    {
        if(filter.ignoreAlreadyTagged)
            queryString += QString("");
        else
            queryString += QString(" and f.hidden = 0 ");

    }
    return queryString;
}

QString DefaultQueryBuilder::ProcessFandomIgnore(StoryFilter filter)
{
    QString queryString;
    {
        if(filter.ignoreFandoms)
            queryString += QString(" and (not ("
                                   "(select case (select count(fandom_id) from ficfandoms where fic_id = f.id) when 1"
                                   "   then (select count(fandom_id) from ignored_fandoms ignf where ignf.fandom_id in (select fandom_id from ficfandoms where fic_id = f.id))"
                                   "   else (select count(fandom_id) from ignored_fandoms ignf where ignf.including_crossovers = 1 and ignf.fandom_id in (select fandom_id from ficfandoms where fic_id = f.id)) end"
                                   " ) > 0  "
                                   "or ( (select count(fandom_id) from ficfandoms where fic_id = f.id) > 1 ) "
                                   "and  (select fandom_id from ficfandoms where fic_id = f.id limit 1) in (select fandom_id from ignored_fandoms) "
                                   "and  (select fandom_id from ficfandoms where fic_id = f.id limit 1 offset 1) in (select fandom_id from ignored_fandoms) "
                                   "))");

        else
            queryString += QString("");
    }
    return queryString;
}

QString DefaultQueryBuilder::ProcessCrossovers(StoryFilter filter)
{
    QString queryString;
    {
        if(filter.crossoversOnly)
            queryString += QString(" and (select count(fandom_id) from ficfandoms where fic_id = f.id) > 1 ");
        else if (!filter.includeCrossovers)
            queryString += QString(" and (select count(fandom_id) from ficfandoms where fic_id = f.id) = 1 ");

        else
            queryString += QString("");
    }
    return queryString;
}


QString DefaultQueryBuilder::ProcessRandomization(StoryFilter filter, QString wherePart)
{
    QString result;
    if(!filter.randomizeResults)
        return result;
    QStringList idList;
    QString part = "  and ID IN ( %1 ) ";
    wherePart = " cfRecommendationsMetascore(f.id) as sumrecs, cfScoresMatchCount(f.id) as scores,  1 as junk from fanfics f where 1 = 1 " + wherePart;
    wherePart = wherePart.arg(userToken);
    wherePart.replace("COLLATE NOCASE", "");
    wherePart+=" COLLATE NOCASE";
    //    for(int i = 0; i < filter.maxFics; i++)
    //    {
    if(rng)
    {
        auto q = NewQuery();
        q->bindings = query->bindings;
        q->str = wherePart.toStdString();

        auto values = rng->Get(q, userToken, db, filter);
        if(values.size() == 0)
            return "";
        idList=values;
    }
    //    }
    if(idList.size() == 0)
        return result;
    result = part.arg(idList.join(","));
    return result;
}

void DefaultQueryBuilder::ProcessBindings(StoryFilter filter,
                                          QSharedPointer<Query> q)
{
    if(filter.minWords > 0)
        q->bindings.push_back({"minwordcount",filter.minWords});
    if(filter.maxWords > 0)
        q->bindings.push_back({"maxwordcount", filter.maxWords});
    if(filter.minFavourites> 0)
        q->bindings.push_back({"favourites",filter.minFavourites});
    if(filter.listForRecommendations > -1)
    {
        q->bindings.push_back({"list_id",filter.listForRecommendations});
        q->bindings.push_back({"list_id2",filter.listForRecommendations});
    }
    //    if(filter.minRecommendations > -1 && filter.listOpenMode)
    //        q->bindings.push_back({"match_count",filter.minRecommendations});
    //    else if (filter.minRecommendations > -1)
    //        q->bindings.push_back({"match_count",filter.minRecommendations + 1});
    if(!filter.useRealGenres)
    {
        if(!filter.genreInclusion.isEmpty())
        {
            int counter = 1;
            for(const auto& genre : std::as_const(filter.genreInclusion))
                q->bindings.push_back({"genreinc" + QString::number(counter++).toStdString(),genre});
        }
        if(!filter.genreExclusion.isEmpty())
        {
            int counter = 1;
            for(const auto& genre : std::as_const(filter.genreExclusion))
                q->bindings.push_back({"genreexc" + QString::number(counter++).toStdString(),genre});
        }
    }
    else
    {
        if(!filter.genreInclusion.isEmpty())
        {
            int counter = 1;
            for(const auto& genre : std::as_const(filter.genreInclusion))
            {
                q->bindings.push_back({"genreinc" + QString::number(counter++).toStdString(),genre});
                q->bindings.push_back({"genreinc" + QString::number(counter++).toStdString(),genre});
                q->bindings.push_back({"genreinc" + QString::number(counter++).toStdString(),genre});
            }
        }
        if(!filter.genreExclusion.isEmpty())
        {
            int counter = 1;
            for(const auto& genre : std::as_const(filter.genreExclusion))
            {
                q->bindings.push_back({"genreexc" + QString::number(counter++).toStdString(),genre});
                q->bindings.push_back({"genreexc" + QString::number(counter++).toStdString(),genre});
                q->bindings.push_back({"genreexc" + QString::number(counter++).toStdString(),genre});
            }
        }
    }
    if(filter.fandom != -1 && filter.secondFandom == -1)
    {
        q->bindings.push_back({"fandom_id1",filter.fandom});
        q->bindings.push_back({"fandom_id2",filter.fandom});
    }
    if(filter.fandom != -1 && filter.secondFandom != -1)
    {
        q->bindings.push_back({"fandom_id1",filter.fandom});
        q->bindings.push_back({"fandom_id2",filter.secondFandom});
        q->bindings.push_back({"fandom_id1_",filter.fandom});
        q->bindings.push_back({"fandom_id2_",filter.secondFandom});
    }
    if(!filter.wordInclusion.isEmpty())
    {
        int counter = 1;
        for(const auto& word: std::as_const(filter.wordInclusion))
        {
            if(word.trimmed().isEmpty())
                continue;
            q->bindings.push_back({"incword" + QString::number(counter++).toStdString(),word});
            q->bindings.push_back({"incword" + QString::number(counter++).toStdString(),word});
        }
    }
    if(!filter.wordExclusion.isEmpty())
    {
        int counter = 1;
        for(const auto& word: std::as_const(filter.wordExclusion))
        {
            if(word.trimmed().isEmpty())
                continue;
            q->bindings.push_back({"excword" + QString::number(counter++).toStdString(),word});
            q->bindings.push_back({"excword" + QString::number(counter++).toStdString(),word}); // todo change on DB switch
        }
    }
    if(filter.recordLimit > 0)
        q->bindings.push_back({"record_limit",filter.recordLimit});
    if(filter.recordPage > -1)
        q->bindings.push_back({"record_offset",filter.recordPage * filter.recordLimit});

}

void DefaultQueryBuilder::InitQuery()
{
    query = NewQuery();
}


QString DefaultQueryBuilder::BuildSortMode(StoryFilter filter)
{
    QString queryString;
    diffField = ProcessDiffField(filter);
    queryString+=" ORDER BY " + diffField;
    return queryString;
}

QString DefaultQueryBuilder::CreateLimitQueryPart(StoryFilter filter, bool collate)
{
    QString result;
    if(filter.recordLimit <= 0)
        return result;
    if(collate && !filter.randomizeResults)
        result = " COLLATE NOCASE ";
    QString limitOffset = QString(" %1 %2 ");
    if(!filter.randomizeResults && filter.recordLimit > 0)
        limitOffset = limitOffset.arg(" LIMIT :record_limit ");
    else
        limitOffset = limitOffset.arg("");
    if(!filter.randomizeResults && filter.recordPage != -1)
        limitOffset = limitOffset.arg(" OFFSET :record_offset");
    else
        limitOffset = limitOffset.arg("");
    if(!limitOffset.trimmed().isEmpty())
        result = result.prepend(limitOffset);
    return result;
}

QSharedPointer<Query> DefaultQueryBuilder::NewQuery()
{
    return QSharedPointer<Query>(new Query);
}

void DefaultQueryBuilder::InitTagFilterBuilder(bool client, QString userToken)
{
    if(client)
    {
        this->userToken = userToken;
        thinClientMode = true;
        tagFilterBuilder.reset(new TagFilteringClient);
        tagFilterBuilder->userToken = userToken;
        ignoredFandomsBuilder.reset(new FandomIgnoreClient);
        ignoredFandomsBuilder->userToken = userToken;
    }
    else
    {
        tagFilterBuilder.reset(new TagFilteringFullDB);
        ignoredFandomsBuilder.reset(new FandomIgnoreFullDB);
    }
}
CountQueryBuilder::CountQueryBuilder(bool client, QString userToken) : DefaultQueryBuilder(client, userToken)
{
}

QSharedPointer<Query> CountQueryBuilder::Build(StoryFilter filter, bool createLimits)
{
    // todo note : randomized queries don't need size queries as size is known beforehand
    QLOG_INFO_PURE() << "//////////";
    QLOG_INFO() << "BUILDING COUNT QUERY";
    QLOG_INFO_PURE() << "//////////";
    auto q = DefaultQueryBuilder::Build(filter, createLimits);
    q->str = "select count(*) as records from ("+ q->str +")";
    QLOG_INFO_PURE() << "//////////";
    QLOG_INFO_PURE() << "COUNT QUERY:" << QString::fromStdString(q->str);
    QLOG_INFO_PURE() << "//////////";
    return q;
}

QString CountQueryBuilder::ProcessWhereSortMode(StoryFilter filter)
{
    QString queryString;
    if(filter.sortMode == StoryFilter::sm_trending)
        queryString += " and ( favourites/(julianday(CURRENT_TIMESTAMP) - julianday(Published)) > " + QString::number(filter.recentAndPopularFavRatio) + " OR  favourites > 1000) ";

    if(filter.sortMode == StoryFilter::sm_trending)
        queryString+= " and published <> updated "
                      " and published > date('now', '-" + QString::number(filter.recentCutoff.date().daysTo(QDate::currentDate())) + " days') "
                                                                                                                                     " and published < date('now', '-" + QString::number(45) + " days') "
                                                                                                                                                                                               " and updated > date('now', '-60 days') ";

    // this doesnt require sumrecs as it's inverted
    return queryString;
}

IWhereFilter::~IWhereFilter()
{

}

TagFilteringFullDB::~TagFilteringFullDB()
{

}

QString TagFilteringFullDB::GetString(StoryFilter filter)
{
    QString queryString;
    QString part =  "'" + filter.activeTags.join("','") + "'";
    if(filter.mode == core::StoryFilter::filtering_in_fics && !filter.activeTags.isEmpty())
        queryString += QString(" and exists (select fic_id from fictags where tag in (%1) and fic_id = f.id) ").arg(part);
    else
    {
        if(filter.ignoreAlreadyTagged)
            queryString += QString("");
        else
            queryString += QString(" and f.hidden = 0 ");

    }
    return queryString;
}

TagFilteringClient::~TagFilteringClient()
{

}

QString TagFilteringClient::GetString(StoryFilter filter)
{
    QString queryString;
    if(filter.tagsAreUsedForAuthors)
    {
        queryString += QString(" and cfInLikedAuthors(f.author_id) > 0 ");
        if(!filter.ignoreAlreadyTagged)
            queryString += QString(" and cfInTags(f.id) < 1 ");
    }
    else
    {
        if(filter.mode == core::StoryFilter::filtering_in_fics && filter.activeTagsCount > 0)
            queryString += QString(" and cfInActiveTags(f.id) = 1 ");
        else
        {
            if(filter.ignoreAlreadyTagged || filter.allTagsCount == 0)
                queryString += QString("");
            else
                queryString += QString(" and cfInTags(f.id) < 1 ");
        }
    }


    return queryString;
}



FandomIgnoreFullDB::~FandomIgnoreFullDB()
{

}

QString FandomIgnoreFullDB::GetString(StoryFilter filter)
{
    QString queryString;
    {
        if(filter.ignoreFandoms)
            queryString += QString(" and (not ("
                                   "(select case (select count(fandom_id) from ficfandoms where fic_id = f.id) when 1"
                                   "   then (select count(fandom_id) from ignored_fandoms ignf where ignf.fandom_id in (select fandom_id from ficfandoms where fic_id = f.id))"
                                   "   else (select count(fandom_id) from ignored_fandoms ignf where ignf.including_crossovers = 1 "
                                   "   and ignf.fandom_id in (select fandom_id from ficfandoms where fic_id = f.id)) end"
                                   " ) > 0  "
                                   "or ( (select count(fandom_id) from ficfandoms where fic_id = f.id) > 1 ) "
                                   "and  (select fandom_id from ficfandoms where fic_id = f.id limit 1) in (select fandom_id from ignored_fandoms) "
                                   "and  (select fandom_id from ficfandoms where fic_id = f.id limit 1 offset 1) in (select fandom_id from ignored_fandoms) "
                                   "))");

        else
            queryString += QString("");
    }
    return queryString;
}

FandomIgnoreClient::~FandomIgnoreClient()
{

}

QString FandomIgnoreClient::GetString(StoryFilter filter)
{
    QString queryString;
    {
        if(filter.ignoreFandoms && filter.ignoredFandomCount > 0)
            queryString += QString(" and cfInIgnoredFandoms(f.fandom1,f.fandom2) < 1");
        else
            queryString += QString("");
    }
    return queryString;
}

}

