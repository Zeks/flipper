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
#include "Interfaces/data_source.h"
#include "pure_sql.h"
#include "Interfaces/db_interface.h"
#include <QDebug>
#include <QSqlError>

FicSourceDirect::FicSourceDirect(QSharedPointer<database::IDBWrapper> dbInterface, QSharedPointer<core::RNGData> rngData){
    this->db = dbInterface;
    std::unique_ptr<core::DefaultRNGgenerator> rng (new core::DefaultRNGgenerator());
    rng->portableDBInterface = dbInterface;
    rng->rngData = rngData;
    queryBuilder.portableDBInterface = dbInterface;
    countQueryBuilder.portableDBInterface = dbInterface;
    QLOG_TRACE() << "RNG INIT";
    queryBuilder.SetIdRNGgenerator(rng.release());
    countQueryBuilder.rng = queryBuilder.rng;
}


void FicSource::AddFicFilter(QSharedPointer<FicFilter> filter)
{
    filters.push_back(filter);
}

void FicSource::ClearFilters()
{
    filters.clear();
}

QSqlQuery FicSourceDirect::BuildQuery(const core::StoryFilter& filter, bool countOnly)
{
    if(countOnly)
        currentQuery = countQueryBuilder.Build(filter);
    else
        currentQuery = queryBuilder.Build(filter);
    QSqlQuery q(db->GetDatabase());
    q.prepare(currentQuery->str);
    auto it = currentQuery->bindings.cbegin();
    auto end = currentQuery->bindings.cend();
    while(it != end)
    {
        if(currentQuery->str.contains(it->key))
        {
            qDebug() << it->key << " " << it->value;
            q.bindValue(it->key, it->value);
        }
        ++it;
    }
    return q;
}

inline core::Fanfic FicSourceDirect::LoadFanfic(QSqlQuery& q)
{
    // todo wait for response of qt guys about whether to use stringliteral here or not
    core::Fanfic result;
    result.identity.id = q.value(QStringLiteral("ID")).toInt();
    const auto fandoms = q.value(QStringLiteral("FANDOMIDS")).toString().split(QStringLiteral("::::"));
    for(const auto& id: fandoms)
        result.fandomIds.push_back(id.toInt());
    result.author = core::Author::NewAuthor();
    result.author_id = q.value(QStringLiteral("AUTHOR_ID")).toInt();;
    result.author->name = q.value(QStringLiteral("AUTHOR")).toString();
    result.title = q.value(QStringLiteral("TITLE")).toString();
    result.summary = q.value(QStringLiteral("SUMMARY")).toString();
    result.genreString = q.value(QStringLiteral("GENRES")).toString();
    result.charactersFull = q.value(QStringLiteral("CHARACTERS")).toString().replace(QStringLiteral("not found"), QStringLiteral(""));
    result.rated = q.value(QStringLiteral("RATED")).toString();
    auto published = q.value(QStringLiteral("PUBLISHED")).toDateTime();
    auto updated   = q.value(QStringLiteral("UPDATED")).toDateTime();
    result.published = published;
    result.updated= updated;
    result.SetUrl(QStringLiteral("ffn"),q.value(QStringLiteral("URL")).toString());
    result.identity.web.ffn = q.value(QStringLiteral("URL")).toInt();

    result.userData.tags = q.value(QStringLiteral("TAGS")).toString();
    result.wordCount = q.value(QStringLiteral("WORDCOUNT")).toString();
    result.favourites = q.value(QStringLiteral("FAVOURITES")).toString();
    result.reviews = q.value(QStringLiteral("REVIEWS")).toString();
    result.chapters = QString::number(q.value(QStringLiteral("CHAPTERS")).toInt());
    result.complete= q.value(QStringLiteral("COMPLETE")).toInt();
    result.userData.atChapter = q.value(QStringLiteral("AT_CHAPTER")).toInt();
    result.recommendationsData.recommendationsMainList= q.value(QStringLiteral("SUMRECS")).toInt();
    QString tg1 = q.value(QStringLiteral("true_genre1")).toString();
    QString tg2 = q.value(QStringLiteral("true_genre2")).toString();
    QString tg3 = q.value(QStringLiteral("true_genre3")).toString();
    if(!tg1.isEmpty())
        result.statistics.realGenreData.push_back({{tg1}, q.value(QStringLiteral("true_genre1_percent")).toFloat()});
    if(!tg2.isEmpty())
        result.statistics.realGenreData.push_back({{tg2}, q.value(QStringLiteral("true_genre2_percent")).toFloat()});
    if(!tg3.isEmpty())
        result.statistics.realGenreData.push_back({{tg3}, q.value(QStringLiteral("true_genre3_percent")).toFloat()});

    result.slashData.keywords_no = q.value(QStringLiteral("keywords_no")).toInt();
    result.slashData.keywords_yes = q.value(QStringLiteral("keywords_yes")).toInt();
    result.slashData.keywords_result = q.value(QStringLiteral("keywords_result")).toInt();
    result.slashData.filter_pass_1 = q.value(QStringLiteral("filter_pass_1")).toInt();
    result.slashData.filter_pass_2 = q.value(QStringLiteral("filter_pass_2")).toInt();

    //QLOG_INFO() << "recs value: " << q.value("sumrecs").toInt();
    return result;
}

