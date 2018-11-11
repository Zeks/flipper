#include "calc_data_holder.h"
#include "threaded_data/common_traits.h"
#include "threaded_data/threaded_save.h"
#include "threaded_data/threaded_load.h"

#include <QThread>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
//#include <experimental/type_traits>

void CalcDataHolder::Clear(){
    fileCounter = -1;
    fics.clear();
    authors.clear();
    favourites.clear();
    genreData.clear();
    fandomLists.clear();
}
void CalcDataHolder::ResetFileCounter()
{
    fileCounter = -1;
}
void CalcDataHolder::SaveToFile(){}
void CalcDataHolder::LoadFromFile(){}


void CalcDataHolder::SaveFicsData(){
    int threadCount = QThread::idealThreadCount()-1;
    thread_boost::SaveFicWeightCalcData("TempData", fics);
    thread_boost::SaveAuthorsData("TempData", authors);
    thread_boost::SaveFavouritesData("TempData", favourites);
    thread_boost::SaveGenreDataForFavLists("TempData", genreData);
    thread_boost::SaveFandomDataForFavLists("TempData", fandomLists);
}

void CalcDataHolder::LoadFicsData(){
    Clear();

    thread_boost::LoadFicWeightCalcData("TempData", fics);
    thread_boost::LoadAuthorsData("TempData", authors);
    thread_boost::LoadFavouritesData("TempData", favourites);
    thread_boost::LoadGenreDataForFavLists("TempData", genreData);
    thread_boost::LoadFandomDataForFavLists("TempData", fandomLists);
}
