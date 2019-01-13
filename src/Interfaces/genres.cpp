/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2018  Marchenko Nikolai

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


Genres::Genres()
{
    index.Init();
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

void Genres::QueueFicsForGenreDetection(int minAuthorRecs, int minFoundLists,int minFaves)
{
    database::puresql::QueueFicsForGenreDetection(minAuthorRecs, minFoundLists, minFaves, db);
}

bool Genres::WriteDetectedGenres(QVector<genre_stats::FicGenreData> fics)
{
    return database::puresql::WriteDetectedGenres(fics, db).success;
}

bool Genres::WriteDetectedGenresIteration2(QVector<genre_stats::FicGenreData> fics)
{
    return database::puresql::WriteDetectedGenresIteration2(fics, db).success;
}

QHash<int, QList<genre_stats::GenreBit> > Genres::GetFullGenreList()
{
    return database::puresql::GetFullGenreList(db).data;
}

void Genres::LogGenreDistribution(std::array<double, 22> &data, QString target)
{
    An<interfaces::GenreIndex> genreIndex;
    QLOG_INFO() << "///////////////////";
    if(!target.isEmpty())
        QLOG_INFO() << target;
    for(size_t i = 0; i < 22; i++)
    {

        auto genre = genreIndex->genresByIndex[i];
        auto value = std::abs(data[i]) > 0.005 ? data[i] : 0;
        QLOG_INFO() << genre.name << ": " << value;
    }
    QLOG_INFO() << "///////////////////";

}


static QStringList GetSignificantNeutralTypes(QStringList list, bool addAdventure = false)
{
    QStringList result;
    if(list.contains("Sci-Fi"))
        result.push_back("Sci-Fi");
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
    if((addAdventure && result.empty()) || list.contains("Adventure"))
        result.push_back("Adventure");
    return result;
}


static QStringList GetSignificantHumorTypes(QStringList list)
{
    QStringList result;
    if(list.contains("Parody"))
        result.push_back("Parody");
    if(result.empty() || list.contains("Humor"))
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
    if(result.empty() || list.contains("Drama"))
        result.push_back("Drama");
    return  result;
}

static QStringList GetSignificantFamilyTypes(QStringList list)
{
    QStringList result;
    if(list.contains("Friendship"))
        result.push_back("Friendship");
    if(result.empty() || list.contains("Family"))
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
        if( (data.strengthRomance - genreData) < 0.1f )
            result = false;
    result = result && data.strengthRomance > 0.3f;
    return result;
}

bool DramaDominates(genre_stats::FicGenreData data)
{
    bool result = true;
    for(auto genreData : {data.strengthRomance,data.strengthHumor, data.strengthHurtComfort})
        if( (data.strengthDrama - genreData) < 0.1f )
            result = false;
    result = result && data.strengthDrama > 0.3f;
    return result;
}

bool RomanceDramaDominate(genre_stats::FicGenreData data)
{
    bool result = true;
    result = std::abs(data.strengthDrama - data.strengthRomance) < 0.15f;
    for(auto genreData : {data.strengthHumor, data.strengthHurtComfort})
        if( (data.strengthRomance - genreData) < 0.1f )
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

// need to filter it up to 3 genres
// whatever is kept as is has relevance 0
QVector<genre_stats::GenreBit>  FinalGenreProcessing(genre_stats::FicGenreData& data)
{
    QVector<genre_stats::GenreBit> result;

    std::sort(data.realGenres.begin(),data.realGenres.end(),[](const genre_stats::GenreBit& g1,const genre_stats::GenreBit& g2){
        if(g1.genres.size() != 0 && g2.genres.size() == 0)
            return true;
        if(g2.genres.size() != 0 && g1.genres.size() == 0)
            return false;

        return g1.relevance > g2.relevance;
    });

    int size = data.realGenres.size() < 3 ? data.realGenres.size() : 3;
    for(int i = 0 ; i< size; i++)
    {
        if(data.realGenres.at(i).relevance > 0)
            result.push_back(data.realGenres.at(i));
    }
    for(auto kept: data.genresToKeep)
        result.push_back({{kept}, 0});
    return result;
}

void GenreConverter::ProcessGenreResult(genre_stats::FicGenreData & genreData)
{
    QVector<genre_stats::GenreBit> result;

    for(auto genre: GetSignificantKeptGenres(genreData.originalGenres))
        genreData.realGenres.push_back({{genre}, 1});

    if(genreData.strengthNeutralAdventure >= 0.8f)
        genreData.realGenres.push_back({GetSignificantNeutralTypes(genreData.originalGenres, true), 1 - (0.8f - genreData.strengthNeutralAdventure)*2.f});
    else if (genreData.strengthNeutralComposite >= 0.8f && genreData.strengthNeutralAdventure >= 0.5f)
        genreData.realGenres.push_back({GetSignificantNeutralTypes(genreData.originalGenres, true), 1 - (0.8f - genreData.strengthNeutralAdventure)*2.f});

    else if(genreData.strengthNeutralComposite >= 0.6f)
        genreData.realGenres.push_back({GetSignificantNeutralTypes(genreData.originalGenres), 1 - (0.8f - genreData.strengthNeutralComposite)*2.f});


    if(genreData.strengthHumor > 0.5f)
        genreData.realGenres.push_back({GetSignificantHumorTypes(genreData.originalGenres), 1});
    else if(genreData.strengthHumor > 0.4f)
        genreData.realGenres.push_back({GetSignificantHumorTypes(genreData.originalGenres), 0.7f});
    else if(genreData.strengthHumor > 0.3f)
        genreData.realGenres.push_back({GetSignificantHumorTypes(genreData.originalGenres), 0.4f});
    else if(genreData.strengthHumor > 0.25f && genreData.strengthDrama < 0.3f )
        genreData.realGenres.push_back({GetSignificantHumorTypes(genreData.originalGenres), 0.2f});
    if(genreData.strengthRomance > 0.7f)
        genreData.realGenres.push_back({{"Romance"}, 1});
    else if(genreData.strengthRomance > 0.5f)
        genreData.realGenres.push_back({{"Romance"}, 0.6f});
    else if(genreData.strengthRomance > 0.25f)
        genreData.realGenres.push_back({{"Romance"}, 0.2f});
    if(genreData.strengthDrama > 0.6f)
        genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 1});
    else if(genreData.strengthDrama > 0.45f)
        genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 0.5f});
    else if(genreData.strengthDrama > 0.25f && genreData.strengthHumor < 0.3f)
        genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 0.2f});

    if(genreData.strengthBonds > 0.3f)
        genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 1});
    else if(genreData.strengthBonds > 0.2f)
        genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 0.7f});
    else if(genreData.strengthBonds > 0.12f)
        genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 0.3f});
    if(genreData.strengthHurtComfort> 0.4f)
        genreData.realGenres.push_back({{"Hurt/Comfort"}, 1});
    else if(genreData.strengthHurtComfort > 0.3f)
        genreData.realGenres.push_back({{"Hurt/Comfort"}, 0.7f});
    else if(genreData.strengthHurtComfort > 0.2f)
        genreData.realGenres.push_back({{"Hurt/Comfort"}, 0.5f});

    genreData.processedGenres = FinalGenreProcessing(genreData);
}

