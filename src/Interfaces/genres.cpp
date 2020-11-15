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
#include "Interfaces/genres.h"
#include "pure_sql.h"


namespace interfaces {


QHash<QString, QString> CreateCodeToDBGenreConverter()
{
    QHash<QString, QString> result;
    result[QStringLiteral("General")] = QStringLiteral("General_");
    result[QStringLiteral("Humor")] = QStringLiteral("Humor");
    result[QStringLiteral("Poetry")] = QStringLiteral("Poetry");
    result[QStringLiteral("Adventure")] = QStringLiteral("Adventure");
    result[QStringLiteral("Mystery")] = QStringLiteral("Mystery");
    result[QStringLiteral("Horror")] = QStringLiteral("Horror");
    result[QStringLiteral("Parody")] = QStringLiteral("Parody");
    result[QStringLiteral("Angst")] = QStringLiteral("Angst");
    result[QStringLiteral("Supernatural")] = QStringLiteral("Supernatural");
    result[QStringLiteral("Suspense")] = QStringLiteral("Suspense");
    result[QStringLiteral("Romance")] = QStringLiteral("Romance");
    result[QStringLiteral("not found")] = QStringLiteral("NoGenre");
    result[QStringLiteral("Sci-Fi")] = QStringLiteral("SciFi");
    result[QStringLiteral("Fantasy")] = QStringLiteral("Fantasy");
    result[QStringLiteral("Spiritual")] = QStringLiteral("Spiritual");
    result[QStringLiteral("Tragedy")] = QStringLiteral("Tragedy");
    result[QStringLiteral("Drama")] = QStringLiteral("Drama");
    result[QStringLiteral("Western")] = QStringLiteral("Western");
    result[QStringLiteral("Crime")] = QStringLiteral("Crime");
    result[QStringLiteral("Family")] = QStringLiteral("Family");
    result[QStringLiteral("Hurt/Comfort")] = QStringLiteral("HurtComfort");
    result[QStringLiteral("Friendship")] = QStringLiteral("Friendship");
    return result;
}

QHash<QString, QString> CreateDBToCodeGenreConverter()
{
    QHash<QString, QString> result;
    result[QStringLiteral("HurtComfort")] = QStringLiteral("Hurt/Comfort");
    result[QStringLiteral("General_")] = QStringLiteral("General");
    result[QStringLiteral("NoGenre")] = QStringLiteral("not found");
    result[QStringLiteral("SciFi")] = QStringLiteral("Sci-Fi");
    result[QStringLiteral("Humor")] = QStringLiteral("Humor");
    result[QStringLiteral("Poetry")] = QStringLiteral("Poetry");
    result[QStringLiteral("Adventure")] = QStringLiteral("Adventure");
    result[QStringLiteral("Mystery")] = QStringLiteral("Mystery");
    result[QStringLiteral("Horror")] = QStringLiteral("Horror");
    result[QStringLiteral("Parody")] = QStringLiteral("Parody");
    result[QStringLiteral("Angst")] = QStringLiteral("Angst");
    result[QStringLiteral("Supernatural")] = QStringLiteral("Supernatural");
    result[QStringLiteral("Suspense")] = QStringLiteral("Suspense");
    result[QStringLiteral("Romance")] = QStringLiteral("Romance");
    result[QStringLiteral("Fantasy")] = QStringLiteral("Fantasy");
    result[QStringLiteral("Spiritual")] = QStringLiteral("Spiritual");
    result[QStringLiteral("Tragedy")] = QStringLiteral("Tragedy");
    result[QStringLiteral("Drama")] = QStringLiteral("Drama");
    result[QStringLiteral("Western")] = QStringLiteral("Western");
    result[QStringLiteral("Crime")] = QStringLiteral("Crime");
    result[QStringLiteral("Family")] = QStringLiteral("Family");
    result[QStringLiteral("Friendship")] = QStringLiteral("Friendship");
    return result;
}


Genres::Genres()
{
    index.Init();
}

bool interfaces::Genres::IsGenreList(QStringList list)
{
    bool success = false;
    for(const auto& token : list)
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


QHash<int, QList<genre_stats::GenreBit> > Genres::GetFullGenreList(bool useOriginalgenres)
{
    if(loadOriginalGenresOnly)
        return database::puresql::GetFullGenreList(db, true).data;
    else
        return database::puresql::GetFullGenreList(db, useOriginalgenres).data;
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
        auto value = std::abs(data.at(i)) > 0.005 ? data.at(i) : 0;
        QLOG_INFO() << genre.name << ": " << value;
    }
    QLOG_INFO() << "///////////////////";

}

QString Genres::MoodForGenre(QString genre)
{
    //QLOG_INFO() << "Reading mood for genre: " << genre;
    QString result;
    thread_local QStringList neutral = {"Adventure" , "Mystery" , "Supernatural" ,  "Suspense" , "Sci-Fi"
                                        , "Fantasy" , "Spiritual" , "Western" , "Crime"};
    thread_local QStringList flirty = {"Romance"};
    thread_local QStringList funny = {"Humor" , "Parody"};
    thread_local QStringList hurty = {"Hurt/Comfort"};
    thread_local QStringList bondy = {"Family" , "Friendship"};
    thread_local QStringList dramatic = { "Drama" , "Tragedy" , "Angst"};
    thread_local QStringList shocky = {"Horror"};
    if(neutral.contains(genre))
        result = QStringLiteral("Neutral");
    if(flirty.contains(genre))
        result = QStringLiteral("Flirty");
    if(funny.contains(genre))
        result = QStringLiteral("Funny");
    if(hurty.contains(genre))
        result = QStringLiteral("Hurty");
    if(bondy.contains(genre))
        result = QStringLiteral("Bondy");
    if(dramatic.contains(genre))
        result = QStringLiteral("Dramatic");
    if(shocky.contains(genre))
        result = QStringLiteral("Shocky");
    //QLOG_INFO() << "returning:" << result;
    return result;
}

void Genres::WriteMoodValue(QString mood, float value, genre_stats::ListMoodData & data)
{
    if(mood == QStringLiteral("Neutral"))
        data.strengthNeutral+=value;
    if(mood == QStringLiteral("Funny"))
        data.strengthFunny+=value;
    if(mood == QStringLiteral("Hurty"))
        data.strengthHurty+=value;
    if(mood == QStringLiteral("Bondy"))
        data.strengthBondy+=value;
    if(mood == QStringLiteral("Dramatic"))
        data.strengthDramatic+=value;
    if(mood == QStringLiteral("Shocky"))
        data.strengthShocky+=value;
    if(mood == QStringLiteral("Flirty"))
        data.strengthFlirty+=value;
}

float Genres::ReadMoodValue(QString mood, const genre_stats::ListMoodData & data)
{
    if(mood ==QStringLiteral("Neutral"))
        return data.strengthNeutral;
    if(mood ==QStringLiteral("Funny"))
        return data.strengthFunny;
    if(mood ==QStringLiteral("Hurty"))
        return data.strengthHurty;
    if(mood ==QStringLiteral("Bondy"))
        return data.strengthBondy;
    if(mood ==QStringLiteral("Dramatic"))
        return data.strengthDramatic;
    if(mood ==QStringLiteral("Shocky"))
        return data.strengthShocky;
    if(mood ==QStringLiteral("Flirty"))
        return data.strengthFlirty;
    return 0.f;
}


static QStringList GetSignificantNeutralTypes(QStringList list, bool addAdventure = false)
{
    QStringList result;
    if(list.contains(QStringLiteral("Sci-Fi")))
        result.push_back(QStringLiteral("Sci-Fi"));
    if(list.contains(QStringLiteral("Spiritual")))
        result.push_back(QStringLiteral("Spiritual"));
    if(list.contains(QStringLiteral("Supernatural")))
        result.push_back(QStringLiteral("Supernatural"));
    if(list.contains(QStringLiteral("Suspense")))
        result.push_back(QStringLiteral("Suspense"));
    if(list.contains(QStringLiteral("Mystery")))
        result.push_back(QStringLiteral("Mystery"));
    if(list.contains(QStringLiteral("Crime")))
        result.push_back(QStringLiteral("Crime"));
    if(list.contains(QStringLiteral("Fantasy")))
        result.push_back(QStringLiteral("Fantasy"));
    if(list.contains(QStringLiteral("Western")))
        result.push_back(QStringLiteral("Western"));
    if((addAdventure && result.empty()) || list.contains(QStringLiteral("Adventure")))
        result.push_back(QStringLiteral("Adventure"));
    return result;
}


static QStringList GetSignificantHumorTypes(QStringList list)
{
    QStringList result;
    if(list.contains(QStringLiteral("Parody")))
        result.push_back(QStringLiteral("Parody"));
    if(result.empty() || list.contains(QStringLiteral("Humor")))
        result.push_back(QStringLiteral("Humor"));
    return result;
}


static QStringList GetSignificantDramaTypes(QStringList list)
{
    QStringList result;
    if(list.contains(QStringLiteral("Angst")))
        result.push_back(QStringLiteral("Angst"));
    if(list.contains(QStringLiteral("Tragedy")))
        result.push_back(QStringLiteral("Tragedy"));
    if(result.empty() || list.contains(QStringLiteral("Drama")))
        result.push_back(QStringLiteral("Drama"));
    return  result;
}

static QStringList GetSignificantFamilyTypes(QStringList list)
{
    QStringList result;
    if(list.contains(QStringLiteral("Friendship")))
        result.push_back(QStringLiteral("Friendship"));
    if(result.empty() || list.contains(QStringLiteral("Family")))
        result.push_back(QStringLiteral("Family"));
    return  result;
}

static QStringList GetSignificantKeptGenres(QStringList list)
{
    QStringList result;
    QStringList keep = {"Horror", "General", "Poetry"};
    for(const auto& genre : list)
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
    for(const auto& kept: std::as_const(data.genresToKeep))
        result.push_back({{kept}, 0});
    return result;
}

void GenreConverter::ProcessGenreResult(genre_stats::FicGenreData & genreData)
{
    //QVector<genre_stats::GenreBit> result;

    const auto significantKeptGenres = GetSignificantKeptGenres(genreData.originalGenres);
    for(const auto& genre: significantKeptGenres)
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
        genreData.realGenres.push_back({{QStringLiteral("Romance")}, 1});
    else if(genreData.strengthRomance > 0.5f)
        genreData.realGenres.push_back({{QStringLiteral("Romance")}, 0.6f});
    else if(genreData.strengthRomance > 0.25f)
        genreData.realGenres.push_back({{QStringLiteral("Romance")}, 0.2f});
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
        genreData.realGenres.push_back({{QStringLiteral("Hurt/Comfort")}, 1});
    else if(genreData.strengthHurtComfort > 0.3f)
        genreData.realGenres.push_back({{QStringLiteral("Hurt/Comfort")}, 0.7f});
    else if(genreData.strengthHurtComfort > 0.2f)
        genreData.realGenres.push_back({{QStringLiteral("Hurt/Comfort")}, 0.5f});

    genreData.processedGenres = FinalGenreProcessing(genreData);
}

void GenreConverter::ProcessGenreResultIteration2(genre_stats::FicGenreData & genreData)
{
    //QVector<genre_stats::GenreBit> result;
    const auto significantKeptGenres = GetSignificantKeptGenres(genreData.originalGenres);
    for(const auto& genre: significantKeptGenres)
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
        genreData.realGenres.push_back({{QStringLiteral("Romance")}, 1});
    else if(genreData.strengthRomance > 0.5f)
        genreData.realGenres.push_back({{QStringLiteral("Romance")}, 0.6f});
    else if(genreData.strengthRomance > 0.25f)
        genreData.realGenres.push_back({{QStringLiteral("Romance")}, 0.2f});

    if(genreData.strengthDrama > 0.6f)
        genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 1});
    else if(genreData.strengthDrama > 0.45f)
        genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 0.5f});
    else if(genreData.strengthDrama > 0.25f && genreData.strengthHumor < 0.3f)
        genreData.realGenres.push_back({GetSignificantDramaTypes(genreData.originalGenres), 0.2f});

    if(genreData.strengthBonds > 0.7f)
        genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 1});
    else if(genreData.strengthBonds > 0.5f)
        genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 0.7f});
    else if(genreData.strengthBonds > 0.35f)
        genreData.realGenres.push_back({GetSignificantFamilyTypes(genreData.originalGenres), 0.3f});


    if(genreData.strengthHurtComfort> 0.7f)
        genreData.realGenres.push_back({{QStringLiteral("Hurt/Comfort")}, 1});
    else if(genreData.strengthHurtComfort > 0.5f)
        genreData.realGenres.push_back({{QStringLiteral("Hurt/Comfort")}, 0.7f});
    else if(genreData.strengthHurtComfort > 0.4f)
        genreData.realGenres.push_back({{QStringLiteral("Hurt/Comfort")}, 0.5f});

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
    data.genresToKeep.push_back(QStringLiteral("Horror"));

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
        data.realGenres.push_back({{QStringLiteral("Romance")}, data.strengthRomance*2});

    bool dramaIsMainGenre = true;
    for(auto genreData : {data.strengthRomance,data.strengthHumor, data.strengthHurtComfort})
        if( (data.strengthDrama - genreData) < 0.15f )
            dramaIsMainGenre = false;

    if(dramaIsMainGenre)
        data.realGenres.push_back({GetSignificantDramaTypes(data.originalGenres), data.strengthDrama*2});


    //QString adventureType;

    if(data.strengthNeutralComposite >= 0.9f)
        data.realGenres.push_back({GetSignificantNeutralTypes(data.originalGenres), data.strengthNeutralComposite});

    if(hurtComfortStatisticallySignificant && (data.strengthHumor < 0.3f))
    {
        data.realGenres.push_back({{QStringLiteral("Hurt/Comfort")}, data.strengthHurtComfort*3});
        data.genresToKeep.push_back(QStringLiteral("Hurt/Comfort"));
    }

    if(data.strengthBonds > 0.15f)
    {
        data.realGenres.push_back({GetSignificantFamilyTypes(data.originalGenres), data.strengthBonds*3});
        data.genresToKeep.push_back(QStringLiteral("Family"));
        data.genresToKeep.push_back(QStringLiteral("Friendship"));
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
            data.realGenres.push_back({{QStringLiteral("Romance")}, data.strengthRomance*2});
            data.realGenres.push_back({GetSignificantDramaTypes(data.originalGenres), data.strengthDrama*2*dramaticMultiplier});
        }
        if(humorIsMainGenre)
        {
            if((data.strengthDrama - data.strengthRomance) > 0.15f)
                data.realGenres.push_back({GetSignificantDramaTypes(data.originalGenres), data.strengthDrama*2});
            else if((data.strengthRomance - data.strengthDrama) > 0.15f)
                data.realGenres.push_back({{QStringLiteral("Romance")}, data.strengthRomance*2});
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
    InitGenre({true,counter++,QStringLiteral("General"), QStringLiteral("General_"), mt_neutral, gc_neutral});
    InitGenre({true,counter++,QStringLiteral("Humor"), QStringLiteral(""), mt_happy, gc_funny});
    InitGenre({true,counter++,QStringLiteral("Poetry"), QStringLiteral(""), mt_neutral, gc_none});
    InitGenre({true,counter++,QStringLiteral("Adventure"), QStringLiteral(""), mt_neutral, gc_neutral});
    InitGenre({true,counter++,QStringLiteral("Mystery"), QStringLiteral(""), mt_neutral, gc_neutral});
    InitGenre({true,counter++,QStringLiteral("Horror"), QStringLiteral(""), mt_neutral, gc_shocky});
    InitGenre({true,counter++,QStringLiteral("Parody"), QStringLiteral(""), mt_happy, gc_funny});
    InitGenre({true,counter++,QStringLiteral("Angst"), QStringLiteral(""), mt_sad, gc_dramatic});
    InitGenre({true,counter++,QStringLiteral("Supernatural"), QStringLiteral(""), mt_neutral, gc_neutral});
    InitGenre({true,counter++,QStringLiteral("Suspense"), QStringLiteral(""), mt_neutral, gc_neutral});
    InitGenre({true,counter++,QStringLiteral("Romance"), QStringLiteral(""), mt_neutral, gc_flirty});
    InitGenre({true,counter++,QStringLiteral("not found"), QStringLiteral("NoGenre"), mt_neutral, gc_none});
    InitGenre({true,counter++,QStringLiteral("Sci-Fi"), QStringLiteral("SciFi"), mt_neutral, gc_neutral});
    InitGenre({true,counter++,QStringLiteral("Fantasy"), QStringLiteral(""), mt_neutral, gc_neutral});
    InitGenre({true,counter++,QStringLiteral("Spiritual"), QStringLiteral(""), mt_neutral, gc_neutral});
    InitGenre({true,counter++,QStringLiteral("Tragedy"), QStringLiteral(""), mt_sad, gc_dramatic});
    InitGenre({true,counter++,QStringLiteral("Western"), QStringLiteral(""), mt_neutral, gc_neutral});
    InitGenre({true,counter++,QStringLiteral("Crime"), QStringLiteral(""), mt_neutral, gc_neutral});
    InitGenre({true,counter++,QStringLiteral("Family"), QStringLiteral(""), mt_neutral, gc_bondy});
    InitGenre({true,counter++,QStringLiteral("Hurt/Comfort"), QStringLiteral("HurtComfort"), mt_neutral, gc_hurty});
    InitGenre({true,counter++,QStringLiteral("Friendship"), QStringLiteral(""), mt_neutral, gc_bondy});
    InitGenre({true,counter++,QStringLiteral("Drama"), QStringLiteral(""), mt_sad, gc_dramatic});
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
