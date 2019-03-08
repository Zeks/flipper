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
#include "tasks/slash_task_processor.h"
#include "include/pagegetter.h"
#include "include/pagetask.h"
#include "include/parsers/ffn/fandomparser.h"
#include "include/transaction.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/fandoms.h"
#include "include/Interfaces/authors.h"
#include "include/Interfaces/pagetask_interface.h"
#include "include/Interfaces/recommendation_lists.h"
#include "include/url_utils.h"
#include "include/timeutils.h"

#include <QThread>

SlashProcessor::SlashProcessor(QSqlDatabase db,
                               QSharedPointer<interfaces::Fanfics> fanficInterface,
                               QSharedPointer<interfaces::Fandoms> fandomsInterface,
                               QSharedPointer<interfaces::Authors> authorsInterface,
                               QSharedPointer<interfaces::RecommendationLists> recsInterface,
                               QSharedPointer<database::IDBWrapper> dbInterface, QObject *obj) : QObject(obj)

{
    this->fanficsInterface = fanficInterface;
    this->fandomsInterface = fandomsInterface;
    this->dbInterface = dbInterface;
    this->authorsInterface = authorsInterface;
    this->recsInterface = recsInterface;
    this->db = db;
}

SlashProcessor::~SlashProcessor()
{

}


inline void SlashProcessor::AddToSlashHash(QList<core::AuthorPtr> authors,
                                       QSet<int> knownNotSlashFics,
                                       QHash<int, int>& slashHash,
                                       bool checkRx)
{
    CommonRegex rx;
    rx.Init();
    for(auto author : authors)
    {
        auto fics = authorsInterface->GetFicList(author);
        for(auto fic : fics)
        {
            if(checkRx)
            {
                if(knownNotSlashFics.contains(fic))
                    continue;
            }
            slashHash[fic]++;
        }
    }
}