void GenreConverter::ProcessGenreResultIteration2(genre_stats::FicGenreData & genreData)
{
    QVector<genre_stats::GenreBit> result;

    for(auto genre: GetSignificantKeptGenres(genreData.originalGenres))
        genreData.realGenres.push_back({{genre}, 1});

    if(genreData.strengthNeutralAdventure >= 0.8f)
        genreData.realGenres.push_back({GetSignificantNeutralTypes(genreData.originalGenres, true), 1 - (0.8f - genreData.strengthNeutralAdventure)*2.f});
    else if (genreData.strengthNeutralComposite >= 0.8f && genreData.strengthNeutralAdventure >= 0.5f)
        genreData.realGenres.push_back({GetSignificantNeutralTypes(genreData.originalGenres, true), 1 - (0.8f - genreData.strengthNeutralAdventure)*2.f});

    else if(genreData.strengthNeutralComposite >= 0.6f)
        genreData.realGenres.push_back({GetSignificantNeutralTypes(genreData.originalGenres), 1 - (0.8f - genreData.strengthNeutralComposite)*2.f});


    if(genreData.strengthHumor > 0.5f)
        genreData.realGenres.push_back({GetSignificantHumorTypes(genreData.originalGenres), 1});
    else if(genreData.strengthHumor > 0.4f)
        genreData.realGenres.push_back({GetSignificantHumorTypes(genreData.originalGenres), 0.7f});
    else if(genreData.strengthHumor > 0.3f)
        genreData.realGenres.push_back({GetSignificantHumorTypes(genreData.originalGenres), 0.4f});
    else if(genreData.strengthHumor > 0.25f && genreData.strengthDrama < 0.3f )
        genreData.realGenres.push_back({GetSignificantHumorTypes(genreData.originalGenres), 0.2f});
    if(genreData.strengthRomance > 0.7f)
        genreData.realGenres.push_back({{"Romance"}, 1});
    else if(genreData.strengthRomance > 0.5f)
        genreData.realGenres.push_back({{"Romance"}, 0.6f});
    else if(genreData.strengthRomance > 0.25f)
        genreData.realGenres.push_back({{"Romance"}, 0.2f});
    if(genreData.strengthDrama > 0.6f)
        genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 1});
    else if(genreData.strengthDrama > 0.45f)
        genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 0.5f});
    else if(genreData.strengthDrama > 0.25f && genreData.strengthHumor < 0.3f)
        genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 0.2f});

    if(genreData.strengthBonds > 0.5f)
        genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 1});
    else if(genreData.strengthBonds > 0.3f)
        genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 0.7f});
    else if(genreData.strengthBonds > 0.2f)
        genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 0.3f});
    if(genreData.strengthHurtComfort> 0.5f)
        genreData.realGenres.push_back({{"Hurt/Comfort"}, 1});
    else if(genreData.strengthHurtComfort > 0.4f)
        genreData.realGenres.push_back({{"Hurt/Comfort"}, 0.7f});
    else if(genreData.strengthHurtComfort > 0.2f)
        genreData.realGenres.push_back({{"Hurt/Comfort"}, 0.5f});

    genreData.processedGenres = FinalGenreProcessing(genreData);
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

