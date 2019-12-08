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
#pragma once

#include "storyfilter.h"
#include "libs/UniversalModels/include/TableDataInterface.h"
#include "libs/UniversalModels/include/TableDataListHolder.h"
#include "libs/UniversalModels/include/AdaptingTableModel.h"
#include "include/tasks/fandom_task_processor.h"
#include "include/tasks/author_task_processor.h"
//#include "qml_ficmodel.h"
#include "include/core/section.h"
#include "include/pagetask.h"
#include "include/rng.h"
#include "include/tasks/fandom_task_processor.h"
#include "include/tasks/author_cache_reprocessor.h"
#include "include/pagegetter.h"
#include "querybuilder.h"
#include "grpc/grpc_source.h"

#include <QQueue>

namespace interfaces{
class Fandoms;
class Fanfics;
class Authors;
class Tags;
class Genres;
class PageTask;
class RecommendationLists;
}

class FicSource;

class ClientInterface
{
public:
};


class QLineEdit;
class QLabel;

struct FilterFrame{

    int selectedIndex = -1;
    int sizeOfCurrentQuery = 0; // "would be" size of the used search query if LIMIT  was not applied
    int pageOfCurrentQuery = 0; // current page that the used search query is at
    int currentLastFanficId = -1;

    bool havePagesBefore = false;
    bool havePagesAfter = false;
    bool authorFilterActive = false;

    QVector<core::Fic> fanfics; // filtered fanfic data

    QSharedPointer<core::Query> currentQuery; // the last query created by query builder. reused when querying subsequent pages
    core::StoryFilter filter; // an intermediary to keep UI filter data to be passed into query builder
};



template <typename T>
struct BastardizedCircularBuffer{
    BastardizedCircularBuffer(int maxSize){
        this->maxSize = maxSize;
    }
    void Push(T value){
        if(data.size() == maxSize)
            data.pop_front();

//        while(currentIndex != 0)
//        {
//            data.pop_();
//            currentIndex--;
//        }

        data.push_back(value);
        currentIndex = 0;
    }

    T& AccessCurrent(){
        if(data.size() <= (currentIndex) || (currentIndex) < 0)
            return nullObject;

        return data[(data.size()-1) - currentIndex];
    }

    T GetNext(){
        if(data.size() <= (currentIndex - 1) || (currentIndex - 1) < 0)
            return T{};

        currentIndex--;
        return data[(data.size()-1) - currentIndex];
    }

    T GetPrevious(){
        if(data.size() <= (currentIndex + 1) || (currentIndex + 1) < 0)
            return T{};

        currentIndex++;
        return data[(data.size()-1) - currentIndex];
    }

    int Size() const{ return data.size();}
    int CurrentIndex() const {return currentIndex;}

    QList<T> data;
    T nullObject;
    int currentIndex = 0; // index from head
    int maxSize;
};

