#ifndef SERVITORWINDOW_H
#define SERVITORWINDOW_H

#include <QMainWindow>

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
    QSharedPointer<database::IDBWrapper> dbInterface;

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

private:
    Ui::servitorWindow *ui;
};

#endif // SERVITORWINDOW_H
