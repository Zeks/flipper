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

core::ExplorerRanges core::ProcesFavouritesIntoPopularityCategory(int favourites)
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


void FicListDataAccumulator::AddPublishDate(QDate date){
    if(!firstPublished.isValid() || firstPublished < date)
        firstPublished = date;
    if(!lastPublished.isValid() || lastPublished > date)
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
    if(fandoms.size() > 1)
        crossoverCounter++;
    for(auto ficFandom : fandoms)
        fandomCounters[ficFandom]++;
}
// wordcount average fic wordcount, size factors
void FicListDataAccumulator::AddWordcount(int wordcount, int chapters){
    this->wordcount+=wordcount;
    this->chapterCounter+=chapters;
    sizeCounters[static_cast<size_t>(core::ProcesWordcountIntoSizeCategory(wordcount))]++;
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
    for(size_t i = 0; i < sizeCounters.size(); i++)
    {
        result.sizeRatios[i] = DivideAsDoubles(sizeCounters[i],ficCount);
        if(sizeCounters[i] > dominatingValue)
            dominatingValue = static_cast<short>(i);
    }
    result.mostFavouritedSize = static_cast<EntitySizeType>(1 + dominatingValue);
    result.averageWordsPerFic = DivideAsDoubles(wordcount,ficCount);
    result.averageWordsPerChapter = DivideAsDoubles(wordcount,chapterCounter);
}

void FicListDataAccumulator::ProcessGenres()
{
    QHash<int, int> moodKeeper;
    An<interfaces::GenreIndex> genreIndex;
    //int moodSum = 0;
    int totalInClumps = 0;
    for(size_t i = 0; i < genreCounters.size(); i++)
    {
        const auto& genre = genreIndex->genresByIndex[i];
        result.genreRatios[genre.indexInDatabase] = DivideAsDoubles(genreCounters[genre.indexInDatabase], ficCount);
        if(DivideAsDoubles(genreCounters[genre.indexInDatabase], ficCount) > 0.4)
            totalInClumps+=genreCounters[genre.indexInDatabase];
        auto type = static_cast<size_t>(genre.moodType);
        moodCounters[type]+= genreCounters[i];
    }
    for(size_t i = 0; i < 4; i++)
        result.moodRatios[i] += DivideAsDoubles(moodCounters[i], ficCount);

    int totalInRest = ficCount - totalInClumps;
    result.genreDiversityRatio= DivideAsDoubles(totalInRest, ficCount);
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
    int totalValue = 0;
    for(auto fandom : fandomCounters.keys())
    {
        if(fandomCounters[fandom] >= 2*averageFicsPerFandom)
            totalInClumps+=fandomCounters[fandom];

        result.fandomRatios[fandom]=DivideAsDoubles(fandomCounters[fandom], ficCount);
    }
    int totalInRest = totalValue - totalInClumps;
    //diverse favourite list  will have this close to 1
    result.fandomDiversityRatio = DivideAsDoubles(totalInRest, ficCount);
}

}
