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
#include "Interfaces/genres.h"
#include "pure_sql.h"

namespace interfaces {


QHash<QString, QString> CreateCodeToDBGenreConverter()
{
    QHash<QString, QString> result;
    result["General"] = "General_";
    result["Humor"] = "Humor";
    result["Poetry"] = "Poetry";
    result["Adventure"] = "Adventure";
    result["Mystery"] = "Mystery";
    result["Horror"] = "Horror";
    result["Parody"] = "Parody";
    result["Angst"] = "Angst";
    result["Supernatural"] = "Supernatural";
    result["Suspense"] = "Suspense";
    result["Romance"] = "Romance";
    result["not found"] = "NoGenre";
    result["Sci-Fi"] = "SciFi";
    result["Fantasy"] = "Fantasy";
    result["Spiritual"] = "Spiritual";
    result["Tragedy"] = "Tragedy";
    result["Drama"] = "Drama";
    result["Western"] = "Western";
    result["Crime"] = "Crime";
    result["Family"] = "Family";
    result["Hurt/Comfort"] = "HurtComfort";
    result["Friendship"] = "Friendship";
    return result;
}

QHash<QString, QString> CreateDBToCodeGenreConverter()
{
    QHash<QString, QString> result;
    result["HurtComfort"] = "Hurt/Comfort";
    result["General_"] = "General";
    result["NoGenre"] = "not found";
    result["SciFi"] = "Sci-Fi";
    result["Humor"] = "Humor";
    result["Poetry"] = "Poetry";
    result["Adventure"] = "Adventure";
    result["Mystery"] = "Mystery";
    result["Horror"] = "Horror";
    result["Parody"] = "Parody";
    result["Angst"] = "Angst";
    result["Supernatural"] = "Supernatural";
    result["Suspense"] = "Suspense";
    result["Romance"] = "Romance";
    result["Fantasy"] = "Fantasy";
    result["Spiritual"] = "Spiritual";
    result["Tragedy"] = "Tragedy";
    result["Drama"] = "Drama";
    result["Western"] = "Western";
    result["Crime"] = "Crime";
    result["Family"] = "Family";
    result["Friendship"] = "Friendship";
    return result;
}


bool interfaces::Genres::IsGenreList(QStringList list)
{
    bool success = false;
    for(auto token : list)
        if(genres.contains(token))
        {
            success = true;
            break;
        }
    return success;
}

bool Genres::LoadGenres()
{
    genres = database::puresql::GetAllGenres(db).data;
    if(genres.empty())
        return false;
    return true;
}

genre_stats::FicGenreData Genres::GetGenreDataForFic(int id)
{
    return  database::puresql::GetRealGenresForFic(id, db).data;
}

QVector<genre_stats::FicGenreData> Genres::GetGenreDataForQueuedFics()
{
    return  database::puresql::GetGenreDataForQueuedFics(db).data;
}

void Genres::QueueFicsForGenreDetection(int minAuthorRecs, int minFoundLists)
{
    database::puresql::QueueFicsForGenreDetection(minAuthorRecs, minFoundLists, db);
}

//static QString GetSignificantNeutralType(QStringList list)
//{
//    if(list.contains("Adventure"))
//        return "Adventure";
//    if(list.contains("Scifi"))
//        return "Scifi";
//    if(list.contains("Spiritual"))
//        return "Spiritual";
//    if(list.contains("Supernatural"))
//        return "Supernatural";
//    if(list.contains("Suspense"))
//        return "Suspense";
//    if(list.contains("Mystery"))
//        return "Mystery";
//    if(list.contains("Crime"))
//        return "Crime";
//    if(list.contains("Fantasy"))
//        return "Fantasy";
//    if(list.contains("Western"))
//        return "Western";

//    return "Adventure";
//}


//static QString GetSignificantHumorType(QStringList list)
//{
//    if(list.contains("Parody"))
//        return "Parody";
//    return "Humor";
//}


//static QString GetSignificantDramaType(QStringList list)
//{
//    if(list.contains("Angst"))
//        return "Angst";
//    if(list.contains("Tragedy"))
//        return "Tragedy";

//    return "Drama";
//}

//void GenreConverter::DetectRealGenres(genre_stats::FicGenreData & data)
//{
//    bool romanceHigherThanDrama = (data.strengthRomance - data.strengthDrama) > 0.15f;
//    bool dramaHigherThanRomance= (data.strengthDrama - data.strengthRomance) > 0.15f;
//    bool humorStatisticallySignificant = data.strengthHumor > 0.3f;
//    bool neutralDropsBelowThreshold = data.strengthNeutralComposite < 0.9f;
//    bool neutralIsNotAdventure = (data.strengthNeutralComposite -data.strengthNeutralAdventure) > 0.3f;
//    bool hurtComfortStatisticallySignificant = data.strengthHurtComfort > 0.2f;

//    // leaving horror as is, because author probbaly knows what he is doing
//    // and there's no reliable way to pull proper statisstics for it anyway
//    data.genresToKeep.push_back("Horror");

//    bool humorIsMainGenre = true;
//    for(auto genreData : {data.strengthDrama,data.strengthRomance, data.strengthHurtComfort})
//        if( (data.strengthHumor - genreData) < 0.15f )
//            humorIsMainGenre = false;

//    if(humorIsMainGenre)
//        data.realGenres.push_back({GetSignificantHumorType(data.originalGenres), data.strengthHumor*3});

//    bool romanceIsMainGenre = true;
//    for(auto genreData : {data.strengthDrama,data.strengthHumor, data.strengthHurtComfort})
//        if( (data.strengthRomance - genreData) < 0.15f )
//            romanceIsMainGenre = false;
//    if(romanceIsMainGenre)
//        data.realGenres.push_back({"Romance", data.strengthRomance*2});

//    bool dramaIsMainGenre = true;
//    for(auto genreData : {data.strengthRomance,data.strengthHumor, data.strengthHurtComfort})
//        if( (data.strengthDrama - genreData) < 0.15f )
//            dramaIsMainGenre = false;

//    if(dramaIsMainGenre)
//        data.realGenres.push_back({GetSignificantDramaType(data.originalGenres), data.strengthDrama*2});


//    QString adventureType;

//    if(data.strengthNeutralComposite >= 0.9f)
//        data.realGenres.push_back({GetSignificantNeutralType(data.originalGenres), data.strengthNeutralComposite});

//    if(hurtComfortStatisticallySignificant && (data.strengthHumor < 0.3f))
//    {
//        data.realGenres.push_back({"Hurt/Comfort", data.strengthHurtComfort*3});
//        data.genresToKeep.push_back("Hurt/Comfort");
//    }

//    if(data.strengthBonds > 0.15f)
//    {
//        data.realGenres.push_back({"Bonding", data.strengthBonds*3});
//        data.genresToKeep.push_back("Family");
//        data.genresToKeep.push_back("Friendship");
//    }

//    if(neutralDropsBelowThreshold )
//    {
//        //need to use aux params
//        float dramaticMultiplier = 1;
//        if(data.strengthHurtComfort > 0.15f)
//            dramaticMultiplier += 0.5;
//        if(data.pureDramaAdvantage > 0.025f)
//            dramaticMultiplier += 0.5;

//        if(!humorIsMainGenre &&
//                std::abs(data.strengthDrama - data.strengthRomance) < 0.15f)
//        {
//            data.realGenres.push_back({"Romance", data.strengthRomance*2});
//            data.realGenres.push_back({GetSignificantDramaType(data.originalGenres), data.strengthDrama*2*dramaticMultiplier});
//        }
//        if(humorIsMainGenre)
//        {
//            if((data.strengthDrama - data.strengthRomance) > 0.15f)
//                data.realGenres.push_back({GetSignificantDramaType(data.originalGenres), data.strengthDrama*2});
//            else if((data.strengthRomance - data.strengthDrama) > 0.15f)
//                data.realGenres.push_back({"Romance", data.strengthRomance*2});
//            else if(dramaticMultiplier > 1){
//                data.realGenres.push_back({GetSignificantDramaType(data.originalGenres), data.strengthDrama*2*dramaticMultiplier});
//            }
//        }

//    }

//}


static QStringList GetSignificantNeutralTypes(QStringList list)
{
    QStringList result;
    if(list.contains("Scifi"))
        result.push_back("Scifi");
    if(list.contains("Spiritual"))
        result.push_back("Spiritual");
    if(list.contains("Supernatural"))
        result.push_back("Supernatural");
    if(list.contains("Suspense"))
        result.push_back("Suspense");
    if(list.contains("Mystery"))
        result.push_back("Mystery");
    if(list.contains("Crime"))
        result.push_back("Crime");
    if(list.contains("Fantasy"))
        result.push_back("Fantasy");
    if(list.contains("Western"))
        result.push_back("Western");
    if(result.empty())
        result.push_back("Adventure");
    return result;
}


static QStringList GetSignificantHumorTypes(QStringList list)
{
    QStringList result;
    if(list.contains("Parody"))
        result.push_back("Parody");
    if(result.empty())
        result.push_back("Humor");
    return result;
}


static QStringList GetSignificantDramaTypes(QStringList list)
{
    QStringList result;
    if(list.contains("Angst"))
        result.push_back("Angst");
    if(list.contains("Tragedy"))
        result.push_back("Tragedy");
    if(result.empty())
        result.push_back("Drama");
    return  result;
}

static QStringList GetSignificantFamilyTypes(QStringList list)
{
    QStringList result;
    if(list.contains("Friendship"))
        result.push_back("Friendship");
    if(result.empty())
        result.push_back("Family");
    return  result;
}

static QStringList GetSignificantKeptGenres(QStringList list)
{
    QStringList result;
    QStringList keep = {"Horror", "General", "Poetry"};
    for(auto genre : list)
    {
        if(keep.contains(genre))
            result.push_back(genre);
    }
    return  result;
}

bool HumorDominates(genre_stats::FicGenreData data)
{
    bool result = true;
    for(auto genreData : {data.strengthDrama,data.strengthRomance, data.strengthHurtComfort})
        if( (data.strengthHumor - genreData) < 0.15f )
            result = false;
    result = result && data.strengthHumor > 0.3f;
    return result;
}

bool RomanceDominates(genre_stats::FicGenreData data)
{
    bool result = true;
    for(auto genreData : {data.strengthDrama,data.strengthHumor, data.strengthHurtComfort})
        if( (data.strengthRomance - genreData) < 0.15f )
            result = false;
    result = result && data.strengthRomance > 0.3f;
    return result;
}

bool DramaDominates(genre_stats::FicGenreData data)
{
    bool result = true;
    for(auto genreData : {data.strengthRomance,data.strengthHumor, data.strengthHurtComfort})
        if( (data.strengthDrama - genreData) < 0.15f )
            result = false;
    result = result && data.strengthDrama > 0.3f;
    return result;
}

bool RomanceDramaDominate(genre_stats::FicGenreData data)
{
    bool result = true;
    result = std::abs(data.strengthDrama - data.strengthRomance) < 0.15f;
    for(auto genreData : {data.strengthHumor, data.strengthHurtComfort})
        if( (data.strengthRomance - genreData) < 0.15f )
            result = false;
    result = result && data.strengthDrama > 0.3f;
    return result;
}

bool BondsSignificant(genre_stats::FicGenreData data)
{
    return data.strengthBonds > 0.15f;
}
bool HurtSignificant(genre_stats::FicGenreData data)
{
    return data.strengthBonds > 0.2f;
}

void GenreConverter::ProcessGenreResult(genre_stats::FicGenreData & genreData)
{
    for(auto genre: GetSignificantKeptGenres(genreData.originalGenres))
        genreData.realGenres.push_back({{genre}, 1});

    if(genreData.strengthNeutralComposite >= 0.9f)
        genreData.realGenres.push_back({GetSignificantNeutralTypes(genreData.originalGenres), 1});
    else
        genreData.realGenres.push_back({GetSignificantNeutralTypes(genreData.originalGenres), 1 - (1 - genreData.strengthNeutralComposite)*2.f});


    if(HumorDominates(genreData))
    {
        if(genreData.strengthHumor > 0.5f)
            genreData.realGenres.push_back({GetSignificantHumorTypes(genreData.originalGenres), 1});
        else if(genreData.strengthHumor > 0.4f)
            genreData.realGenres.push_back({GetSignificantHumorTypes(genreData.originalGenres), 0.6f});
        else if(genreData.strengthHumor > 0.3f)
            genreData.realGenres.push_back({GetSignificantHumorTypes(genreData.originalGenres), 0.4f});
    }

    if(RomanceDominates(genreData))
    {
        if(genreData.strengthRomance > 0.7f)
            genreData.realGenres.push_back({{"Romance"}, 1});
        else if(genreData.strengthRomance > 0.5f)
            genreData.realGenres.push_back({{"Romance"}, 0.6f});
        else if(genreData.strengthRomance > 0.3f)
            genreData.realGenres.push_back({{"Romance"}, 0.3f});
    }

    if(DramaDominates(genreData))
    {
        if(genreData.strengthDrama > 0.6f)
            genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 1});
        else if(genreData.strengthDrama > 0.45f)
            genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 0.4f});
        else if(genreData.strengthDrama > 0.3f)
            genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 0.3f});
    }

    if(RomanceDramaDominate(genreData))
    {
        if(genreData.strengthRomance > 0.7f)
        {
            genreData.realGenres.push_back({{"Romance"}, 1});
            genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 1});
        }
        else if(genreData.strengthRomance > 0.45f)
        {
            genreData.realGenres.push_back({{"Romance"}, 0.5f});
            genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 0.5f});
        }
        else if(genreData.strengthRomance > 0.3f)
        {
            genreData.realGenres.push_back({{"Romance"}, 0.3f});
            genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 0.3f});
        }
    }

    if(BondsSignificant(genreData))
    {
        if(genreData.strengthBonds > 0.3f)
            genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 1});
        else if(genreData.strengthBonds > 0.2f)
            genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 0.7f});
        else if(genreData.strengthBonds > 0.15f)
            genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 0.3f});
    }

    if(HurtSignificant(genreData))
    {
        if(genreData.strengthBonds > 0.4f)
            genreData.realGenres.push_back({{"Hurt/Comfort"}, 1});
        else if(genreData.strengthDrama > 0.3f)
            genreData.realGenres.push_back({{"Hurt/Comfort"}, 0.7f});
        else if(genreData.strengthDrama > 0.2f)
            genreData.realGenres.push_back({{"Hurt/Comfort"}, 0.5f});
    }



}

