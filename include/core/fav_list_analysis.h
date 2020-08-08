#pragma once
#include <QHash>
#include <QString>
#include <array>
#include <optional>
#include "core/section.h"
namespace core{

enum class ExplorerRanges{
    none = 0,
    barely_known= 1,
    relatively_unknown = 2,
    popular = 3
};
core::ExplorerRanges ProcesFavouritesIntoPopularityCategory(int);
core::EntitySizeType ProcesWordcountIntoSizeCategory(int);

struct FicDataAccumulatorResult{
    std::array<double, 4> moodRatios{};
    std::array<double, 5> sizeRatios{};
    std::array<double, 22> genreRatios{};
    QHash<int, double> fandomRatios;

    double averageWordsPerFic = 0;
    double averageWordsPerChapter = 0;

    double explorerRatio = 0;
    double megaExplorerRatio = 0;
    double slashRatio = 0;
    double crackRatio = 0; // unused
    double smutRatio = 0; // unused
    double crossoverRatio = 0;
    double unfinishedRatio = 0;
    double matureRatio = 0;
    double moodUniformityRatio = 0;
    double fandomDiversityRatio = 0;
    double genreDiversityRatio = 0;

    EntitySizeType mostFavouritedSize;
    EntitySizeType sectionRelativeSize;

    FavListDetails::MoodType prevalentMood;


};

struct FicListDataAccumulator{
    FicListDataAccumulator();
    std::array<short, 4> moodCounters{};
    std::array<short, 5> sizeCounters{};
    std::array<short, 22> genreCounters{};
    std::array<short, 4> popularityCounters{};
    QHash<int, int> fandomCounters;
    QDate firstPublished, lastPublished;
    int slashCounter = 0;
    int crackCounter = 0; // unused
    int smutCounter = 0; // unused
    int favourites = 0;
    int ficCount = 0;
    int crossoverCounter = 0;
    int unfinishedCounter = 0;
    int wordcount= 0;
    int explorerCounter = 0;
    int megaExplorerCounter = 0;
    int chapterCounter = 0;
    int matureCounter = 0;

    void AddPublishDate(QDate date);
    // favs, explorer, megaexplorer, fav size category
    void AddFavourites(int favCount);
    void AddFandoms(const QList<int>& fandoms);
    // wordcount average fic wordcount, size factors
    void AddWordcount(int wordcount, int chapters);
    void ProcessIntoResult();
    void ProcessFicSize();
    void ProcessGenres();
    void ProcessFandoms();

    core::FicDataAccumulatorResult result;

};
}
