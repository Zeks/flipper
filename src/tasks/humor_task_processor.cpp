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
#include "tasks/humor_task_processor.h"
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
#include "include/EGenres.h"
#include <array>
#include <cmath>

#include <QThread>

HumorProcessor::HumorProcessor(QSqlDatabase db,
                               QSharedPointer<interfaces::Fanfics> fanficInterface,
                               QSharedPointer<interfaces::Fandoms> fandomsInterface,
                               QSharedPointer<interfaces::PageTask> pageInterface,
                               QSharedPointer<interfaces::Authors> authorsInterface,
                               QSharedPointer<interfaces::RecommendationLists> recsInterface,
                               QSharedPointer<database::IDBWrapper> dbInterface, QObject *obj) : QObject(obj)

{
    this->fanficsInterface = fanficInterface;
    this->fandomsInterface = fandomsInterface;
    this->pageInterface = pageInterface;
    this->dbInterface = dbInterface;
    this->authorsInterface = authorsInterface;
    this->recsInterface = recsInterface;
    this->db = db;
}

HumorProcessor::~HumorProcessor()
{

}


inline void HumorProcessor::AddToCountingHumorHash(QList<core::AuthorPtr> authors,
                                          QHash<int, int>& countingHash,
                                          QHash<int, double>& valueHash,
                                          QHash<int, double>& totalHappiness,
                                          QHash<int, double>& totalSlash)
{
    QList<double> coeffs ;
    coeffs = {0.2, 0.4, 0.7, 1}; //second
    for(const auto& author : authors)
    {
        auto fics = authorsInterface->GetFicList(author);
        double listvalue;
        listvalue = 1;

        for(auto fic : fics)
        {
            countingHash[fic]++;
            valueHash[fic] += listvalue;// * pow(author->stats.favouriteStats.moodHappy, 1/2);
            totalHappiness[fic] += author->stats.favouriteStats.moodHappy;
            totalSlash[fic] += author->stats.favouriteStats.slashRatio;
        }
    }
}


