#ifndef SERVITORWINDOW_H
#define SERVITORWINDOW_H

#include <QMainWindow>

namespace Ui {
class servitorWindow;
}

class ServitorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServitorWindow(QWidget *parent = 0);
    ~ServitorWindow();
    void ReadSettings();
    void WriteSettings();

private slots:
    void on_pbLoadFic_clicked();

    void on_pbReprocessFics_clicked();

    void on_pushButton_clicked();

private:
    Ui::servitorWindow *ui;
};

#endif // SERVITORWINDOW_H
