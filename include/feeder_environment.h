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
#pragma once
#include "storyfilter.h"
#include "include/core/section.h"
#include "querybuilder.h"
#include "storyfilter.h"

namespace interfaces{
class Fandoms;
class Fanfics;
class Authors;
class Tags;
class Genres;
class PageTask;
class RecommendationLists;
}



class FeederEnvironment : public QObject
{
Q_OBJECT
public:
    FeederEnvironment(QObject* obj = nullptr);

    struct Interfaces{
        // the interface classes used to avoid direct database access in the application
        QSharedPointer<interfaces::RecommendationLists> recs;
        QSharedPointer<database::IDBWrapper> db;
    };


    // reads settings into a files
    void ReadSettings();
    // writes settings into a files
    void WriteSettings();

    void Init();
    // used to set up connections between database and interfaces
    // and between differnt interfaces themselves
    void InitInterfaces();

    // used to build the actual query to be used in the database from filters
    QSqlQuery BuildQuery(bool countOnly = false);
    inline core::Fanfic LoadFanfic(QSqlQuery& q);
    void LoadData(SlashFilterState);
    int GetResultCount();

    void Log(QString);

    core::DefaultQueryBuilder queryBuilder; // builds search queries
    core::CountQueryBuilder countQueryBuilder; // builds specialized query to get the last page for the interface;
    core::StoryFilter filter; // an intermediary to keep UI filter data to be passed into query builder
    Interfaces interfaces;

    int sizeOfCurrentQuery = 0; // "would be" size of the used search query if LIMIT  was not applied
    int pageOfCurrentQuery = 0; // current page that the used search query is at
    int currentLastFanficId = -1;

    QList<core::Fanfic> fanfics; // filtered fanfic data

    QSharedPointer<core::Query> currentQuery; // the last query created by query builder. reused when querying subsequent pages

signals:
    void requestProgressbar(int);
    void resetEditorText();
    void updateCounter(int);
    void updateInfo(QString);
};

