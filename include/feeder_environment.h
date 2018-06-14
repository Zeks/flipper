#pragma once
#include "storyfilter.h"
#include "include/core/section.h"
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
    inline core::Fic LoadFanfic(QSqlQuery& q);
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

    QList<core::Fic> fanfics; // filtered fanfic data

    QSharedPointer<core::Query> currentQuery; // the last query created by query builder. reused when querying subsequent pages

signals:
    void requestProgressbar(int);
    void resetEditorText();
    void updateCounter(int);
    void updateInfo(QString);
};

