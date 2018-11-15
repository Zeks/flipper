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
#include "include/favholder.h"
#include "include/Interfaces/authors.h"
#include "logger/QsLog.h"
#include "include/timeutils.h"
#include "threaded_data/threaded_save.h"
#include "threaded_data/threaded_load.h"
#include <QSettings>
#include <QDir>
#include <algorithm>
#include <execution>
#include <cmath>
namespace core{

auto lambda = [](DataHolder* holder, QString storageFolder, QString fileBase, auto& data, auto interface, auto loadFunc, auto saveFunc)->void{
    holder->CreateTempDataDir(storageFolder);
    QSettings settings(holder->settingsFile, QSettings::IniFormat);
    if(settings.value("Settings/usestoreddata", true).toBool() && QFile::exists(storageFolder + "/" + fileBase + "_0.txt"))
        thread_boost::LoadData(storageFolder, fileBase, data);
    else
    {
        auto& item = data;
        item = loadFunc(interface);
        saveFunc(storageFolder);
    }
};

#define DISPATCH(X) \
template <> \
void DataHolder::LoadData<X>(QString storageFolder){ \
    auto[data, interface] = get<X>(); \
            lambda(this,storageFolder, "", data.get(),interface, DataHolderInfo<X>::loadFunc(),\
            std::bind(&DataHolder::SaveData<X>, this, std::placeholders::_1));\
}

DISPATCH(rdt_favourites)
DISPATCH(rdt_fics)
//            template <>
//void DataHolder::LoadData<rdt_fics>(QString storageFolder){
//auto[data, interface] = get<rdt_fics>();
//    lambda(this, storageFolder,"", data.get(),interface, DataHolderInfo<rdt_fics>::loadFunc(),
//    std::bind(&DataHolder::SaveData<rdt_favourites>, this, std::placeholders::_1));
//}



                void RecCalculator::LoadFavourites(QSharedPointer<interfaces::Authors> authorInterface)
        {
            CreateTempDataDir();
            QSettings settings("settings_server.ini", QSettings::IniFormat);
            if(settings.value("Settings/usestoreddata", false).toBool() && QFile::exists("ServerData/roafav_0.txt"))
            {
                LoadStoredFavouritesData();
            }
            else
            {
                LoadFavouritesDataFromDatabase(authorInterface);
                SaveFavouritesData();
            }
        }

        void RecCalculator::CreateTempDataDir()
        {
            QDir dir(QDir::currentPath());
            dir.mkdir("ServerData");
        }

        void RecCalculator::LoadFavouritesDataFromDatabase(QSharedPointer<interfaces::Authors> authorInterface)
        {
            //    auto favourites = authorInterface->LoadFullFavouritesHashset();
            //    for(auto key: favourites.keys())
            //    {
            //        for(auto item : favourites[key])
            //        this->favourites[key].add(item);
            //    }
        }

        void RecCalculator::LoadStoredFavouritesData()
        {
            //thread_boost::LoadFavouritesData("ServerData", favourites);
        }

        void RecCalculator::SaveFavouritesData()
        {
            //thread_boost::SaveFavouritesData("ServerData", favourites);
        }

        struct AuthorResult{
            int id;
            int matches;
            double ratio;
            int size;
            double distance = 0;
        };

        double quadratic_coef(double ratio, double median, double sigma, int base, int scaler)
        {
            auto  distanceFromMedian = median - ratio;
            if(distanceFromMedian < 0)
                return 0;
            double tau = distanceFromMedian/sigma;
            return base + std::pow(tau,2)*scaler;
        }

        double sqrt_coef(double ratio, double median, double sigma, int base, int scaler)
        {
            auto  distanceFromMedian = median - ratio;
            if(distanceFromMedian < 0)
                return 0;
            double tau = distanceFromMedian/sigma;
            return base + std::sqrt(tau)*scaler;
        }


