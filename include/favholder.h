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
#include <QHash>
#include <QSet>
#include <QSharedPointer>
#include <QVector>
#include <QDir>
#include <QSettings>
#include <utility>
#include "GlobalHeaders/SingletonHolder.h"
#include "third_party/roaring/roaring.hh"
#include "include/core/section.h"
#include "threaded_data/threaded_load.h"
#include "threaded_data/threaded_save.h"
#include "include/Interfaces/authors.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/genres.h"
#include "include/reclist_author_result.h"
//namespace interfaces{
//class Authors;
//class Fanfics;
//}


namespace core{


struct RecommendationListResult{
    void AddToBreakdown(uint32_t fic, AuthorWeightingResult::EAuthorType type, double value){
        if(!breakdowns.contains(fic))
            breakdowns[fic].ficId = fic;
        breakdowns[fic].AddAuthor(type, value);
    }
    QHash<int, int> recommendations;
    QHash<int, int> matchReport;
    QHash<uint32_t, MatchBreakdown> breakdowns;
    QList<int> authors;
};

enum ERecDataType
{
    rdt_favourites =0,
    rdt_fics =1,
    rdt_fic_genres_composite = 2,
    rdt_fic_genres_original = 3,
    rdt_author_genre_distribution = 4
};

template<ERecDataType E>
struct DataHolderInfo
{};

template<>
struct DataHolderInfo<rdt_favourites>
{
    static const std::string fileBase(QString suffix = ""){
        if(suffix.isEmpty())
            return "roafav";
        else
            return QString("roafav" + suffix).toStdString();
    }

    typedef  QHash<int, Roaring> type;
    static auto loadFunc (){return [](QSharedPointer<interfaces::Authors> authorInterface){
            auto favourites = authorInterface->LoadFullFavouritesHashset();
            DataHolderInfo<rdt_favourites>::type roaring;
            for(auto key: favourites.keys())
            {
                for(auto item : favourites[key])
                    roaring[key].add(item);
            }
            return roaring;
        };}
};
template<>
struct DataHolderInfo<rdt_fics>
{
    static const std::string fileBase(QString suffix = ""){
        if(suffix.isEmpty())
            return "fics";
        else
            return QString("fics" + suffix).toStdString();
    }
    typedef  QHash<int, core::FicWeightPtr> type;
    static auto loadFunc (){return [](QSharedPointer<interfaces::Fanfics> fanficsInterface){
            return fanficsInterface->GetHashOfAllFicsWithEnoughFavesForWeights(0);
        };}
};
template<>
struct DataHolderInfo<rdt_fic_genres_composite>
{
    static const std::string fileBase(QString suffix = ""){
        if(suffix.isEmpty())
            return "fic_genres_composite";
        else
            return QString("fic_genres_composite" + suffix).toStdString();
    }
    typedef  QHash<int, QList<genre_stats::GenreBit>>  type;
    static auto loadFunc (){return [](QSharedPointer<interfaces::Genres> genreInterface){
            return genreInterface->GetFullGenreList();
        };}
};
template<>
struct DataHolderInfo<rdt_fic_genres_original>
{
    static const std::string fileBase(QString suffix = ""){
        if(suffix.isEmpty())
            return "fic_genres_original";
        else
            return QString("fic_genres_original_" + suffix).toStdString();
    }
    typedef  QHash<int, QString>  type;
    static auto loadFunc (){return [](QSharedPointer<interfaces::Fanfics> fanficsInterface){
            return fanficsInterface->GetGenreForFics();
        };}
};

template<>
struct DataHolderInfo<rdt_author_genre_distribution>
{
    static const std::string fileBase(QString suffix = ""){
        if(suffix.isEmpty())
            return "agd";
        else
            return QString("agd_" + suffix).toStdString();
    }
    typedef  QHash<int, std::array<double, 22>>  type;
    static auto loadFunc (){return [](QSharedPointer<interfaces::Authors> authorInterface){
            return authorInterface->GetListGenreData();
        };}
};


struct DataHolder
{
    typedef DataHolderInfo<rdt_favourites>::type FavType;
    typedef DataHolderInfo<rdt_fics>::type FicType;
    DataHolder(QString settingsFile,
               QSharedPointer<interfaces::Authors> authorsInterface,
               QSharedPointer<interfaces::Fanfics> fanficsInterface)
        :settingsFile(settingsFile),
          authorsInterface(authorsInterface),
          fanficsInterface(fanficsInterface){}

    template <std::size_t I>
    auto get()
    {
        if constexpr (I == rdt_favourites)
                return std::tuple{std::ref(this->faves), this->authorsInterface};
        else if constexpr (I == rdt_fics)
                return std::tuple{std::ref(this->fics), this->fanficsInterface};
    }

    template <ERecDataType T>
    void LoadData(QString storageFolder);

    template <ERecDataType T>
    void SaveData(QString storageFolder){
        CreateTempDataDir(storageFolder);
        auto[data, interface] = get<T>();
        QString fileBase = QString::fromStdString(DataHolderInfo<T>::fileBase());
        thread_boost::SaveData(storageFolder, fileBase, data);
    }
    void CreateTempDataDir(QString storageFolder)
    {
        QDir dir(QDir::currentPath());
        dir.mkdir(storageFolder);
    }

    QString settingsFile;
    QSharedPointer<interfaces::Authors> authorsInterface;
    QSharedPointer<interfaces::Fanfics> fanficsInterface;

    FavType faves;
    FicType fics;
};



class RecommendationList;
class RecCalculator
{
public:
    RecCalculator(QString settingsFile = "", QSharedPointer<interfaces::Authors> authors = {}, QSharedPointer<interfaces::Fanfics> fanfics = {})
        : holder(settingsFile, authors, fanfics){}
    void CreateTempDataDir();
    void LoadFavourites(QSharedPointer<interfaces::Authors> authorInterface);
    void LoadFics(QSharedPointer<interfaces::Fanfics> fanficsInterface);
    void LoadFavouritesDataFromDatabase(QSharedPointer<interfaces::Authors> authorInterface);
    void LoadStoredFavouritesData();
    void SaveFavouritesData();

    RecommendationListResult GetMatchedFicsForFavList(QHash<uint32_t, FicWeightPtr> fetchedFics,
                                                      QSharedPointer<core::RecommendationList> params);
    DataHolder holder;
};



}
BIND_TO_SELF_SINGLE(core::RecCalculator);
