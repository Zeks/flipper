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

sql::Query FicSourceDirect::BuildQuery(const core::StoryFilter& filter, bool countOnly)
{
    if(countOnly)
        currentQuery = countQueryBuilder.Build(filter);
    else
        currentQuery = queryBuilder.Build(filter);
    sql::Query q(db->GetDatabase());
    q.prepare(currentQuery->str);
    auto it = currentQuery->bindings.cbegin();
    auto end = currentQuery->bindings.cend();
    while(it != end)
    {
        if(currentQuery->str.find(it->key) != std::string::npos)
        {
            qDebug() << QString::fromStdString(it->key) << " " << it->value;
            q.bindValue(it->key, it->value);
        }
        ++it;
    }
    return q;
}

inline core::Fanfic FicSourceDirect::LoadFanfic(sql::Query& q)
{
    // todo wait for response of qt guys about whether to use stringliteral here or not
    core::Fanfic result;
    result.identity.id = q.value("ID").toInt();
    const auto fandoms = QString::fromStdString(q.value("FANDOMIDS").toString()).split(QStringLiteral("::::"));
    for(const auto& id: fandoms)
        result.fandomIds.push_back(id.toInt());
    result.author = core::Author::NewAuthor();
    result.author_id = q.value("AUTHOR_ID").toInt();;
    result.author->name = QString::fromStdString(q.value("AUTHOR").toString());
    result.title = QString::fromStdString(q.value("TITLE").toString());
    result.summary = QString::fromStdString(q.value("SUMMARY").toString());
    result.genreString = QString::fromStdString(q.value("GENRES").toString());
    result.charactersFull = QString::fromStdString(q.value("CHARACTERS").toString()).replace(QStringLiteral("not found"), QStringLiteral(""));
    result.rated = QString::fromStdString(q.value("RATED").toString());
    auto published = q.value("PUBLISHED").toDateTime();
    auto updated   = q.value("UPDATED").toDateTime();
    result.published = published;
    result.updated= updated;
    result.SetUrl("ffn",QString::fromStdString(q.value("URL").toString()));
    result.identity.web.ffn = q.value("URL").toInt();

    result.userData.tags = QString::fromStdString(q.value("TAGS").toString());
    result.wordCount = QString::fromStdString(q.value("WORDCOUNT").toString());
    result.favourites = QString::fromStdString(q.value("FAVOURITES").toString());
    result.reviews = QString::fromStdString(q.value("REVIEWS").toString());
    result.chapters = QString::number(q.value("CHAPTERS").toInt());
    result.complete= q.value("COMPLETE").toInt();
    result.userData.atChapter = q.value("AT_CHAPTER").toInt();
    result.recommendationsData.recommendationsMainList= q.value("SUMRECS").toInt();
    auto tg1 = QString::fromStdString(q.value("true_genre1").toString());
    auto tg2 = QString::fromStdString(q.value("true_genre2").toString());
    auto tg3 = QString::fromStdString(q.value("true_genre3").toString());
    if(tg1.length() != 0)
        result.statistics.realGenreData.push_back({{tg1}, q.value("true_genre1_percent").toFloat()});
    if(tg2.length() != 0)
        result.statistics.realGenreData.push_back({{tg2}, q.value("true_genre2_percent").toFloat()});
    if(tg3.length() != 0)
        result.statistics.realGenreData.push_back({{tg3}, q.value("true_genre3_percent").toFloat()});

    result.slashData.keywords_no = q.value("keywords_no").toInt();
    result.slashData.keywords_yes = q.value("keywords_yes").toInt();
    result.slashData.keywords_result = q.value("keywords_result").toInt();
    result.slashData.filter_pass_1 = q.value("filter_pass_1").toInt();
    result.slashData.filter_pass_2 = q.value("filter_pass_2").toInt();

    //QLOG_INFO() << "recs value: " << q.value("sumrecs").toInt();
    return result;
}

int FicSourceDirect::GetFicCount(const core::StoryFilter& filter)
{
    auto q = BuildQuery(filter, true);
    q.setForwardOnly(true);
    if(!sql::ExecAndCheck(q))
        return -1;
    q.next();
    auto result =  q.value("records").toInt();
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
        qDebug() << "Error loading data:" << q.lastError().text();
        qDebug() << q.lastQuery();
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
    QLOG_INFO_PURE() << "EXECUTED QUERY:" << QString::fromStdString(q.lastQuery());
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
