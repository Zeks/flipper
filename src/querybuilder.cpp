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
    idQuery = NewQuery();
}

QSharedPointer<Query> DefaultQueryBuilder::Build(StoryFilter filter)
{
    query = NewQuery();

    queryString.clear();
    queryString = "ID, ";
    queryString+= CreateCustomFields(filter) + " f.* ";
    queryString+=" from fanfics f where 1 = 1 and alive = 1 " ;
    QString where = CreateWhere(filter);
    queryString+= where;
    ProcessBindings(filter, query);
    queryString+= ProcessRandomization(filter, queryString);
    queryString+= BuildSortMode(filter);
    queryString+= CreateLimitQueryPart(filter);

    qDebug() << queryString;
    query->str = "select " + queryString;
    return query;
}

QString DefaultQueryBuilder::CreateCustomFields(StoryFilter filter)
{
    QString queryString;
    queryString+=ProcessSumFaves(filter);
    queryString+=ProcessSumRecs(filter);
    queryString+=ProcessTags(filter);
    queryString+=ProcessUrl(filter);

    return queryString;
}

QString DefaultQueryBuilder::CreateWhere(StoryFilter filter)
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

QString DefaultQueryBuilder::ProcessSumFaves(StoryFilter filter)
{
    QString sumOfAuthorFavourites = " (SELECT sumfaves FROM recommenders where name = f.author) as sumfaves, ";
    return sumOfAuthorFavourites;
}

// not exactly what its supposed to do but okay query to save
//QString currentRecTagValue = " ((SELECT match_count FROM RecommendationListData rfs where rfs.fic_id = f.id and rfs.list_id = 1)* "
//        " ( case when (select (select max(average_faves_top_3) from fandoms)/(select max(average_faves_top_3) from fandoms fs where fs.fandom in (f.fandom1, f.fandom2) )) > 3  then 1.5 "
//        " when (select (select max(average_faves_top_3) from fandoms)/(select max(average_faves_top_3) from fandoms fs where fs.fandom in (f.fandom1, f.fandom2) )) > 15 then 2 "
//        " else 1 end))  as sumrecs, ";

QString DefaultQueryBuilder::ProcessSumRecs(StoryFilter filter)
{
    QString currentRecTagValue = " (SELECT match_count FROM RecommendationListData rfs where rfs.fic_id = f.id and rfs.list_id = :list_id) as sumrecs, ";
    return currentRecTagValue;
}

QString DefaultQueryBuilder::ProcessTags(StoryFilter)
{
    QString currentTagValue = " (SELECT  group_concat(tag, ' ')  FROM fictags where fic_id = f.id order by tag asc) as tags, ";
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
    if(filter.mode == core::StoryFilter::filtering_in_recommendations)
    {
        QString qsl = " and id in (select fic_id from recommendations %1)";

        if(filter.useThisRecommenderOnly != -1)
            qsl=qsl.arg(QString(" where recommender_id = %1 ").arg(QString::number(filter.useThisRecommenderOnly)));
        else
            qsl=qsl.arg(QString(""));
        queryString+=qsl;
    }
    return queryString;
}

QString DefaultQueryBuilder::ProcessWhereSortMode(StoryFilter filter)
{
    QString queryString;
    if(filter.sortMode == StoryFilter::favrate)
        queryString += " and ( favourites/(julianday(Updated) - julianday(Published)) > " + QString::number(filter.recentAndPopularFavRatio) + " OR  favourites > 1000) ";

    if(filter.sortMode == StoryFilter::reccount)
        queryString += QString(" AND sumrecs > 0 ");
    if(filter.sortMode == StoryFilter::favrate)
        queryString+= " and published <> updated and published > date('now', '-" + QString::number(filter.recentCutoff.date().daysTo(QDate::currentDate())) + " days') and updated > date('now', '-60 days') ";
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
    else if(filter.sortMode == StoryFilter::reccount)
        diffField = " sumrecs desc";
    else if(filter.sortMode == StoryFilter::favrate)
        diffField = " favourites/(julianday(Updated) - julianday(Published)) desc";
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
    if(filter.includeCrossovers)
        queryString+=QString(" and  f.fandom like '%%1%' and f.fandom like '%CROSSOVER%'").arg(filter.fandom);
    else
        queryString+=QString(" and  f.fandom like '%%1%'").arg(filter.fandom);
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
            queryString += QString(" and not exists  (select fic_id from fictags where fic_id = f.id)");

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

void DefaultQueryBuilder::ProcessBindings(StoryFilter filter, QSharedPointer<Query> q)
{
    if(filter.minWords > 0)
        q->bindings[":minwordcount"] = filter.minWords;
    if(filter.maxWords > 0)
        q->bindings[":maxwordcount"] = filter.maxWords;
    if(filter.minFavourites> 0)
        q->bindings[":favourites"] = filter.minFavourites;
    if(filter.listForRecommendations > -1)
        q->bindings[":list_id"] = filter.listForRecommendations;
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
    int maxFicCountValue = filter.maxFics;
    if(maxFicCountValue > 0)
        result+= QString(" LIMIT %1 COLLATE NOCASE ").arg(QString::number(maxFicCountValue));
    return result;
}

QSharedPointer<Query> DefaultQueryBuilder::NewQuery()
{
    return QSharedPointer<Query>(new Query);
}


QString DefaultRNGgenerator::Get(QSharedPointer<Query> query, QSqlDatabase db)
{
    QString where = query->str;

    if(!randomIdLists.contains(where))
    {
        auto idList = portableDBInterface->GetIdListForQuery(query, db);
        if(idList.size() == 0)
            idList.push_back("-1");
        randomIdLists[where] = idList;
    }
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    auto& currentList = randomIdLists[where];
    std::uniform_int_distribution<> distr(0, currentList.size()); // define the range
    auto value = distr(eng);
    return currentList[value];
}

}

