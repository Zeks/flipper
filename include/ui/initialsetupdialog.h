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

#pragma once
#include <QDialog>
#include "include/flipper_client_logic.h"

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
    bool CreateRecommendationsFromProfile();
    bool CreateRecommendationsFromUrls(QVector<int>);

    bool ProcessRecommendationsFromFFNProfile();
    bool ProcessRecommendationsFromListOfUrls();

    QVector<int> PickFicIDsFromString(QString str);


    QSharedPointer<FlipperClientLogic> env;
    bool authorTestSuccessfull = false;
    bool initComplete = false;
    bool readsSlash = false;
    bool recsCreated = false;
private slots:
    void on_pbVerifyUserFFNId_clicked();

    void on_pbSelectDatabaseFile_clicked();

    void on_pbPerformInit_clicked();

    void on_rbHaveFFNList_clicked();

    void on_rbNoFFNList_clicked();

private:
    Ui::InitialSetupDialog *ui;

};