class CoreEnvironment : public QObject
{
Q_OBJECT
public:
    CoreEnvironment(QObject* obj = nullptr);

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
        QSharedPointer<database::IDBWrapper> userDb;
        QSharedPointer<database::IDBWrapper> backupDb;
        QSharedPointer<database::IDBWrapper> pageCache;
        QSharedPointer<database::IDBWrapper> tasks;
    };


    // reads settings into a files
    void ReadSettings();
    // writes settings into a files
    void WriteSettings();

    bool Init();
    static void InitMetatypes();
    // used to set up connections between database and interfaces
    // and between differnt interfaces themselves
    void InitInterfaces();
    void InstantiateClientDatabases(QString);
    void LoadData();
    void LoadHistoryFrame(FilterFrame);
    int GetResultCount();

    void LoadMoreAuthors(QString listname, ECacheMode cacheMode);
    void LoadAllLinkedAuthors(ECacheMode cacheMode);
    void LoadAllLinkedAuthorsMultiFromCache();


    void UseAuthorTask(PageTaskPtr task);
    void UseFandomTask(PageTaskPtr task);
    PageTaskPtr ProcessFandomsAsTask(QList<core::FandomPtr> fandoms,
                                     QString taskComment,
                                     bool allowCacheRefresh,
                                     ECacheMode cacheMode,
                                     QString cutoffText,
                                     ForcedFandomUpdateDate forcedDate = ForcedFandomUpdateDate());


    void UpdateAllAuthorsWith(std::function<void(QSharedPointer<core::Author>, WebPage)> updater);
    // used to fix author names when one of the parsers gets a bunch wrong
    void ReprocessAuthorNamesFromTheirPages();

    // used to create recommendation list from lists/source.txt
    void ProcessListIntoRecommendations(QString list);

    QVector<int> GetSourceFicsFromFile(QString filename);
    int  BuildRecommendationsServerFetch(QSharedPointer<core::RecommendationList> params, QVector<int> sourceFics);

    core::FicSectionStats GetStatsForFicList(QVector<int>);
    int  BuildRecommendationsLocalVersion(QSharedPointer<core::RecommendationList> params, bool clearAuthors = true);
    int  BuildRecommendations(QSharedPointer<core::RecommendationList> params,
                              QVector<int> sourceFics);

    int  BuildDiagnosticsForRecList(QSharedPointer<core::RecommendationList> params,
                              QVector<int> sourceFics);

    bool ResumeUnfinishedTasks();

    void CreateSimilarListForGivenFic(int id,  QSqlDatabase db);
    QVector<int> GetListSourceFFNIds(int listId);
    QVector<int> GetFFNIds(QSet<int> fics);

    void Log(QString);

    core::AuthorPtr LoadAuthor(QString url, QSqlDatabase db);
    QSet<QString> LoadAuthorFicIdsForRecCreation(QString url,
                                                 QLabel* infoTarget = nullptr,
                                                 bool silent = false);
    bool TestAuthorID(QString id);
    bool TestAuthorID(QLineEdit*, QLabel*);

    QList<QSharedPointer<core::Fic>>  LoadAuthorFics(QString url);

    PageTaskPtr LoadTrackedFandoms(ForcedFandomUpdateDate forcedDate, ECacheMode cacheMode, QString wordCutoff);
    void FillDBIDsForTags();
    QSet<int> GetAuthorsContainingFicFromRecList(int fic, QString recList);
    QSet<int> GetFicsForTags(QStringList);
    QSet<int> GetFicsForNegativeTags();
    QSet<int> GetIgnoredDeadFics();
    void LoadNewScoreValuesForFanfics(core::ReclistFilter filter, QVector<core::Fic>& fanfics);
    void BackupUserDatabase();
    int CreateDefaultRecommendationsForCurrentUser();


    void RefreshSnoozes();

    Interfaces interfaces;
    core::StoryFilter filter; // an intermediary to keep UI filter data to be passed into query builder

    int sizeOfCurrentQuery = 0; // "would be" size of the used search query if LIMIT  was not applied
    int pageOfCurrentQuery = 0; // current page that the used search query is at
    int currentLastFanficId = -1;

    QVector<core::Fic> fanfics; // filtered fanfic data

    QSharedPointer<core::Query> currentQuery; // the last query created by query builder. reused when querying subsequent pages

    QSharedPointer<FicSource> ficSource;
    QSharedPointer<core::IRNGGenerator> rngGenerator;
    QString userToken;
    QSet<int> likedAuthors;
    QHash<int, int> ficScores;
    ServerStatus status;
    BastardizedCircularBuffer<FilterFrame> searchHistory;


    // in case of non-gui applications these will just fire without an effect and its correct
signals:
    void requestProgressbar(int);
    void resetEditorText();
    void updateCounter(int);
    void updateInfo(QString);
};
namespace env {
    WebPage RequestPage(QString pageUrl, ECacheMode forcedCacheMode = ECacheMode::use_cache, bool autoSaveToDB = false);
}

