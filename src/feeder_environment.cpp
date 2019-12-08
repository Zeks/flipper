/*
Flipper is a replacement search engine for fanfiction.net search results
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
#include "include/feeder_environment.h"
#include "include/regex_utils.h"
#include "include/Interfaces/recommendation_lists.h"
#include "include/pure_sql.h"
#include "include/url_utils.h"
#include "querybuilder.h"
#include <QSqlQuery>
#include <QDebug>
#include <QSettings>
#include <QSqlError>
#include <QTextCodec>
#include <memory>

QSqlQuery FeederEnvironment::BuildQuery(bool countOnly)
{
    QSqlDatabase db = QSqlDatabase::database();
    if(countOnly)
        currentQuery = countQueryBuilder.Build(filter);
    else
        currentQuery = queryBuilder.Build(filter);
    QSqlQuery q(db);
    q.prepare(currentQuery->str);
    auto it = currentQuery->bindings.begin();
    auto end = currentQuery->bindings.end();
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

void FeederEnvironment::LoadData(SlashFilterState slashFilter)
{
    auto q = BuildQuery();
    q.setForwardOnly(true);
    q.exec();
    qDebug().noquote() << q.lastQuery();
    if(q.lastError().isValid())
    {
        qDebug() << " ";
        qDebug() << " ";
        qDebug() << "Error loading data:" << q.lastError();
        qDebug().noquote() << q.lastQuery();
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
        currentLastFanficId = fanfics.last().id;
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
    rng->portableDBInterface = interfaces.db;
    queryBuilder.SetIdRNGgenerator(rng.release());

    QSettings settings("settings/ui.ini", QSettings::IniFormat);
    auto storedRecList = settings.value("Settings/currentList").toString();
    interfaces.recs->SetCurrentRecommendationList(interfaces.recs->GetListIdForName(storedRecList));
    interfaces.recs->LoadAvailableRecommendationLists();
}


inline core::Fic FeederEnvironment::LoadFanfic(QSqlQuery& q)
{
    core::Fic result;
    result.id = q.value("ID").toInt();
    result.fandom = q.value("FANDOM").toString();
    result.author = core::Author::NewAuthor();
    result.author->name = q.value("AUTHOR").toString();
    result.title = q.value("TITLE").toString();
    result.summary = q.value("SUMMARY").toString();
    result.genreString = q.value("GENRES").toString();
    result.charactersFull = q.value("CHARACTERS").toString().replace("not found", "");
    result.rated = q.value("RATED").toString();
    auto published = q.value("PUBLISHED").toDateTime();
    auto updated   = q.value("UPDATED").toDateTime();
    result.published = published;
    result.updated= updated;
    result.SetUrl("ffn",q.value("URL").toString());
    result.tags = q.value("TAGS").toString();
    result.wordCount = q.value("WORDCOUNT").toString();
    result.favourites = q.value("FAVOURITES").toString();
    result.reviews = q.value("REVIEWS").toString();
    result.chapters = QString::number(q.value("CHAPTERS").toInt());
    result.complete= q.value("COMPLETE").toInt();
    result.atChapter = q.value("AT_CHAPTER").toInt();
    result.recommendationsMainList= q.value("SUMRECS").toInt();
    //QLOG_INFO() << "recs value: " << result.recommendations;
    return result;
}
void FeederEnvironment::InitInterfaces()
{
    interfaces.recs   = QSharedPointer<interfaces::RecommendationLists> (new interfaces::RecommendationLists());
    interfaces.recs->portableDBInterface = interfaces.db;
    interfaces.recs->db    = interfaces.db->GetDatabase();
    queryBuilder.portableDBInterface = interfaces.db;
    countQueryBuilder.portableDBInterface = interfaces.db;
}

int FeederEnvironment::GetResultCount()
{
    auto q = BuildQuery(true);
    q.setForwardOnly(true);
    if(!database::puresql::ExecAndCheck(q))
        return -1;
    q.next();
    auto result =  q.value("records").toInt();
    return result;
}

void FeederEnvironment::Log(QString value)
{
    qDebug() << value;
}
