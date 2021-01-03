/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

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
#include "tasks/recommendations_reload_precessor.h"
#include "include/pagegetter.h"
#include "include/pagetask.h"
#include "include/parsers/ffn/fandomparser.h"
#include "include/parsers/ffn/desktop_favparser.h"
#include "include/transaction.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/fandoms.h"
#include "include/Interfaces/authors.h"
#include "include/Interfaces/pagetask_interface.h"
#include "include/Interfaces/recommendation_lists.h"
//#include "include/environment.h"
#include "include/in_tag_accessor.h"
#include "include/url_utils.h"
#include "include/environment.h"
#include "include/timeutils.h"
#include "include/page_utils.h"
#include "include/EGenres.h"
#include <array>

#include <QThread>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

RecommendationsProcessor::RecommendationsProcessor(sql::Database db,
                                                   QSharedPointer<interfaces::Fanfics> fanficInterface,
                                                   QSharedPointer<interfaces::Fandoms> fandomsInterface,
                                                   QSharedPointer<interfaces::Authors> authorsInterface,
                                                   QSharedPointer<interfaces::RecommendationLists> recsInterface,
                                                   QObject *obj) : QObject(obj)

{
    this->fanficsInterface = fanficInterface;
    this->fandomsInterface = fandomsInterface;
    this->authorsInterface = authorsInterface;
    this->recsInterface = recsInterface;
    this->db = db;
}

RecommendationsProcessor::~RecommendationsProcessor()
{

}


void RecommendationsProcessor::ReloadRecommendationsList(ECacheMode cacheMode)
{
    database::Transaction transaction(db);

    auto& authors = stagedAuthors;
    std::sort(authors.begin(), authors.end(), [](const auto& a1, const auto& a2){
        return a1->stats.favouritesLastChecked > a2->stats.favouritesLastChecked;
    });
    emit requestProgressbar(authors.size());
    QSet<QString> fandoms;
    //QList<core::FicRecommendation> recommendations;
    auto fanficsInterface = this->fanficsInterface;
    auto authorsInterface = this->authorsInterface;
    auto fandomsInterface = this->fandomsInterface;
    auto job = [fanficsInterface,authorsInterface,fandomsInterface](QString url, QString content){
        //QList<QSharedPointer<core::Fanfic> > sections;
        FavouriteStoryParser parser;
        parser.ProcessPage(url, content);
        return parser;
    };
    int counter = 0;
    QLOG_INFO() << " Scheduled authors size: " << authors.size();
    for(auto author: authors)
    {
//        if(counter > 0)
//            break;
        if(counter%50 == 0)
        {
            QLOG_INFO() << "=========================================================================";
            QLOG_INFO() << "At this moment processed:  "<< counter << " authors of: " << authors.size();
            QLOG_INFO() << "=========================================================================";
        }
        //QList<QSharedPointer<core::Fanfic>> sections;
        QVector<QFuture<FavouriteStoryParser>> futures;
        //QSet<int> uniqueAuthors;
        authorsInterface->DeleteLinkedAuthorsForAuthor(author->id);
        auto startPageRequest = std::chrono::high_resolution_clock::now();
        auto page = env::RequestPage(author->url("ffn"), cacheMode);
        auto elapsed = std::chrono::high_resolution_clock::now() - startPageRequest;
        qDebug() <<  "Loading author: " << author->GetWebID("ffn");
        //qDebug() << "Fetched page in: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        auto startPageProcess = std::chrono::high_resolution_clock::now();
        FavouriteStoryParser parser;
        //parser.ProcessPage(page.url, page.content);

        auto splittings = page_utils::SplitJob(page.content);
        for(const auto& part: std::as_const(splittings.parts))
        {
            futures.push_back(QtConcurrent::run(job, page.url, part.data));
        }
        for(auto future: futures)
        {
            future.waitForFinished();
        }

        ////
        FavouriteStoryParser sumParser;
        sumParser.SetAuthor(author);

        QList<FavouriteStoryParser> finishedParsers;
        for(const auto& actualParser: futures)
            finishedParsers.push_back(actualParser.result());

        FavouriteStoryParser::MergeStats(author,fandomsInterface, finishedParsers);
        authorsInterface->UpdateAuthorRecord(author);

        for(const auto& actualParser: std::as_const(finishedParsers))
            sumParser.processedStuff+=actualParser.processedStuff;
        ////
        {
            WriteProcessedFavourites(sumParser, author, fanficsInterface, authorsInterface, fandomsInterface);
            if(fanficsInterface->skippedCounter > 0)
                qDebug() << "skipped: " << fanficsInterface->skippedCounter;
        }
        counter++;
        emit resetEditorText();
        emit updateInfo(parser.diagnostics.join(""));
        emit updateCounter(counter);
        QCoreApplication::processEvents();

        elapsed = std::chrono::high_resolution_clock::now() - startPageProcess;
        //qDebug() << "Processed page in: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    }
    fandomsInterface->RecalculateFandomStats(fandoms.values());
    transaction.finalize();
    fanficsInterface->ClearProcessedHash();
}

bool RecommendationsProcessor::AddAuthorToRecommendationList(QString listName, QString authorUrl)
{
    database::Transaction transaction(db);
    auto listId = recsInterface->GetListIdForName(listName);
    if(listId == -1)
    {
        QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
        params->name = listName;
        recsInterface->LoadListIntoDatabase(params);
        listId = recsInterface->GetListIdForName(listName);
    }
    auto author = authorsInterface->GetByWebID("ffn", url_utils::GetWebId(authorUrl, "ffn").toInt());
    if(!author)
        return false;

    QSharedPointer<core::AuthorRecommendationStats> stats(new core::AuthorRecommendationStats);
    stats->authorId = author->id;
    stats->listId = listId;
    stats->authorName = author->name;
    recsInterface->LoadAuthorRecommendationsIntoList(author->id, listId);
    recsInterface->LoadAuthorRecommendationStatsIntoDatabase(listId, stats);
    recsInterface->IncrementAllValuesInListMatchingAuthorFavourites(author->id,listId);
    transaction.finalize();
    return true;
}

bool RecommendationsProcessor::RemoveAuthorFromRecommendationList(QString listName, QString authorUrl)
{
    database::Transaction transaction(db);
    auto author = authorsInterface->GetByWebID("ffn", url_utils::GetWebId(authorUrl, "ffn").toInt());
    if(!author)
        return false;

    auto listId = recsInterface->GetListIdForName(listName);
    if(listId == -1)
        return false;

    recsInterface->RemoveAuthorRecommendationStatsFromDatabase(listId, author->id);
    recsInterface->DecrementAllValuesInListMatchingAuthorFavourites(author->id,listId);
    transaction.finalize();
    return true;
}

void RecommendationsProcessor::StageAuthorsForList(QString listName)
{
    recsInterface->SetCurrentRecommendationList(recsInterface->GetListIdForName(listName));
    auto recList = recsInterface->GetCurrentRecommendationList();
    stagedAuthors = recsInterface->GetAuthorsForRecommendationList(recList);
}

void RecommendationsProcessor::SetStagedAuthors(QList<core::AuthorPtr> list)
{
    stagedAuthors = list;
}