void GenreConverter::DetectRealGenres(genre_stats::FicGenreData & data)
{
    //bool romanceHigherThanDrama = (data.strengthRomance - data.strengthDrama) > 0.15f;
    //bool dramaHigherThanRomance= (data.strengthDrama - data.strengthRomance) > 0.15f;
    //bool humorStatisticallySignificant = data.strengthHumor > 0.3f;
    bool neutralDropsBelowThreshold = data.strengthNeutralComposite < 0.9f;
    //bool neutralIsNotAdventure = (data.strengthNeutralComposite -data.strengthNeutralAdventure) > 0.3f;
    bool hurtComfortStatisticallySignificant = data.strengthHurtComfort > 0.2f;

    // leaving horror as is, because author probbaly knows what he is doing
    // and there's no reliable way to pull proper statisstics for it anyway
    data.genresToKeep.push_back("Horror");

    // we have  two cases
    // one where adventure firmly sits above 0.9 as a dominant genre
    // two where genre is spread among secondary ones of humor, drama and romance

    // in case of first we only need to pick a secondary genre
    // humor is simple, if it's dominant
    // romance most likely has to be disambiguated with drama
    // whatever is dominant gets picked
    // otherwise we check if hurtcomfort is high and multiply drama
    // we also check that fic has more drama lists than romance (in case hurtcomfort isnt an indicator enough)

    // we then multiply a secondary genre to estimate how relevant it is relative to adventure
    // humor, family and hurtcomfot by 3
    // romance and drama by 2

    // in case where adventure pack dips below 0.9 we cant be sure that it's a primary
    // we establish a primary genre (disambiguating R&D as usual)
    // we obly inject adventure as a genre ito these if it's above 0.7
    // humor if it's above 0.3
    // if romance and drama are equal amount, drop both to 2 bars


    bool humorIsMainGenre = true;
    for(auto genreData : {data.strengthDrama,data.strengthRomance, data.strengthHurtComfort})
        if( (data.strengthHumor - genreData) < 0.15f )
            humorIsMainGenre = false;

    if(humorIsMainGenre)
        data.realGenres.push_back({GetSignificantHumorTypes(data.originalGenres), data.strengthHumor*3});

    bool romanceIsMainGenre = true;
    for(auto genreData : {data.strengthDrama,data.strengthHumor, data.strengthHurtComfort})
        if( (data.strengthRomance - genreData) < 0.15f )
            romanceIsMainGenre = false;
    if(romanceIsMainGenre)
        data.realGenres.push_back({{"Romance"}, data.strengthRomance*2});

    bool dramaIsMainGenre = true;
    for(auto genreData : {data.strengthRomance,data.strengthHumor, data.strengthHurtComfort})
        if( (data.strengthDrama - genreData) < 0.15f )
            dramaIsMainGenre = false;

    if(dramaIsMainGenre)
        data.realGenres.push_back({GetSignificantDramaTypes(data.originalGenres), data.strengthDrama*2});


    QString adventureType;

    if(data.strengthNeutralComposite >= 0.9f)
        data.realGenres.push_back({GetSignificantNeutralTypes(data.originalGenres), data.strengthNeutralComposite});

    if(hurtComfortStatisticallySignificant && (data.strengthHumor < 0.3f))
    {
        data.realGenres.push_back({{"Hurt/Comfort"}, data.strengthHurtComfort*3});
        data.genresToKeep.push_back("Hurt/Comfort");
    }

    if(data.strengthBonds > 0.15f)
    {
        data.realGenres.push_back({GetSignificantFamilyTypes(data.originalGenres), data.strengthBonds*3});
        data.genresToKeep.push_back("Family");
        data.genresToKeep.push_back("Friendship");
    }

    if(neutralDropsBelowThreshold )
    {
        //need to use aux params
        float dramaticMultiplier = 1;
        if(data.strengthHurtComfort > 0.15f)
            dramaticMultiplier += 0.5;
        if(data.pureDramaAdvantage > 0.025f)
            dramaticMultiplier += 0.5;

        if(!humorIsMainGenre &&
                std::abs(data.strengthDrama - data.strengthRomance) < 0.15f)
        {
            data.realGenres.push_back({{"Romance"}, data.strengthRomance*2});
            data.realGenres.push_back({GetSignificantDramaTypes(data.originalGenres), data.strengthDrama*2*dramaticMultiplier});
        }
        if(humorIsMainGenre)
        {
            if((data.strengthDrama - data.strengthRomance) > 0.15f)
                data.realGenres.push_back({GetSignificantDramaTypes(data.originalGenres), data.strengthDrama*2});
            else if((data.strengthRomance - data.strengthDrama) > 0.15f)
                data.realGenres.push_back({{"Romance"}, data.strengthRomance*2});
            else if(dramaticMultiplier > 1){
                data.realGenres.push_back({GetSignificantDramaTypes(data.originalGenres), data.strengthDrama*2*dramaticMultiplier});
            }
        }

    }

}

GenreConverter GenreConverter::Instance()
{
    thread_local GenreConverter instance;
    return instance;
}


}
