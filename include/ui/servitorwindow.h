/*
Flipper is a recommendation and search engine for fanfiction.net
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
#ifndef SERVITORWINDOW_H
#define SERVITORWINDOW_H

#include <QMainWindow>
//#include <QtCharts>

//using namespace QtCharts;
#include "environment.h"
#include "include/favholder.h"
#include "third_party/roaring/roaring.hh"
#include "include/calc_data_holder.h"
#include "tasks/author_genre_iteration_processor.h"

namespace Ui {
class servitorWindow;
}
namespace database {
class IDBWrapper;
}
namespace QtCharts{
class QChartView;
class QChart;
class QValueAxis;
}
class FicSourceGRPC;
struct GenreDetectionSources{
    QHash<int, std::array<double, 22>> genreAuthorLists;
    QHash<uint32_t, genre_stats::ListMoodData> moodAuthorLists;
    QHash<int, QString> originalFicGenres;
    QHash<int, QSet<int>> ficsToUse; // set of authors that have it
};
struct CutoffControls{
    float funny = 0.3f;
    float flirty = 0.5f;
    float adventure = 0.3f;
    float drama = 0.3f;
    float bonds = 0.3f;
    float hurty = 0.15f;
};

struct ChartData{
    QSharedPointer<QtCharts::QChartView> chartView;
    QSharedPointer<QtCharts::QChart> chart;
    QSharedPointer<QtCharts::QValueAxis> axisY;
};

struct InputForGenresIteration2{
    typedef core::DataHolderInfo<core::rdt_favourites>::type FavType;
    typedef core::DataHolderInfo<core::rdt_fic_genres_composite>::type FicGenreCompositeType;
    typedef core::DataHolderInfo<core::rdt_fic_genres_original>::type FicGenreOriginalType;

    FavType faves;
    FicGenreCompositeType ficGenresComposite;
    FicGenreCompositeType ficGenresOriginalsInCompositeFormat;
    FicGenreOriginalType ficGenresOriginal;


    FicGenreCompositeType filteredFicGenresComposite;
    FicGenreOriginalType filteredFicGenresOriginal;
};

class ServitorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServitorWindow(QWidget *parent = 0);
    ~ServitorWindow();
    void ReadSettings();
    void WriteSettings();
    void UpdateInterval(int, int);
    void CreateChartView(ChartData&, QWidget *widget, QStringList categories);
    void CreateChartViews();
    void InitGenreChartView(QString);
    void InitMoodChartView(QString);
    void InitGenreCompareChartView(QList<int> users);
    void InitMoodCompareChartView(QList<int> users, bool useOriginalMoods = true);
    void InitMatchingListChartView(genre_stats::ListMoodData data, genre_stats::ListMoodData combinedData);
    void InitGrpcSource();

    void DetectGenres(int minAuthorRecs, int minFoundLists);
    void CreateAdjustedGenresForAuthors();
    void CreateSecondIterationOfGenresForFics(int minAuthorRecs, int minFoundLists);
    QVector<genre_stats::FicGenreData> CreateGenreDataForFics(GenreDetectionSources input,
                                                              CutoffControls cutoff,
                                                              bool userIterationForGenreProcessing = false, bool displayLog = false);
    void LoadDataForCalculation(CalcDataHolder& data);
    void ProcessCDHData(CalcDataHolder& data);
    void CalcConstantMemory();
    void FillFicsForUser(QString);
    QHash<int, int> CreateSummaryMatches();
    QHash<uint32_t, core::FicWeightPtr> ficData;
    QHash<uint32_t, QSet<uint32_t>> ficsForFandoms;
    QHash<uint32_t, Roaring> ficsToFavLists;
    QList<uint32_t> keys;

    QSharedPointer<database::IDBWrapper> dbInterface;
    CoreEnvironment env;
    AuthorGenreIterationProcessor iteratorProcessor;
    QHash<int, std::array<double, 22>> authorGenreDataOriginal;

    std::array<double, 22> listGenreData;
    genre_stats::ListMoodData listMoodData;
    std::array<double, 22> adjustedListGenreData;
    genre_stats::ListMoodData adjustedListMoodData;

    QHash<uint32_t, genre_stats::ListMoodData> originalMoodData;
    QHash<uint32_t, genre_stats::ListMoodData> adjustedMoodData;

    ChartData viewGenre;
    ChartData viewMood;
    ChartData viewListCompare;
    ChartData viewMoodCompare;
    ChartData viewMatchingList;
    QStringList genreList;
    QStringList moodList;
    QHash<int, core::FavouritesMatchResult> matchesForUsers;
    QSharedPointer<FicSourceGRPC> grpcSource;
    InputForGenresIteration2 inputs;
    bool stopDiscordProcessing = false;

private slots:
    void on_pbLoadFic_clicked();

    void on_pbReprocessFics_clicked();

    void on_pushButton_clicked();

    void on_pbGetGenresForFic_clicked();

    void on_pbGetGenresForEverything_clicked();

    void on_pushButton_2_clicked();

    void on_pbGetData_clicked();

    void on_pushButton_3_clicked();

    void on_pbUpdateFreshAuthors_clicked();

    void OnResetTextEditor();
    void OnProgressBarRequested();
    void OnUpdatedProgressValue(int value);
    void OnNewProgressString(QString value);

    void on_pbUnpdateInterval_clicked();

    void on_pbReprocessAllFavPages_clicked();

    void on_pbGetNewFavourites_clicked();

    void on_pbReprocessCacheLinked_clicked();

    void on_pbPCRescue_clicked();

    void on_pbSlashCalc_clicked();

    void on_pbFindSlashSummary_clicked();

    void on_pbCalcWeights_clicked();

    void on_pbCleanPrecalc_clicked();

    void on_pbGenresIteration2_clicked();

    void on_cbGenres_currentIndexChanged(const QString &arg1);

    void on_pbLoadGenreDistributions_clicked();

    void on_cbMoodSelector_currentTextChanged(const QString &arg1);

    void on_pbCalcFicGenres_clicked();

    void on_pbCompareGenres_clicked();

    void on_cbMoodSource_currentIndexChanged(const QString &arg1);

    void on_cbUserIDs_currentIndexChanged(const QString &arg1);

    void on_pbExtractDiscords_clicked();

    void on_pbStopDiscordProcessing_clicked();

    void on_pbTestDiscord_clicked();

private:
    Ui::servitorWindow *ui;
};

#endif // SERVITORWINDOW_H