void HumorProcessor::CreateListOfHumorCandidates(QList<core::AuthorPtr > authors)
{
    QSqlDatabase db = QSqlDatabase::database();
    database::Transaction transaction(db);

    QHash<int, std::array<double, 22>> authorGenreHash;
    QHash<int, std::array<double, 22>> ficGenreHash;
    QHash<int, double> reviewRatios;
    TimedAction getReviewRatios("get review ratios", [&](){
        reviewRatios = fanficsInterface->GetDoubleValueHashForFics("reviewstofavourites");
    });
    getReviewRatios.run();

    TimedAction getGenres("getGenres", [&](){
        authorGenreHash = authorsInterface->GetListGenreData();
    });
    getGenres.run();
    TimedAction getFicGenres("getGenres", [&](){
        ficGenreHash = fanficsInterface->GetFullFicGenreData();
    });
    getFicGenres.run();



    QList<core::AuthorPtr> humorAuthors;
    QList<core::AuthorPtr> validAuthors;
    for(const auto& author : authors)
    {
        if(author->stats.favouriteStats.favourites < 50)
            continue;
        validAuthors.push_back(author);

        auto& stats = author->stats.favouriteStats;
        if(stats.moodHappy > 0.6 && authorGenreHash[author->id][10] < 0.5)
            humorAuthors.push_back(author);
    }
    QHash<int, int> humorFics;
    QHash<int, double> humorValues;
    QHash<int, double> dummyValues;
    QHash<int, int> allFics;
    QHash<int, double> totalHappiness;
    QHash<int, double> totalSlash;
    QHash<int, double> dummyHappiness;
    QHash<int, double> dummySlash;
    TimedAction processHumor("ProcHumor", [&](){
        AddToCountingHumorHash(humorAuthors,
                          humorFics,
                          humorValues,
                          totalHappiness,
                          dummySlash);
        AddToCountingHumorHash(validAuthors, allFics, dummyValues, dummyHappiness,totalSlash );
    });

    processHumor.run();
    QHash<int, int> relativeFics;

    for(auto i = humorFics.cbegin(); i != humorFics.cend(); i++)
    {
        auto fic = i.key();
        double averageHappiness = totalHappiness[fic]/static_cast<double>(i.value());
        auto appearanceInHumor = static_cast<double>(i.value());
        double adjustedHumorValue;
        double logvalue = log10(humorValues[fic]);
        if(logvalue < 1)
            logvalue = 1;
        adjustedHumorValue = static_cast<double>(humorValues[fic])/logvalue;
        double listSizeAdjuster = 1.;
        if(appearanceInHumor < 5)
            listSizeAdjuster = 0.2;
        else if(appearanceInHumor < 10)
            listSizeAdjuster = 0.4;
        else if(appearanceInHumor < 50)
            listSizeAdjuster = 0.8;
        else
            listSizeAdjuster = 1;

        double ficGenreAdjuster = 1;
        if(ficGenreHash[fic][genres::Humor] < 0.1)
            ficGenreAdjuster = 0.3;
        else if(ficGenreHash[fic][genres::Humor] < 0.2)
            ficGenreAdjuster = 0.75;
        else if(ficGenreHash[fic][genres::Humor] > 0.4)
            ficGenreAdjuster = 1.2;

        double genrePreferenceAdjuster = 1;
        double preferenceValue = ficGenreHash[fic][genres::Humor]  + ficGenreHash[fic][genres::Parody];
        if(preferenceValue > ficGenreHash[fic][genres::Romance] &&
                preferenceValue > ficGenreHash[fic][genres::Adventure] &&
                preferenceValue > ficGenreHash[fic][genres::Drama])
            genrePreferenceAdjuster = 1.5;
        if((ficGenreHash[fic][genres::Adventure] - preferenceValue) > 0.2 )
            genrePreferenceAdjuster = 0.8;
        if((ficGenreHash[fic][genres::Romance] - preferenceValue) > 0.15 )
            genrePreferenceAdjuster = 0.5;

        double reviewRatioAdjuster = 1;
        if(reviewRatios[fic] > 1.5)
            reviewRatioAdjuster = 0.3;




        relativeFics[fic] = static_cast<int>(100*adjustedHumorValue
                                             *averageHappiness
                                             *listSizeAdjuster
                                             *ficGenreAdjuster
                                             *genrePreferenceAdjuster
                                             *reviewRatioAdjuster);
    }

    TimedAction writeSlashLists("WriteHumor", [&](){
        recsInterface->CreateRecommendationList("HumorAlgo", relativeFics);
    });
    writeSlashLists.run();

    transaction.finalize();
    qDebug () << "finished";
}



inline void HumorProcessor::AddToHumorHash(QList<core::AuthorPtr> authors,QHash<int, int>& countingHash)
{
    for(const auto& author : std::as_const(authors))
    {
        auto fics = authorsInterface->GetFicList(author);
        for(auto fic : fics)
        {
            //qDebug() << "adding fic: " << fic;
            countingHash[fic]++;
        }
    }
}

void HumorProcessor::CreateRecListOfHumorProfiles(QList<core::AuthorPtr> authors)
{
    database::Transaction transaction(db);

    QHash<int, std::array<double, 22>> authorGenreHash;

    TimedAction getGenres("getGenres", [&](){
        authorGenreHash = authorsInterface->GetListGenreData();
    });
    getGenres.run();

    QList<core::AuthorPtr> humorAuthors;
    //QList<core::AuthorPtr> validAuthors;
    for(const auto& author : authors)
    {
        if(author->stats.favouriteStats.favourites < 50)
            continue;

        auto& stats = author->stats.favouriteStats;
        if(stats.moodHappy > 0.5)
            humorAuthors.push_back(author);
    }
    qDebug() << "humor size: " << humorAuthors.size();
    QHash<int, int> humorFics;
//    QHash<int, double> humorValues;
//    QHash<int, double> dummyValues;
//    QHash<int, int> allFics;
//    QHash<int, double> totalHappiness;
//    QHash<int, double> totalSlash;
//    QHash<int, double> dummyHappiness;
//    QHash<int, double> dummySlash;
    TimedAction processHumor("ProcHumor", [&](){
        AddToHumorHash(humorAuthors,humorFics);
    });
    processHumor.run();

    TimedAction writeSlashLists("WriteHumor", [&](){
        recsInterface->CreateRecommendationList("HumorRecs", humorFics);
    });
    writeSlashLists.run();
    transaction.finalize();
    qDebug () << "finished";
}
