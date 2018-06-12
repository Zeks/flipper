#pragma once
#include "tagwidget.h"
#include "storyfilter.h"
#include "libs/UniversalModels/include/TableDataInterface.h"
#include "libs/UniversalModels/include/TableDataListHolder.h"
#include "libs/UniversalModels/include/AdaptingTableModel.h"
#include "qml_ficmodel.h"
#include "include/core/section.h"
#include "include/pagetask.h"
#include "include/tasks/fandom_task_processor.h"
#include "include/pagegetter.h"
#include "querybuilder.h"

namespace interfaces{
class Fandoms;
class Fanfics;
class Authors;
class Tags;
class Genres;
class PageTask;
class RecommendationLists;
}

struct SlashFilterState
{
    bool applyLocalEnabled;
    bool invertedEnabled;
    bool slashOnlyEnabled;
    bool invertedLocalEnabled;
    bool slashOnlyLocalEnabled;

};

class CoreEnvironment
{
public:
    struct Interfaces{
        // the interface classes used to avoid direct database access in the application
        QSharedPointer<interfaces::Fandoms> fandoms;
        QSharedPointer<interfaces::Fanfics> fanfics;
        QSharedPointer<interfaces::Authors> authors;
        QSharedPointer<interfaces::Tags> tags;
        QSharedPointer<interfaces::Genres> genres;
        QSharedPointer<interfaces::PageTask> pageTask;
        QSharedPointer<interfaces::RecommendationLists> recs;
        QSharedPointer<database::IDBWrapper> db;
        QSharedPointer<database::IDBWrapper> pageCache;
        QSharedPointer<database::IDBWrapper> tasks;
    };
    void Init();
    static void InitMetatypes();
    // used to set up connections between database and interfaces
    // and between differnt interfaces themselves
    void InitInterfaces();

    // used to build the actual query to be used in the database from filters
    QSqlQuery BuildQuery(bool countOnly = false);
    inline core::Fic LoadFanfic(QSqlQuery& q);
    void LoadData(SlashFilterState);

    WebPage RequestPage(QString pageUrl, ECacheMode forcedCacheMode = ECacheMode::use_cache, bool autoSaveToDB = false);
    int GetResultCount();

    core::DefaultQueryBuilder queryBuilder; // builds search queries
    core::CountQueryBuilder countQueryBuilder; // builds specialized query to get the last page for the interface;
    core::StoryFilter filter; // an intermediary to keep UI filter data to be passed into query builder
    Interfaces interfaces;

    int sizeOfCurrentQuery = 0; // "would be" size of the used search query if LIMIT  was not applied
    int pageOfCurrentQuery = 0; // current page that the used search query is at
    int currentLastFanficId = -1;


    QList<core::Fic> fanfics; // filtered fanfic data

    QThread pageThread; // thread for pagegetter worker to live in
    PageThreadWorker* worker = nullptr;
    PageQueue pageQueue; // collects data sent from PageThreadWorker

    QSharedPointer<core::Query> currentQuery; // the last query created by query builder. reused when querying subsequent pages

    int lastI = 0; // used in slash filtering
};