void SlashProcessor::CreateListOfSlashCandidates(double neededNotslashMatchesCoeff, QList<core::AuthorPtr > authors)
{
    QSqlDatabase db = QSqlDatabase::database();
    database::Transaction transaction(db);

    auto keyWordSlashRepo = fanficsInterface->GetAllKnownSlashFics();
    auto keyWordNotSlashRepo = fanficsInterface->GetAllKnownNotSlashFics();

    QHash<int, QList<core::AuthorPtr>> slashAuthors;
    QList<core::AuthorPtr> notSlashAuthors;


    for(auto author : authors)
    {
        auto& stats = author->stats.favouriteStats;
        if(stats.slashRatio > 0.7)
            slashAuthors[0].push_back(author);
        if(stats.slashRatio > 0.35)
            slashAuthors[1].push_back(author);

        // will need to inject a minimal favourite count clause for fics that are present just once
        // catching odd slash in other fandoms which should be smut instead
        if(stats.slashRatio > 0.15)
            slashAuthors[2].push_back(author);

        if(stats.slashRatio < 0.01 && stats.favourites > 5)
            notSlashAuthors.push_back(author);
    }

    // first is slash certainty, 0 is the highest
    // second is fic id
    // third is fic count
    qDebug() << "slash 2 authors: " << slashAuthors[2].size();
    QHash<int, QHash<int, int>> slashFics;
    QHash<int, int> notSlashFics;

    TimedAction processSlash("ProcSlash", [&](){
        AddToSlashHash(slashAuthors[0], keyWordNotSlashRepo, slashFics[0]);
        AddToSlashHash(slashAuthors[1], keyWordNotSlashRepo, slashFics[1]);
        AddToSlashHash(slashAuthors[2], keyWordNotSlashRepo, slashFics[2]);
    });
    processSlash.run();

    TimedAction writeSlashLists("WriteSlash", [&](){
        recsInterface->CreateRecommendationList("SlashSure", slashFics[0]);
        recsInterface->CreateRecommendationList("SlashProbably", slashFics[1]);
        recsInterface->CreateRecommendationList("SlashMaybe", slashFics[2]);
    });
    writeSlashLists.run();
    TimedAction processNotSlash("ProcNotSlash", [&](){
        AddToSlashHash(notSlashAuthors, keyWordSlashRepo, notSlashFics, false);
    });
    processNotSlash.run();
    QList<int> intersection;
    intersection.reserve(50000);

    // sufficient matches depends on if a fic is present in lists 0 and 1
    // 0 - 2 matches or more requires 7 non slash
    // 1 - 5 matches or more requires 5 non slash
    QSet<int> inSoleLargeLists = fanficsInterface->GetSingularFicsInLargeButSlashyLists();
    qDebug() << "Size of additional exclusions: " << inSoleLargeLists.size();
    int sufficientMatchesCount = 3;
    qDebug() << "Slash 2 size before filtering: " << slashFics[2].size();
    auto TRepo = fanficsInterface->GetAllKnownFicIDs(" rated <> 'M' ");
    int exclusionTriggers = 0;
    TimedAction intersect("Intersect", [&](){
        for(const auto& fic: slashFics[2].keys())
        {

            if(slashFics[0][fic] >= 2)
                sufficientMatchesCount = 7;
            else if(slashFics[1][fic] >= 5)
                sufficientMatchesCount = 5;

            bool soleTMatch = (TRepo.contains(fic) && slashFics[2][fic]==1);
            bool cantTellReliably = slashFics[2][fic]==1 && slashFics[1][fic] == 0;
            bool sufficientMatches = notSlashFics[fic] >= static_cast<double>(sufficientMatchesCount)/neededNotslashMatchesCoeff;
            if(fic == 86768)
            {
                qDebug() << "Sole T: " << soleTMatch << " CantReliably: " << cantTellReliably << " Sufficient matches: "  << sufficientMatches;
            }
            bool exclusionTriggered = inSoleLargeLists.contains(fic);
            if(exclusionTriggered)
                exclusionTriggers++;
            if(!keyWordSlashRepo.contains(fic) && (exclusionTriggered || cantTellReliably || soleTMatch || sufficientMatches))
                intersection.push_back(fic);
            else
            {
                bool sufficientMatches = notSlashFics[fic] >= 5;
            }
        }
    });
    intersect.run();
    qDebug() << "Additional exclusion triggered: " << exclusionTriggers << " times";
    qDebug() << "Intersection size: " << intersection.size();
    //qDebug() << intersection;
    QHash<int, int> filteredSlashFics;

    TimedAction filter("Fill Intersection", [&](){
        for(auto fic : intersection)
        {
            filteredSlashFics[fic]=slashFics[2][fic];
            slashFics[2].remove(fic);
        }
    });
    filter.run();
    qDebug() << "Slash 2 size after filtering: " << slashFics[2].size();
    qDebug() << "Filtered size after filtering: " << filteredSlashFics.size();

    recsInterface->CreateRecommendationList("SlashCleaned", slashFics[2]);
    recsInterface->CreateRecommendationList("SlashFilteredOut", filteredSlashFics);

    transaction.finalize();
    qDebug () << "finished";
}

void SlashProcessor::AssignSlashKeywordsMetaInfomation(QSqlDatabase db)
{
    database::Transaction transaction(db);
    CommonRegex rx;
    rx.Init();
    fanficsInterface->ProcessSlashFicsBasedOnWords( [&](QString summary, QString characters, QString fandoms){
        auto result = rx.ContainsSlash(summary, characters, fandoms);
        return result;
    });
    transaction.finalize();
}

void SlashProcessor::DoFullCycle(QSqlDatabase db, int passCount)
{
    auto authors = authorsInterface->GetAllAuthors("ffn", true);
    {
        database::Transaction transaction(db);
        qDebug() << "Assigning metainformation for first pass";
        AssignSlashKeywordsMetaInfomation(db);
        qDebug() << "Calculating statistics for first pass";
        authorsInterface->CalculateSlashStatisticsPercentages("keywords_pass_result");
        qDebug() << "Calculated statistics for first pass";
        transaction.finalize();
    }
    qDebug() << "Starting iterations";
    for(int i = 1; i < passCount+1; i++)
    {
        qDebug() << "Iteration: " << i;
        {
            database::Transaction transaction(db);
            auto authors = authorsInterface->GetAllAuthors("ffn", true);
            qDebug() << "Calculating statistics for pass";
            CreateListOfSlashCandidates(i, authors);
            qDebug() << "Assigning iteration";
            fanficsInterface->AssignIterationOfSlash("pass_" + QString::number(i));
            qDebug() << "Calculating in database";
            authorsInterface->CalculateSlashStatisticsPercentages("pass_" + QString::number(i));
            transaction.finalize();
            lastI = i;
        }
    }
}
