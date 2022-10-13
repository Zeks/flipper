/*
Flipper is a recommendation and search engine for fanfiction.net
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
#include "include/flipper_server_logic.h"
#include "include/regex_utils.h"
#include "include/Interfaces/recommendation_lists.h"
#include "include/pure_sql.h"
#include "querybuilder.h"
#include "sql_abstractions/sql_query.h"
#include <QDebug>
#include <QSettings>
#include <QSqlError>
#include <QTextCodec>
#include <memory>

sql::Query FeederEnvironment::BuildQuery(bool countOnly)
{
    sql::Database db = sql::Database::database();
    if(countOnly)
        currentQuery = countQueryBuilder.Build(filter);
    else
        currentQuery = queryBuilder.Build(filter);
    sql::Query q(db);
    q.prepare(currentQuery->str);
    auto it = currentQuery->bindings.cbegin();
    auto end = currentQuery->bindings.cend();
    while(it != end)
    {
        if(currentQuery->str.find(it->key) != std::string::npos)
        {
            qDebug() << it->key << " " << it->value;
            q.bindValue(it->key, it->value);
        }
        ++it;
    }
    return q;
}

void FeederEnvironment::LoadData(SlashFilterState slashFilter)
{
    auto q = BuildQuery();
    q.setForwardOnly(true);
    q.exec();
    qDebug() << q.lastQuery();
    if(q.lastError().isValid())
    {
        qDebug() << " ";
        qDebug() << " ";
        qDebug() << "Error loading data:" << q.lastError().text();
        qDebug() << q.lastQuery();
    }
    int counter = 0;
    fanfics.clear();
    currentLastFanficId = -1;
    CommonRegex regexToken;
    regexToken.Init();
    while(q.next())
    {
        counter++;
        bool allow = true;
        auto fic = LoadFanfic(q);
        SlashPresence slashToken;
        if(slashFilter.excludeSlash || slashFilter.includeSlash)
            slashToken = regexToken.ContainsSlash(fic.summary, fic.charactersFull, fic.fandom);

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
        if(allow)
            fanfics.push_back(fic);
        if(counter%10000 == 0)
            qDebug() << "tick " << counter/1000;
    }
    if(fanfics.size() > 0)
        currentLastFanficId = fanfics.last().GetIdInDatabase();
    qDebug() << "loaded fics:" << counter;
}

FeederEnvironment::FeederEnvironment(QObject *obj): QObject(obj)
{
    ReadSettings();
}

void FeederEnvironment::ReadSettings()
{

}

void FeederEnvironment::WriteSettings()
{

}

void FeederEnvironment::Init()
{
    //QLOG_INFO() << "RNG INIT";
    std::unique_ptr<core::DefaultRNGgenerator> rng (new core::DefaultRNGgenerator());
    queryBuilder.SetIdRNGgenerator(rng.release());

    QSettings settings("settings/ui.ini", QSettings::IniFormat);
    auto storedRecList = settings.value("Settings/currentList").toString();
    interfaces.recs->SetCurrentRecommendationList(interfaces.recs->GetListIdForName(storedRecList));
    interfaces.recs->LoadAvailableRecommendationLists();
}


inline core::Fanfic FeederEnvironment::LoadFanfic(sql::Query& q)
{
    core::Fanfic result;
    result.identity.id = q.value("ID").toInt();
    result.fandom = QString::fromStdString(q.value("FANDOM").toString());
    result.author = core::Author::NewAuthor();
    result.author->name = QString::fromStdString(q.value("AUTHOR").toString());
    result.title = QString::fromStdString(q.value("TITLE").toString());
    result.summary = QString::fromStdString(q.value("SUMMARY").toString());
    result.genreString = QString::fromStdString(q.value("GENRES").toString());
    result.charactersFull = QString::fromStdString(q.value("CHARACTERS").toString()).replace("not found", "");
    result.rated = QString::fromStdString(q.value("RATED").toString());
    auto published = q.value("PUBLISHED").toDateTime();
    auto updated   = q.value("UPDATED").toDateTime();
    result.published = published;
    result.updated= updated;
    result.SetUrl("ffn",QString::fromStdString(q.value("URL").toString()));
    result.userData.tags = QString::fromStdString(q.value("TAGS").toString());
    result.wordCount = QString::fromStdString(q.value("WORDCOUNT").toString());
    result.favourites =QString::fromStdString(q.value("FAVOURITES").toString());
    result.reviews = QString::fromStdString(q.value("REVIEWS").toString());
    result.chapters = QString::number(q.value("CHAPTERS").toInt());
    result.complete= q.value("COMPLETE").toInt();
    result.userData.atChapter = q.value("AT_CHAPTER").toInt();
    result.recommendationsData.recommendationsMainList= q.value("SUMRECS").toInt();
    //QLOG_INFO() << "recs value: " << result.recommendations;
    return result;
}
void FeederEnvironment::InitInterfaces()
{
    interfaces.recs   = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    interfaces.recs->db    = interfaces.db;
    queryBuilder.db = interfaces.db;
    countQueryBuilder.db = interfaces.db;
}

int FeederEnvironment::GetResultCount()
{
    auto q = BuildQuery(true);
    q.setForwardOnly(true);
    if(!sql::ExecAndCheck(q))
        return -1;
    q.next();
    auto result =  q.value("records").toInt();
    return result;
}

void FeederEnvironment::Log(QString value)
{
    qDebug() << value;
}
