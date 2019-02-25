#pragma once

#include "Interfaces/fanfics.h"
#include "Interfaces/authors.h"
#include "Interfaces/genres.h"
#include "threaded_data/threaded_load.h"
#include "threaded_data/threaded_save.h"
#include "third_party/roaring/roaring.hh"


#include <QString>
#include <QHash>
#include <QSharedPointer>
#include <QDir>

#include <string>


namespace core{

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


}
