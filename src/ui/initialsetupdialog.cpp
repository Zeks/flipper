/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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

#include "include/ui/initialsetupdialog.h"
#include "ui_initialsetupdialog.h"
#include "Interfaces/recommendation_lists.h"
#include "include/ImmediateTooltipStyle.h"
#include <QRegularExpression>
#include <QCoreApplication>
#include <QFileDialog>
#include <QDir>
#include <QSettings>
#include <QMessageBox>
#include <QTextCodec>
#include <QProxyStyle>
#include <QStandardPaths>



InitialSetupDialog::InitialSetupDialog(QDialog *parent) :
    QDialog(parent),
    ui(new Ui::InitialSetupDialog)
{
    ui->setupUi(this);

    ui->leDBFileLocation->setText( QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QString info = "Since it's the first time Flipper has been launched, let's do some initial setup\n"
                   "If you have used Flipper previously, don't forget to point it to your database folder.";
    ui->lblInfo->setText(info);
    ui->lblStatus->setVisible(false);
    ui->lblStatus->setWordWrap(true);
    ui->lblFileInfo->setStyle(new ImmediateTooltipProxyStyle());
    ui->lblListInfo->setStyle(new ImmediateTooltipProxyStyle());
    ui->lblSlashInfo->setStyle(new ImmediateTooltipProxyStyle());

    ui->leUserFFNId->setPlaceholderText(QString("Favourites from this profile will be used to create recommendations. If you don't have a profile on FFN, leave it empty"));
    ui->leDBFileLocation->setPlaceholderText(QString("Flipper keeps recommendations and tags there"));
    ui->wdgNoFaves->hide();
    QCoreApplication::processEvents();
    adjustSize();
}

InitialSetupDialog::~InitialSetupDialog()
{
    delete ui;
}

void InitialSetupDialog::VerifyUserID()
{
    auto userID = ui->leUserFFNId->text();
    ui->lblStatus->setText("<font color=\"darkBlue\">Status: Verifying user ID.</font>");
    ui->lblStatus->setVisible(true);

    QCoreApplication::processEvents();
    authorTestSuccessfull =  env->TestAuthorID(ui->leUserFFNId, ui->lblStatus);

}

bool InitialSetupDialog::CreateRecommendationsFromProfile()
{
    ui->lblStatus->setText("<font color=\"darkBlue\">Status: Creating recommendation list, this may take a while.</font>");
    ui->lblStatus->setVisible(true);
    QCoreApplication::processEvents();

    QString url = "https://www.fanfiction.net/u/" + ui->leUserFFNId->text();
    auto sourceFicsSet = env->LoadAuthorFicIdsForRecCreation(url, ui->lblStatus);

    QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
    params->minimumMatch = 1;
    params->maxUnmatchedPerMatch = 50;
    params->alwaysPickAt = 9999;
    params->isAutomatic = true;
    params->useWeighting = true;
    params->useMoodAdjustment = true;
    params->name = "Recommendations";
    params->assignLikedToSources = true;
    params->userFFNId = env->interfaces.recs->GetUserProfile();
    QVector<int> sourceFics;
    for(auto fic : sourceFicsSet)
        sourceFics.push_back(fic.toInt());

    auto result = env->BuildRecommendations(params, sourceFics);
    return result;
}

bool InitialSetupDialog::CreateRecommendationsFromUrls(QVector<int> ids)
{
    ui->lblStatus->setText("<font color=\"darkBlue\">Status: Creating recommendation list, this may take a while.</font>");
    ui->lblStatus->setVisible(true);
    QCoreApplication::processEvents();

    QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
    params->minimumMatch = 1;
    params->maxUnmatchedPerMatch = 50;
    params->alwaysPickAt = 9999;
    params->isAutomatic = true;
    params->useWeighting = true;
    params->useMoodAdjustment = true;
    params->name = "Recommendations";
    params->assignLikedToSources = true;
    params->userFFNId = env->interfaces.recs->GetUserProfile();
    QVector<int> sourceFics;
    for(auto fic : ids)
        sourceFics.push_back(fic);

    auto result = env->BuildRecommendations(params, sourceFics);
    return result;
}

bool InitialSetupDialog::ProcessRecommendationsFromFFNProfile()
{
    if(!authorTestSuccessfull)
    {
        if(!ui->leUserFFNId->text().isEmpty())
            VerifyUserID();
    }

    if(!authorTestSuccessfull)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Warning", "You haven't provided your FFN user ID or it is not valid.\n"
                                                       "This means recommendation list won't be generated automatically.\n"
                                                       "You will be able to do this later.\n"
                                                       "Do you want to finish initial setup?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No)
            return false;
    }
    return true;

}

bool InitialSetupDialog::ProcessRecommendationsFromListOfUrls()
{
    auto sources = PickFicIDsFromString(ui->edtUrls->toPlainText());
    if(sources.size() == 0)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Warning", "You haven't provided valid FFN urls to base your recommendations on.\n"
                                                       "This means recommendation list won't be generated automatically.\n"
                                                       "You will be able to do this later.\n"
                                                       "Do you want to finish initial setup?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No)
            return false;
    }
    return true;
}

