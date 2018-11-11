#ifndef SERVITORWINDOW_H
#define SERVITORWINDOW_H

#include <QMainWindow>
#include "environment.h"
#include "third_party/roaring/roaring.hh"
#include "include/calc_data_holder.h"

namespace Ui {
class servitorWindow;
}
namespace database {
class IDBWrapper;
}

class ServitorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServitorWindow(QWidget *parent = 0);
    ~ServitorWindow();
    void ReadSettings();
    void WriteSettings();
    void UpdateInterval(int, int);

    void DetectGenres(int minAuthorRecs, int minFoundLists);
    void LoadDataForCalculation(CalcDataHolder& data);
    void ProcessCDHData(CalcDataHolder& data);
    void CalcConstantMemory();
    QHash<uint32_t, core::FicWeightPtr> ficData;
    QHash<uint32_t, QSet<uint32_t>> ficsForFandoms;
    QHash<uint32_t, Roaring> ficsToFavLists;
    QList<uint32_t> keys;

    QSharedPointer<database::IDBWrapper> dbInterface;
    CoreEnvironment env;

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

private:
    Ui::servitorWindow *ui;
};

#endif // SERVITORWINDOW_H
