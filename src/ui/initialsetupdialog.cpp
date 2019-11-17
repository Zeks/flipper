#include "include/ui/initialsetupdialog.h"
#include "ui_initialsetupdialog.h"
#include "Interfaces/recommendation_lists.h"
#include <QRegularExpression>
#include <QCoreApplication>
#include <QFileDialog>
#include <QDir>
#include <QSettings>
#include <QMessageBox>
#include <QTextCodec>

InitialSetupDialog::InitialSetupDialog(QDialog *parent) :
    QDialog(parent),
    ui(new Ui::InitialSetupDialog)
{
    ui->setupUi(this);
    ui->leDBFileLocation->setText(QCoreApplication::applicationDirPath());
    QString info = "Since it's the first time flipper has been launched, let's do some initial setup\n"
            "If you have used Flipper previously, don't forget to point it to your database folder.";
    ui->lblInfo->setText(info);
    ui->pbInitializationStatus->setVisible(false);
}

InitialSetupDialog::~InitialSetupDialog()
{
    delete ui;
}

void InitialSetupDialog::VerifyUserID()
{
    auto userID = ui->leUserFFNId->text();
    ui->lblStatus->setText("<font color=\"darkBlue\">Status: Verifying user ID.</font>");

    QCoreApplication::processEvents();
    authorTestSuccessfull =  env->TestAuthorID(ui->leUserFFNId, ui->lblStatus);

}

bool InitialSetupDialog::CreateRecommendations()
{
    ui->lblStatus->setText("<font color=\"darkBlue\">Status: Creating recommendation list, this may take a while.</font>");
    QCoreApplication::processEvents();

    QString url = "https://www.fanfiction.net/u/" + ui->leUserFFNId->text();
    auto sourceFicsSet = env->LoadAuthorFicIdsForRecCreation(url);

    QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
    params->minimumMatch = 6;
    params->maxUnmatchedPerMatch = 50;
    params->maximumNegativeMatches = -1;
    params->alwaysPickAt = 9999;
    params->isAutomatic = true;
    params->useWeighting = true;
    params->useMoodAdjustment = true;
    params->name = "Recommendations";
    params->userFFNId = env->interfaces.recs->GetUserProfile();
    QVector<int> sourceFics;
    for(auto fic : sourceFicsSet)
        sourceFics.push_back(fic.toInt());

    auto result = env->BuildRecommendations(params, sourceFics, false, false);
    return result;
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
              return;
    }

    QDir dir(ui->leDBFileLocation->text());
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

    env->interfaces.recs->SetUserProfile(ui->leUserFFNId->text().toInt());
    if(authorTestSuccessfull)
    {
        ui->lblStatus->setText("<font color=\"darkBlue\">Status: Creating recommendations.</font>");
        QCoreApplication::processEvents();
        bool recommendationsResult = CreateRecommendations();
        if(!recommendationsResult)
        {
            QMessageBox::warning(nullptr, "Warning!", "Failed to create recommendations list for your profile.\n"
                                                      "Flipper will work as a pure search engine instead of recommendation engine.\n"
                                                      "You will be able to retry or create a new list later from URLs instead.\n"
                                                      "Using \"New Recommendation List\" button.");
        }
    }
    uiSettings.setValue("Settings/initialInitComplete", true);
    uiSettings.sync();
    hide();
}
