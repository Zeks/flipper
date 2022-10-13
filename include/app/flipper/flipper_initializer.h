#pragma once

#include "ui/initialsetupdialog.h"
#include "Interfaces/db_interface.h"

#include "include/sqlitefunctions.h"
#include "include/pure_sql.h"
#include "include/db_fixers.h"
#include "include/sql/backups.h"
#include "logger/QsLog.h"
#include <QSettings>
#include <QTextCodec>


class MainWindow;
struct FlipperInitializer{
    FlipperInitializer():
        uiSettings("settings/ui.ini", QSettings::IniFormat),
        settings("settings/settings.ini", QSettings::IniFormat)
    {
        uiSettings.setIniCodec(QTextCodec::codecForName("UTF-8"));
        settings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    }
    bool Init();

private:
    void SetupLogger() const;
    bool RequiresReinit() const;
    void EnsureBackupPathExists(QString path) const;
    void EnsureDelayForUserParsing() const;

    void ProcessStateOfExistingDatabase();
    bool PerformInitialSetup();
    bool SetupForValidDatabase();
    bool SetupMainWindow();

    QSharedPointer<CoreEnvironment> coreEnvironment;
    QSharedPointer<MainWindow> w;

    bool hasDBFile = false;
    bool backupRestored = false;
    bool newInstanceCreatedRecs = false;
    bool slashFilterScheduled = false;;
    QSettings uiSettings;
    QSettings settings;
};

