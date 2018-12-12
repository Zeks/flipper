#include "include/core/fav_list_analysis.h"
#include "include/Interfaces/genres.h"
template <typename T1, typename T2>
inline double DivideAsDoubles(T1 arg1, T2 arg2){
    return static_cast<double>(arg1)/static_cast<double>(arg2);
}
namespace core{
core::EntitySizeType ProcesWordcountIntoSizeCategory(int wordCount)
{
    core::EntitySizeType result{core::EntitySizeType::none};

    if(wordCount <= 20000)
        result = core::EntitySizeType::small;
    else if(wordCount <= 100000)
        result = core::EntitySizeType::medium;
    else if(wordCount <= 400000)
        result = core::EntitySizeType::big;
    else
        result = core::EntitySizeType::huge;

    return result;
}

core::ExplorerRanges ProcesFavouritesIntoPopularityCategory(int favourites)
{
    core::ExplorerRanges result{core::ExplorerRanges::none};

    if(favourites <= 50)
        result = core::ExplorerRanges::barely_known;
    if(favourites <= 150)
        result = core::ExplorerRanges::relatively_unknown;
    if(favourites > 150)
        result = core::ExplorerRanges::popular;
    return result;
}


FicListDataAccumulator::FicListDataAccumulator()
{
//    sizeCounters[0] = 0;
//    sizeCounters[1] = 0;
//    sizeCounters[2] = 0;
//    sizeCounters[3] = 0;
//    sizeCounters[4] = 0;

}

void FicListDataAccumulator::AddPublishDate(QDate date){
    if(!firstPublished.isValid() || firstPublished > date)
        firstPublished = date;
    if(!lastPublished.isValid() || lastPublished < date)
        lastPublished = date;
}
void FicListDataAccumulator::AddFavourites(int favCount) // favs, explorer, megaexplorer, fav size category
{
    ficCount++;
    favourites+=favCount;
    auto type = core::ProcesFavouritesIntoPopularityCategory(favCount);
    popularityCounters[static_cast<size_t>(type)]++;
}
void FicListDataAccumulator::AddFandoms(const QList<int>& fandoms){
    if(fandoms.size() > 1 && !fandoms.contains(-1))
        crossoverCounter++;
    for(auto ficFandom : fandoms)
    {
        if(ficFandom != -1)
            fandomCounters[ficFandom]++;
    }
}
// wordcount average fic wordcount, size factors
void FicListDataAccumulator::AddWordcount(int wordcount, int chapters){
    this->wordcount+=wordcount;
    this->chapterCounter+=chapters;
    auto category = static_cast<size_t>(core::ProcesWordcountIntoSizeCategory(wordcount));
//    if((wordcount > 20000 && wordcount < 100000) ||
//            wordcount > 400000 )
//    qDebug() << "Sending wordcount " << wordcount << " to category: " << category;
    sizeCounters[category]++;
}

void FicListDataAccumulator::ProcessIntoResult()
{
    ProcessFicSize();
    ProcessGenres();
    ProcessFandoms();
    result.megaExplorerRatio = DivideAsDoubles(popularityCounters[1], ficCount);
    result.explorerRatio = DivideAsDoubles(popularityCounters[2], ficCount);
    result.matureRatio = DivideAsDoubles(matureCounter, ficCount);
    result.slashRatio = DivideAsDoubles(slashCounter, ficCount);
    result.unfinishedRatio = DivideAsDoubles(unfinishedCounter, ficCount);
}

void FicListDataAccumulator::ProcessFicSize()
{
    short dominatingValue = 0;
    for(size_t i = 1; i < sizeCounters.size(); i++)
    {
        result.sizeRatios[i] = DivideAsDoubles(sizeCounters[i],ficCount);
        if(sizeCounters[i] > dominatingValue)
            dominatingValue = static_cast<short>(i);
    }
    result.mostFavouritedSize = static_cast<EntitySizeType>(dominatingValue);
    result.averageWordsPerFic = DivideAsDoubles(wordcount,ficCount);
    result.averageWordsPerChapter = DivideAsDoubles(wordcount,chapterCounter);
}

void FicListDataAccumulator::ProcessGenres()
{
    QHash<int, int> moodKeeper;
    An<interfaces::GenreIndex> genreIndex;
    //int moodSum = 0;
    int totalInClumps = 0;
    int total = 0;
    for(size_t i = 0; i < genreCounters.size(); i++)
    {
        const auto& genre = genreIndex->genresByIndex[i];
        result.genreRatios[genre.indexInDatabase] = DivideAsDoubles(genreCounters[genre.indexInDatabase], ficCount);
        total+=genreCounters[genre.indexInDatabase];
        if(DivideAsDoubles(genreCounters[genre.indexInDatabase], ficCount) > 0.4)
            totalInClumps+=genreCounters[genre.indexInDatabase];
        auto type = static_cast<size_t>(genre.moodType);
        moodCounters[type]+= genreCounters[i];
    }
    int totalMoodValue = 0;
    for(size_t i = 0; i < 4; i++)
        totalMoodValue += moodCounters[i];
    for(size_t i = 0; i < 4; i++)
        result.moodRatios[i] += DivideAsDoubles(moodCounters[i], totalMoodValue);

    int totalInRest = total - totalInClumps;
    result.genreDiversityRatio= DivideAsDoubles(totalInRest, total);
    auto minMood = std::min(result.moodRatios[1], std::min(result.moodRatios[2], result.moodRatios[3]));
    auto maxMood = std::max(result.moodRatios[1], std::max(result.moodRatios[2], result.moodRatios[3]));
    result.moodUniformityRatio = DivideAsDoubles(minMood,maxMood);
    if(result.moodRatios[1] > result.moodRatios[2] && result.moodRatios[1] > result.moodRatios[3])
        result.prevalentMood = FicSectionStats::MoodType::sad;
    if(result.moodRatios[2] > result.moodRatios[1] && result.moodRatios[2] > result.moodRatios[3])
        result.prevalentMood = FicSectionStats::MoodType::neutral;
    if(result.moodRatios[3] > result.moodRatios[2] && result.moodRatios[3] > result.moodRatios[1])
        result.prevalentMood = FicSectionStats::MoodType::positive;
}

void FicListDataAccumulator::ProcessFandoms()
{
    double averageFicsPerFandom = DivideAsDoubles(ficCount,fandomCounters.keys().size()); //static_cast<double>(ficTotal)/static_cast<double>(fandomKeeper.keys().size());

    int totalInClumps = 0;
    int total = 0;
    for(auto fandom : fandomCounters.keys())
    {
        auto currentFandomCounter = fandomCounters[fandom];
        total+=fandomCounters[fandom];
        if(currentFandomCounter >= 2*averageFicsPerFandom)
            totalInClumps+=fandomCounters[fandom];

        result.fandomRatios[fandom]=DivideAsDoubles(fandomCounters[fandom], ficCount);
    }
    int totalInRest = total - totalInClumps;
    //diverse favourite list  will have this close to 1
    result.fandomDiversityRatio = DivideAsDoubles(totalInRest, ficCount);
    result.crossoverRatio = DivideAsDoubles(crossoverCounter, ficCount);
}

}