        RecommendationListResult RecCalculator::GetMatchedFicsForFavList(QHash<uint32_t, core::FicWeightPtr> fetchedFics, QSharedPointer<RecommendationList> params)
        {
            QHash<int, AuthorResult> authorsResult;
            RecommendationListResult ficResult;
            //QHash<int, int> ficResult;
            auto& favs = holder.faves;
            qDebug() << "faves is of size: " << favs.size();
            auto sourceFics = QSet<uint32_t>::fromList(fetchedFics.keys());
            Roaring r;
            for(auto bit : sourceFics)
                r.add(bit);
            qDebug() << "finished creating roaring";
            int minMatches, maxMatches;
            minMatches =  params->minimumMatch;
            maxMatches = minMatches;
            int matchSum = 0;
            double ratioSum = 0;
            TimedAction action("Reclist Creation",[&](){
                auto it = favs.begin();
                while (it != favs.end())
                {
                    auto& author = authorsResult[it.key()];
                    author.id = it.key();
                    author.matches = 0;
                    author.size = it.value().cardinality();
                    Roaring temp = r;
                    temp = temp & it.value();
                    author.matches = temp.cardinality();
                    if(maxMatches < author.matches)
                        maxMatches = author.matches;
                    matchSum+=author.matches;

                    it++;
                }
                QList<int> filteredAuthors;
                filteredAuthors.reserve(authorsResult.size()/10);
                for(auto& author: authorsResult)
                {
                    if(author.matches >= params->minimumMatch || author.matches >= params->alwaysPickAt)
                    {
                        author.ratio = static_cast<double>(author.size)/static_cast<double>(author.matches);

                        if(author.ratio <= params->pickRatio && author.matches > 0)
                        {
                            qDebug() << "detected ratio of: " << author.ratio;
                            ratioSum+=author.ratio;
                            filteredAuthors.push_back(author.id);
                        }
                    }
                }
                int matchMedian = matchSum/favs.size();
                double ratioMedian = static_cast<double>(ratioSum)/static_cast<double>(filteredAuthors.size());

                double normalizer = 1./static_cast<double>(filteredAuthors.size()-1.);
                double sum = 0;
                for(auto author: filteredAuthors)
                {
                    sum+=std::pow(authorsResult[author].ratio - ratioMedian, 2);
                }
                auto quad = std::sqrt(normalizer * sum);


                qDebug () << "median value is: " << matchMedian;
                qDebug () << "median ratio is: " << ratioMedian;

                auto keysRatio = favs.keys();
                auto keysMedian = favs.keys();
                std::sort(std::execution::par, keysMedian.begin(), keysMedian.end(),[&](const int& i1, const int& i2){
                    return authorsResult[i1].matches < authorsResult[i2].matches;
                });

                std::sort(std::execution::par, filteredAuthors.begin(), filteredAuthors.end(),[&](const int& i1, const int& i2){
                    return authorsResult[i1].ratio < authorsResult[i2].ratio;
                });

                auto ratioMedianIt = std::lower_bound(filteredAuthors.begin(), filteredAuthors.end(), ratioMedian, [&](const int& i1, const int& i2){
                    return authorsResult[i1].ratio < ratioMedian;
                });
                auto dist = ratioMedianIt - filteredAuthors.begin();
                qDebug() << "distance to median is: " << dist;
                qDebug() << "vector size is: " << filteredAuthors.size();

                qDebug() << "sigma: " << quad;
                qDebug() << "2 sigma: " << quad * 2;

                auto ratioSigma2 = std::lower_bound(filteredAuthors.begin(), filteredAuthors.end(), ratioMedian, [&](const int& i1, const int& i2){
                    return authorsResult[i1].ratio < (ratioMedian - quad*2);
                });
                auto sigma2Dist = ratioSigma2 - filteredAuthors.begin();
                qDebug() << "distance to sigma15 is: " << sigma2Dist;
                int counter2Sigma = 0;
                int counter17Sigma = 0;
                int authorSize = filteredAuthors.size();
                for(auto& author: authorsResult)
                {
                    if((author.matches >= params->minimumMatch && author.ratio <= params->pickRatio)
                            || author.matches >= params->alwaysPickAt)
                    {
                        bool gtSigma = (ratioMedian - quad) >= author.ratio;
                        bool gtSigma2 = (ratioMedian - 2 * quad) >= author.ratio;
                        bool gtSigma17 = (ratioMedian - 1.7 * quad) >= author.ratio;

                        double coef = 0;
                        if(gtSigma2)
                        {
                            counter2Sigma++;
                            coef = quadratic_coef(author.ratio,ratioMedian,2*quad,authorSize/10, authorSize/20);
                        }
                        else if(gtSigma17)
                        {
                            counter17Sigma++;
                            coef = quadratic_coef(author.ratio,ratioMedian, 1.7*quad,authorSize/20, authorSize/40);
                        }
                        else if(gtSigma)
                            coef = quadratic_coef(author.ratio,ratioMedian, quad, 1, 5);

                        for(auto fic: favs[author.id])
                            ficResult.recommendations[fic]+= 1+coef;
                    }
                }
                qDebug() << "All: " << filteredAuthors.size() << " 2s: " << counter2Sigma << " 17s: " << counter17Sigma;
            });
            action.run();
            for(auto author: authorsResult)
            {
                if((author.matches >= params->minimumMatch && author.ratio <= params->pickRatio)
                        || author.matches >= params->alwaysPickAt)
                {
                    ficResult.matchReport[author.matches]++;
                }
            }
            QList<AuthorResult> authors = authorsResult.values();
            std::sort(authors.begin(), authors.end(), [](const AuthorResult& a1, const AuthorResult& a2){
                return a1.matches > a2.matches;
            });
            qDebug() << "/////////////////// AUTHOR IDs: ////////////////";
            int counter = 0;
            for(int i = 0; counter < 15 && i < authors.size(); i++)
            {
                if(authors[i].size < 800)
                {
                    qDebug() << authors[i].id;
                    counter++;
                }
            }
            return ficResult;
        }





    }