GenreIndex::GenreIndex()
{
    Init();
}

void GenreIndex::Init()
{
    size_t counter = 0;
    InitGenre({true,counter++,"General", "General_", mt_neutral, gc_neutral});
    InitGenre({true,counter++,"Humor", "", mt_happy, gc_funny});
    InitGenre({true,counter++,"Poetry", "", mt_neutral, gc_none});
    InitGenre({true,counter++,"Adventure", "", mt_neutral, gc_neutral});
    InitGenre({true,counter++,"Mystery", "", mt_neutral, gc_neutral});
    InitGenre({true,counter++,"Horror", "", mt_neutral, gc_shocky});
    InitGenre({true,counter++,"Parody", "", mt_happy, gc_funny});
    InitGenre({true,counter++,"Angst", "", mt_sad, gc_dramatic});
    InitGenre({true,counter++,"Supernatural", "", mt_neutral, gc_neutral});
    InitGenre({true,counter++,"Suspense", "", mt_neutral, gc_neutral});
    InitGenre({true,counter++,"Romance", "", mt_neutral, gc_flirty});
    InitGenre({true,counter++,"not found", "NoGenre", mt_neutral, gc_none});
    InitGenre({true,counter++,"Sci-Fi", "SciFi", mt_neutral, gc_neutral});
    InitGenre({true,counter++,"Fantasy", "", mt_neutral, gc_neutral});
    InitGenre({true,counter++,"Spiritual", "", mt_neutral, gc_neutral});
    InitGenre({true,counter++,"Tragedy", "", mt_sad, gc_dramatic});
    InitGenre({true,counter++,"Western", "", mt_neutral, gc_neutral});
    InitGenre({true,counter++,"Crime", "", mt_neutral, gc_neutral});
    InitGenre({true,counter++,"Family", "", mt_neutral, gc_bondy});
    InitGenre({true,counter++,"Hurt/Comfort", "HurtComfort", mt_neutral, gc_hurty});
    InitGenre({true,counter++,"Friendship", "", mt_neutral, gc_bondy});
    InitGenre({true,counter++,"Drama", "", mt_sad, gc_dramatic});
}

void GenreIndex::InitGenre(const Genre &genre)
{
    genresByName[genre.name] = genre;
    genresByIndex[genre.indexInDatabase] = genre;
    genresByCategory[genre.genreCategory].push_back(genre);
    genresByMood[genre.moodType].push_back(genre);
    genresByDbName[genre.nameInDatabase] = genre;
}

Genre &GenreIndex::GenreByName(QString name)
{
    if(genresByName.contains(name))
        return genresByName[name];
    return nullGenre;
}

size_t GenreIndex::IndexByFFNName(QString name) const
{
    if(genresByName.contains(name))
        return genresByName[name].indexInDatabase;
    Q_ASSERT(false);
    return -1;
}

}
