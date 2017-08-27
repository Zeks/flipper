#include "querybuilder.h"

namespace  core{
QString WrapTag(QString tag)
{
    tag= "(.{0,}" + tag + ".{0,})";
    return tag;
}
QString CreateLimitQueryPart(StoryFilter filter)
{
    QString result;
    int maxFicCountValue = ui->chkFicLimitActivated->isChecked() ? ui->sbMaxFicCount->value()  : 0;
    if(maxFicCountValue > 0 && maxFicCountValue < 51)
        result+= QString(" LIMIT %1 ").arg(QString::number(maxFicCountValue));
    return result;
}
QString DefaultQueryBuilder::Build(StoryFilter filter)
{

    ProcessBindings(filter);

    queryString+= CreateFields(filter);
    queryString+=" f.* from fanfics f where 1 = 1 " ;
    queryString+= CreateWhere(filter);
    queryString+="COLLATE NOCASE ORDER BY " + diffField;
    queryString+=CreateLimitQueryPart(filter);

    std::function<QSqlQuery(QString)> bindQuery =
            [&](QString qs){
        QSqlQuery q(db);
        q.prepare(qs);

        if(ui->cbMinWordCount->currentText().toInt() > 0)
            q.bindValue(":minwordcount", ui->cbMinWordCount->currentText().toInt());
        if(ui->cbMaxWordCount->currentText().toInt() > 0)
            q.bindValue(":maxwordcount", ui->cbMaxWordCount->currentText().toInt());
        if(ui->sbMinimumFavourites->value() > 0)
            q.bindValue(":favourites", ui->sbMinimumFavourites->value());

        q.bindValue(":tags", tags);
        q.bindValue(":rectag", ui->cbRecGroup->currentText());
        return q;
    };

    int maxFicCountValue = ui->chkFicLimitActivated->isChecked() ? ui->sbMaxFicCount->value()  : 0;
    if(ui->chkRandomizeSelection->isChecked() && maxFicCountValue > 0 && maxFicCountValue < 51)
    {
        PopulateIdList(bindQuery, queryString);
    }
    auto q = bindQuery(AddIdList(queryString, maxFicCountValue));
    return q;
}

QString DefaultQueryBuilder::CreateFields(StoryFilter filter)
{
    queryString = "select ID, ";
    queryString+=ProcessSumFaves(filter);
    queryString+=ProcessSumRecs(filter);
}

QString DefaultQueryBuilder::CreateWhere(StoryFilter)
{
    diffField = ProcessDiffField(filter);
    activeTags = ProcessActiveTags(filter)    ;

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

QString DefaultQueryBuilder::ProcessSumRecs(StoryFilter filter)
{
    QString currentRecTagValue = " (SELECT sumrecs FROM RecommendationFicStats rfs where rfs.fic_id = f.id and rfs.tag = '%1') as sumrecs, ";
    currentRecTagValue=currentRecTagValue.arg(filter.tagForRecommendations);
    return currentRecTagValue;
}

QString DefaultQueryBuilder::ProcessWordcount(StoryFilter filter)
{
    if(filter.minWords > 0)
        queryString += " and wordcount > :minwordcount ";
    if(filter.maxWords > 0)
        queryString += " and wordcount < :maxwordcount ";
}

QString DefaultQueryBuilder::ProcessGenreIncluson(StoryFilter filter)
{
    if(filter.genreInclusion.size() > 0)
        for(auto genre : filter.genreInclusion)
            queryString += QString(" AND genres like '%%1%' ").arg(genre);

    if(filter.genreExclusion.size() > 0)
        for(auto genre : filter.genreExclusion)
            queryString += QString(" AND genres not like '%%1%' ").arg(genre);
}

QString DefaultQueryBuilder::ProcessWordInclusion(StoryFilter filter)
{
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
}

QString DefaultQueryBuilder::ProcessActiveRecommendationsPart(StoryFilter filter)
{
    if(filter.mode == core::StoryFilter::filtering_in_recommendations)
    {
        QString qsl = " and id in (select fic_id from recommendations %1)";

        if(filter.useThisRecommenderOnly != -1)
            qsl=qsl.arg(QString(" where recommender_id = %1 ").arg(QString::number(currentRecommenderId)));
        else
            qsl=qsl.arg(QString(""));
        queryString+=qsl;
    }
}

QString DefaultQueryBuilder::ProcessWhereSortMode(StoryFilter filter)
{
    if(filter.sortMode == StoryFilter::favrate)
        queryString += " and ( favourites/(julianday(Updated) - julianday(Published)) > " + QString::number(ui->sbFavrateValue->value()) + " OR  favourites > 1000) ";

    if(filter.sortMode == StoryFilter::reccount)
        queryString += QString(" AND sumrecs > 0 ");
    if(filter.sortMode == StoryFilter::favrate)
        queryString+= " and published <> updated and published > date('now', '-" + QString::number(ui->dteFavRateCut->date().daysTo(QDate::currentDate())) + " days') and updated > date('now', '-60 days') ";
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
}

QString DefaultQueryBuilder::ProcessNormalOrCrossover(StoryFilter filter)
{
    if(filter.includeCrossovers)
        queryString+=QString(" and  f.fandom like '%%1%' and f.fandom like '%CROSSOVER%'").arg(filter.fandom);
    else
        queryString+=QString(" and  f.fandom like '%%1%'").arg(filter.fandom);

}

QString DefaultQueryBuilder::ProcessFilteringMode(StoryFilter filter)
{
    if(filter.mode == core::StoryFilter::filtering_in_fics)
    {
        if(!tags.isEmpty())
            queryString +=" and cfRegexp(:tags, tags) ";
        else
            queryString +=" and tags = ' none ' ";

    }
    else
    {
        if(filter.ignoreAlreadyTagged)
            queryString += QString(" and tags = ' none ' ");
        else
            queryString += QString("");
    }
}

QString DefaultQueryBuilder::ProcessActiveTags(StoryFilter filter)
{
    QString tags;
    for(auto tag : filter.activeTags)
        tags.push_back(WrapTag(tag) + "|");

    tags.chop(1);

}

QString DefaultQueryBuilder::ProcessIdRandomization(StoryFilter)
{

}

QString DefaultQueryBuilder::ProcessBindings(StoryFilter filter)
{
    if(filter.minWords > 0)
        query.bindings[":minwordcount"] = filter.minWords;
    if(filter.maxWords > 0)
        query.bindings[":maxwordcount"] = filter.maxWords;
    if(filter.minFavourites> 0)
        query.bindings[":favourites"] = filter.minFavourites;
    query.bindings[":tags"] = filter.activeTags;
    query.bindings[":rectag"] = filter.tagForRecommendations;
}

QString DefaultQueryBuilder::BuildSortMode(StoryFilter)
{

}

QString DefaultQueryBuilder::BuildConditions(StoryFilter)
{

}

}

