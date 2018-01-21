/*
FFSSE is a replacement search engine for fanfiction.net search results
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
#include "querybuilder.h"
#include "pure_sql.h"
#include "Interfaces/db_interface.h"
#include <QDebug>

namespace  core{
QString WrapTag(QString tag)
{
    tag= "(.{0,}" + tag + ".{0,})";
    return tag;
}

DefaultQueryBuilder::DefaultQueryBuilder()
{
    query = NewQuery();
}

QSharedPointer<Query> DefaultQueryBuilder::Build(StoryFilter filter)
{
    query = NewQuery();

    queryString.clear();
    queryString = "ID, ";
    queryString+= CreateCustomFields(filter) + " f.* ";

    queryString+=" from vFanfics f where alive = 1 " ;
    QString where = CreateWhere(filter);
    qDebug().noquote() << "WHERE IS: " << where;
    ProcessBindings(filter, query);
    where+= ProcessRandomization(filter, queryString);

    //where+= CreateLimitQueryPart(filter);
    bool useRecommendationFiltering = filter.sortMode == StoryFilter::reccount || filter.minRecommendations > 0;
    if(!where.trimmed().isEmpty() || useRecommendationFiltering)
    {
        if(useRecommendationFiltering)
        {
            QString temp = " and id in ( select distinct fic_id as fid from RecommendationListData rt left join fanfics ff  on ff.id = rt.fic_id and rt.list_id = :list_id2 where 1=1 ";
            if(!filter.showOriginsInLists)
                temp+=" and is_origin <> 1 ";
            temp += where + "order by rt.match_count desc" + CreateLimitQueryPart(filter) + ")";
            where = temp;
        }
        else
            where = " and id in ( select id as fid from fanfics ff where 1=1 " + where + BuildSortMode(filter) + CreateLimitQueryPart(filter) + ")";
        queryString += where + BuildSortMode(filter);
    }
    else
    {
        queryString += where + BuildSortMode(filter);
        queryString += CreateLimitQueryPart(filter);
    }



    query->str = "select " + queryString;
    qDebug().noquote() << query->str;
    return query;
}

QString DefaultQueryBuilder::CreateCustomFields(StoryFilter filter)
{
    QString queryString;
    //queryString+=ProcessSumFaves(filter);
    queryString+=ProcessFandoms(filter);
    queryString+=ProcessSumRecs(filter);
    queryString+=ProcessTags(filter);
    queryString+=ProcessUrl(filter);
    return queryString;
}

QString DefaultQueryBuilder::CreateWhere(StoryFilter filter,
                                         bool usePageLimiter)
{
    QString queryString;

    queryString+= ProcessWordcount(filter);
    queryString+= ProcessGenreIncluson(filter);
    queryString+= ProcessWordInclusion(filter);
    queryString+= ProcessBias(filter);
    queryString+= ProcessWhereSortMode(filter);
    queryString+= ProcessActiveRecommendationsPart(filter);

    if(filter.minFavourites > 0)
        queryString += " and favourites > :favourites ";

    queryString+= ProcessStatusFilters(filter);
    queryString+= ProcessNormalOrCrossover(filter);
    queryString+= ProcessFilteringMode(filter);

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

QString DefaultQueryBuilder::ProcessSumFaves(StoryFilter)
{
    QString sumOfAuthorFavourites = " (SELECT sumfaves FROM recommenders where name = f.author) as sumfaves, \n";
    return sumOfAuthorFavourites;
}

QString DefaultQueryBuilder::ProcessFandoms(StoryFilter filter)
{
    //return QString();
    QString fandoms = " ( select group_concat(name, ' & ') from fandomindex where id in (select fandom_id  from ficfandoms where fic_id = f.id)) as fandom, \n";
    return fandoms;
}

// not exactly what its supposed to do but okay query to save
//QString currentRecTagValue = " ((SELECT match_count FROM RecommendationListData rfs where rfs.fic_id = f.id and rfs.list_id = 1)* "
//        " ( case when (select (select max(average_faves_top_3) from fandoms)/(select max(average_faves_top_3) from fandoms fs where fs.fandom in (f.fandom1, f.fandom2) )) > 3  then 1.5 "
//        " when (select (select max(average_faves_top_3) from fandoms)/(select max(average_faves_top_3) from fandoms fs where fs.fandom in (f.fandom1, f.fandom2) )) > 15 then 2 "
//        " else 1 end))  as sumrecs, ";

QString DefaultQueryBuilder::ProcessSumRecs(StoryFilter filter, bool appendComma)
{

    QString currentRecTagValue = " (SELECT match_count FROM RecommendationListData rfs where rfs.fic_id = f.id and rfs.list_id = :list_id %1) as sumrecs";
    if(appendComma)
        currentRecTagValue+= ",";
    currentRecTagValue+= " \n";
    if(filter.showOriginsInLists)
        currentRecTagValue=currentRecTagValue.arg("");
    else
        currentRecTagValue=currentRecTagValue.arg("and is_origin <> 1");

    return currentRecTagValue;
}

QString DefaultQueryBuilder::ProcessTags(StoryFilter)
{
    QString currentTagValue = " (SELECT  group_concat(tag, ' ')  FROM fictags where fic_id = f.id order by tag asc) as tags, \n";
    return currentTagValue;
}

QString DefaultQueryBuilder::ProcessUrl(StoryFilter)
{
    QString currentTagValue = " f.ffn_id as url, ";
    return currentTagValue;
}

QString DefaultQueryBuilder::ProcessWordcount(StoryFilter filter)
{
    QString queryString;
    if(filter.minWords > 0)
        queryString += " and wordcount > :minwordcount ";
    if(filter.maxWords > 0)
        queryString += " and wordcount < :maxwordcount ";
    return queryString;
}

QString DefaultQueryBuilder::ProcessGenreIncluson(StoryFilter filter)
{
    QString queryString;
    if(filter.genreInclusion.size() > 0)
        for(auto genre : filter.genreInclusion)
            queryString += QString(" AND genres like '%%1%' ").arg(genre);

    if(filter.genreExclusion.size() > 0)
        for(auto genre : filter.genreExclusion)
            queryString += QString(" AND genres not like '%%1%' ").arg(genre);
    return queryString;
}

QString DefaultQueryBuilder::ProcessWordInclusion(StoryFilter filter)
{
    QString queryString;
    if(filter.wordInclusion.size() > 0)
    {
        for(QString word: filter.wordInclusion)
        {
            if(word.trimmed().isEmpty())
                continue;
            queryString += QString(" AND ((summary like '%%1%' and summary not like '%not %1%') or (title like '%%1%' and title not like '%not %1%') ) ").arg(word);
        }
    }
    if(filter.wordExclusion.size() > 0)
    {
        for(QString word: filter.wordExclusion)
        {
            if(word.trimmed().isEmpty())
                continue;
            queryString += QString(" AND summary not like '%%1%' and summary not like '%not %1%'").arg(word);
        }
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
    if(filter.sortMode == StoryFilter::favrate)
        queryString += " and ( favourites/(julianday(CURRENT_TIMESTAMP) - julianday(Published)) > " + QString::number(filter.recentAndPopularFavRatio) + " OR  favourites > 1000) ";

    if(filter.sortMode == StoryFilter::favrate)
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
    if(filter.sortMode == StoryFilter::wordcount)
        diffField = " WORDCOUNT DESC";
    if(filter.sortMode == StoryFilter::wcrcr)
        diffField = " (wcr ) asc";
    else if(filter.sortMode == StoryFilter::favourites)
        diffField = " FAVOURITES DESC";
    else if(filter.sortMode == StoryFilter::updatedate)
        diffField = " updated DESC";
    else if(filter.sortMode == StoryFilter::publisdate)
        diffField = " published DESC";
    else if(filter.sortMode == StoryFilter::reccount)
        diffField = " sumrecs desc";
    else if(filter.sortMode == StoryFilter::favrate)
        diffField = " favourites/(julianday(CURRENT_TIMESTAMP) - julianday(Published)) desc";
    return diffField;
}

QString DefaultQueryBuilder::ProcessStatusFilters(StoryFilter filter)
{
    QString queryString;
    QString activeString = " cast("
                           "("
                           " strftime('%s',f.updated)-strftime('%s',CURRENT_TIMESTAMP) "
                           " ) AS real "
                           " )/60/60/24 >-365";

    if(filter.ensureCompleted)
        queryString+=QString(" and  complete = 1");

    if(!filter.allowUnfinished)
        queryString+=QString(" and  ( complete = 1 or " + activeString + " )");

    if(!filter.allowNoGenre)
        queryString+=QString(" and  ( genres != 'not found' )");

    if(filter.ensureActive)
        queryString+=QString(" and " + activeString);
    return queryString;
}

QString DefaultQueryBuilder::ProcessNormalOrCrossover(StoryFilter filter)
{
    QString queryString;
    if(filter.fandom.trimmed().isEmpty())
        return queryString;
    //todo this can be optimized if I pass fandom id directly
    QString add = " and id in (select fic_id from ficfandoms where fandom_id = (select id from fandomindex where name = '%1')) ";
    queryString+=add.arg(filter.fandom);
    return queryString;

}

QString DefaultQueryBuilder::ProcessFilteringMode(StoryFilter filter)
{
    QString queryString;
    QString part =  "'" + filter.activeTags.join("','") + "'";
    if(filter.mode == core::StoryFilter::filtering_in_fics && !filter.activeTags.isEmpty())
        queryString += QString(" and exists (select fic_id from fictags where tag in (%1) and fic_id = ff.id) ").arg(part);
    else
    {
        if(filter.ignoreAlreadyTagged)
            queryString += QString("");
        else
            queryString += QString(" and not exists  (select fic_id from fictags where fic_id = fid)");

    }
    return queryString;
}


QString DefaultQueryBuilder::ProcessRandomization(StoryFilter filter, QString wherePart)
{
    QString result;
    if(!filter.randomizeResults)
        return result;
    QStringList idList;
    QString part = " and ID IN ( %1 ) ";
    for(int i = 0; i < filter.maxFics; i++)
    {
        if(rng)
        {
            auto q = NewQuery();
            q->bindings = query->bindings;
            q->str = wherePart;
            auto value = rng->Get(q, db);
            if(value == "-1")
                return "";
            idList+=value;
        }
    }
    if(idList.size() == 0)
        return result;
    result = part.arg(idList.join(","));
    return result;
}

void DefaultQueryBuilder::ProcessBindings(StoryFilter filter,
                                          QSharedPointer<Query> q)
{
    if(filter.minWords > 0)
        q->bindings[":minwordcount"] = filter.minWords;
    if(filter.maxWords > 0)
        q->bindings[":maxwordcount"] = filter.maxWords;
    if(filter.minFavourites> 0)
        q->bindings[":favourites"] = filter.minFavourites;
    if(filter.listForRecommendations > -1)
    {
        q->bindings[":list_id"] = filter.listForRecommendations;
        q->bindings[":list_id2"] = filter.listForRecommendations;
    }
    if(filter.minRecommendations > -1)
        q->bindings[":match_count"] = filter.minRecommendations;
    if(filter.recordLimit > 0)
        q->bindings[":record_limit"] = filter.recordLimit;
    if(filter.recordPage > -1)
        q->bindings[":record_offset"] = filter.recordPage * filter.recordLimit;
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

QString DefaultQueryBuilder::CreateLimitQueryPart(StoryFilter filter)
{
    QString result;
    if(filter.recordLimit <= 0)
        return result;
    result = " COLLATE NOCASE ";
    QString limitOffset = QString(" %1 %2 ");
    if(filter.recordLimit > 0)
        limitOffset = limitOffset.arg(" LIMIT :record_limit ");
    if(filter.recordPage != -1)
        limitOffset = limitOffset.arg(" OFFSET :record_offset");
    if(!limitOffset.trimmed().isEmpty())
        result = result.prepend(limitOffset);
    return result;
}

QSharedPointer<Query> DefaultQueryBuilder::NewQuery()
{
    return QSharedPointer<Query>(new Query);
}


QString DefaultRNGgenerator::Get(QSharedPointer<Query> query, QSqlDatabase )
{
    QString where = query->str;

    if(!randomIdLists.contains(where))
    {
        auto idList = portableDBInterface->GetIdListForQuery(query);
        if(idList.size() == 0)
            idList.push_back("-1");
        randomIdLists[where] = idList;
    }
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    auto& currentList = randomIdLists[where];
    std::uniform_int_distribution<> distr(0, currentList.size()-1); // define the range
    auto value = distr(eng);
    return currentList[value];
}

CountQueryBuilder::CountQueryBuilder()
{

}

QSharedPointer<Query> CountQueryBuilder::Build(StoryFilter filter)
{
    // todo note : randomized queries don't need size queries as size is known beforehand
    query = NewQuery();
    queryString.clear();

    // special cases that need to be optimised in order of necessity
    // recommendation list sorting can and needs to be inverted for instant results
    QString wrappignString =  "select count(fic_id) as records from RecommendationListData  where list_id = :list_id and match_count > :match_count and exists (%1)";
    QString normalString = "select count(id) as records %1 ";
    queryString = "  from vFanfics ff where alive = 1 " ;
    QString where;
    {
        where+= ProcessWordcount(filter);
        where+= ProcessGenreIncluson(filter);
        where+= ProcessWordInclusion(filter);
        where+= ProcessBias(filter);
        where+= ProcessWhereSortMode(filter);
        where+= ProcessActiveRecommendationsPart(filter);

        if(filter.minFavourites > 0)
            where += " and favourites > :favourites ";

        where+= ProcessStatusFilters(filter);
        where+= ProcessNormalOrCrossover(filter);
        where+= ProcessFilteringMode(filter);

    }
    queryString+= where;
    if(!queryString.contains("as fid"))
        queryString.replace("= fid", "= ff.id");
    ProcessBindings(filter, query);


    if(filter.sortMode == StoryFilter::reccount)
        queryString = wrappignString.arg("select id as fid" + queryString);
    else
        queryString = normalString.arg(queryString);
    queryString+= " COLLATE NOCASE ";

    query->str = queryString;
    qDebug().noquote() << query->str;
    return query;
}

QString CountQueryBuilder::ProcessWhereSortMode(StoryFilter filter)
{
    QString queryString;
    if(filter.sortMode == StoryFilter::favrate)
        queryString += " and ( favourites/(julianday(CURRENT_TIMESTAMP) - julianday(Published)) > " + QString::number(filter.recentAndPopularFavRatio) + " OR  favourites > 1000) ";

    if(filter.sortMode == StoryFilter::favrate)
        queryString+= " and published <> updated "
                      " and published > date('now', '-" + QString::number(filter.recentCutoff.date().daysTo(QDate::currentDate())) + " days') "
                      " and published < date('now', '-" + QString::number(45) + " days') "
                      " and updated > date('now', '-60 days') ";

    // this doesnt require sumrecs as it's inverted
    return queryString;
}

}