QVector<int> InitialSetupDialog::PickFicIDsFromString(QString str)
{
    QVector<int> sourceFics;
    QStringList lines = str.split("\n");
    QRegularExpression rx("https://www.fanfiction.net/s/(\\d+)");
    for(auto line : lines)
    {
        if(!line.startsWith("http"))
            continue;
        auto match = rx.match(line);
        if(!match.hasMatch())
            continue;
        QString val = match.captured(1);
        sourceFics.push_back(val.toInt());
    }
    return sourceFics;
}

void InitialSetupDialog::on_pbVerifyUserFFNId_clicked()
{
    VerifyUserID();
}

void InitialSetupDialog::on_pbSelectDatabaseFile_clicked()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::FileMode::Directory);
    dialog.setDirectory(ui->leDBFileLocation->text());
    dialog.exec();
    if(dialog.directory().exists())
        ui->leDBFileLocation->setText(dialog.directory().path());
    else
        ui->leDBFileLocation->setText(QCoreApplication::applicationDirPath());
}

void InitialSetupDialog::on_pbPerformInit_clicked()
{
    ui->pbPerformInit->hide();
    ui->lblStatus->setVisible(true);
    readsSlash = ui->rbReadSlashYes->isChecked();

    if(ui->rbHaveFFNList->isChecked())
    {
        if(!ProcessRecommendationsFromFFNProfile())
            return;
    }
    else
    {
        if(!ProcessRecommendationsFromListOfUrls())
            return;
    }

    QDir dir(ui->leDBFileLocation->text());
    if(!dir.exists())
    {
        ui->lblStatus->setText("<font color=\"red\">Status: Folder for data is not valid.</font>");
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Warning", "You haven't provided a valid folder to store user data.\n"
                                                       "If you continue, it will be written to the folder with flipper's executable.\n"
                                                       "Do you want to continue?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No)
            return;
    }



    QSettings uiSettings("settings/ui.ini", QSettings::IniFormat);
    uiSettings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    if(dir.exists())
        uiSettings.setValue("Settings/dbPath", ui->leDBFileLocation->text());
    else
        uiSettings.setValue("Settings/dbPath", QCoreApplication::applicationDirPath());

    // first I need to actually set up the databases and init accessors
    ui->lblStatus->setText("<font color=\"darkBlue\">Status: Initializing database.</font>");
    QCoreApplication::processEvents();

    env->InstantiateClientDatabases(uiSettings.value("Settings/dbPath", QCoreApplication::applicationDirPath()).toString());
    ui->lblStatus->setText("<font color=\"darkBlue\">Status: Fetching initial data from server.</font>");
    QCoreApplication::processEvents();

    env->InitInterfaces();
    ui->lblStatus->setText("<font color=\"darkBlue\">Status: Instantiating environment.</font>");
    QCoreApplication::processEvents();

    env->Init();
    if(!env->status.isValid)
    {
        QCoreApplication::exit(1);
        return;
    }

    if(authorTestSuccessfull)
        env->interfaces.recs->SetUserProfile(ui->leUserFFNId->text().toInt());

    if(ui->rbHaveFFNList->isChecked() && authorTestSuccessfull)
    {
        ui->lblStatus->setText("<font color=\"darkBlue\">Status: Creating recommendations.</font>");
        QCoreApplication::processEvents();
        bool recommendationsResult = CreateRecommendationsFromProfile();
        if(!recommendationsResult)
        {
            QMessageBox::warning(nullptr, "Warning!", "Failed to create recommendations list for your profile.\n"
                                                      "Flipper will work as a pure search engine instead of recommendation engine.\n"
                                                      "You will be able to retry or create a new list later from URLs instead.\n"
                                                      "Use \"New Recommendation List\" button for that.");
        }
    }
    if(ui->rbNoFFNList->isChecked())
    {
        ui->lblStatus->setText("<font color=\"darkBlue\">Status: Creating recommendations.</font>");
        QCoreApplication::processEvents();
        bool recommendationsResult = CreateRecommendationsFromUrls(PickFicIDsFromString(ui->edtUrls->toPlainText()));
        if(!recommendationsResult)
        {
            QMessageBox::warning(nullptr, "Warning!", "Failed to create recommendations list for provided list of urls.\n"
                                                      "Flipper will work as a pure search engine instead of recommendation engine.\n"
                                                      "You will be able to retry or create a new list later when the application loads.\n"
                                                      "Use \"New Recommendation List\" button for that.");
        }
    }
    else
        recsCreated = true;

    uiSettings.setValue("Settings/initialInitComplete", true);
    uiSettings.sync();
    initComplete = true;
    hide();
}

void InitialSetupDialog::on_rbHaveFFNList_clicked()
{
    ui->wdgNoFaves->setVisible(false);
    ui->leUserFFNId->setVisible(true);
    ui->lblFFNIdInfo->setVisible(true);
    ui->pbVerifyUserFFNId->setVisible(true);
    ui->lblListInfo->setVisible(true);
}

void InitialSetupDialog::on_rbNoFFNList_clicked()
{
    ui->wdgNoFaves->setVisible(true);
    ui->leUserFFNId->setVisible(false);
    ui->lblFFNIdInfo->setVisible(false);
    ui->pbVerifyUserFFNId->setVisible(false);
    ui->lblListInfo->setVisible(false);
}
