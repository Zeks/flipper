#ifndef INITIALSETUPDIALOG_H
#define INITIALSETUPDIALOG_H

#include <QDialog>
#include "include/environment.h"

namespace Ui {
class InitialSetupDialog;
}

class InitialSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InitialSetupDialog(QDialog *parent = nullptr);
    ~InitialSetupDialog();
    void VerifyUserID();
    bool CreateRecommendations();
    QSharedPointer<CoreEnvironment> env;
    bool authorTestSuccessfull = false;
    bool initComplete = false;
private slots:
    void on_pbVerifyUserFFNId_clicked();

    void on_pbSelectDatabaseFile_clicked();

    void on_pbPerformInit_clicked();

private:
    Ui::InitialSetupDialog *ui;

};

#endif // INITIALSETUPDIALOG_H