int FicSourceDirect::GetFicCount(const core::StoryFilter& filter)
{
    auto q = BuildQuery(filter, true);
    q.setForwardOnly(true);
    if(!database::puresql::ExecAndCheck(q))
        return -1;
    q.next();
    auto result =  q.value(QStringLiteral("records")).toInt();
    return result;
}

//QSet<int> FicSourceDirect::GetAuthorsForFics(QSet<int> ficIDsForActivetags)
//{

//}

void FicSourceDirect::InitQueryType(bool client, QString userToken)
{
    queryBuilder.InitTagFilterBuilder(client, userToken);
    countQueryBuilder.InitTagFilterBuilder(client, userToken);
}

void FicSourceDirect::FetchData(const core::StoryFilter& searchfilter, QVector<core::Fanfic> *data)
{
    if(!data)
        return;
    QLOG_TRACE() << "Starting to build query";

    auto q = BuildQuery(searchfilter);
    QLOG_TRACE() << "Build query: success";
    q.setForwardOnly(true);
    q.exec();
    QLOG_TRACE() << "Exec query: success";
    if(q.lastError().isValid())
    {
        qDebug() << " ";
        qDebug() << " ";
        qDebug() << "Error loading data:" << q.lastError();
        qDebug().noquote() << q.lastQuery();
    }
    int counter = 0;
    data->clear();
    lastFicId = -1;
    while(q.next())
    {
        counter++;
        auto fic = LoadFanfic(q);
        bool filterOk = true;
        for(auto filter: std::as_const(filters))
            filterOk = filterOk && filter->Passed(&fic, searchfilter.slashFilter);
        if(filterOk)
            data->push_back(fic);
        if(counter%10000 == 0)
            QLOG_INFO_PURE() << "tick " << counter/1000;
    }
    QLOG_INFO_PURE() << "EXECUTED QUERY:" << q.lastQuery();
    if(data->size() > 0)
        lastFicId = (*data)[data->size() - 1].identity.id;
    QLOG_TRACE_PURE() << "loaded fics:" << counter;
}

FicFilterSlash::FicFilterSlash()
{
    regexToken.Init();
}



bool FicFilterSlash::Passed(core::Fanfic * fic, const SlashFilterState& slashFilter)
{
    bool allow = true;
    SlashPresence slashToken;
    if(slashFilter.excludeSlash || slashFilter.includeSlash)
        slashToken = regexToken.ContainsSlash(fic->summary, fic->charactersFull, fic->fandom);

    if(slashFilter.applyLocalEnabled && slashFilter.excludeSlashLocal)
    {
        if(slashToken.IsSlash())
            allow = false;
    }
    if(slashFilter.applyLocalEnabled && slashFilter.includeSlashLocal)
    {
        if(!slashToken.IsSlash())
            allow = false;
    }
    return allow;
}
