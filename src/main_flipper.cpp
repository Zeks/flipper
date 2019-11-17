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
#include "ui/mainwindow.h"
#include "ui/initialsetupdialog.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "include/sqlitefunctions.h"
#include "include/db_fixers.h"

#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QTextCodec>
void SetupLogger()
{
    QSettings settings("settings/settings.ini", QSettings::IniFormat);

    An<QsLogging::Logger> logger;
    logger->setLoggingLevel(static_cast<QsLogging::Level>(settings.value("Logging/loglevel").toInt()));
    QString logFile = settings.value("Logging/filename").toString();
    QsLogging::DestinationPtr fileDestination(

                QsLogging::DestinationFactory::MakeFileDestination(logFile,
                                                                   settings.value("Logging/rotate", true).toBool(),
                                                                   settings.value("Logging/filesize", 512).toInt()*1000000,
                                                                   settings.value("Logging/amountOfFilesToKeep", 50).toInt()));

    QsLogging::DestinationPtr debugDestination(
                QsLogging::DestinationFactory::MakeDebugOutputDestination() );
    logger->addDestination(debugDestination);
    logger->addDestination(fileDestination);

}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Flipper");
    SetupLogger();

    QSharedPointer<CoreEnvironment> coreEnvironment(new CoreEnvironment());
    qDebug() << "current appPath is: " << QDir::currentPath();

    QSettings uiSettings("settings/ui.ini", QSettings::IniFormat);
    uiSettings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    if(!uiSettings.value("Settings/initialInitComplete", false).toBool())
    {
        InitialSetupDialog setupDialog;
        setupDialog.setWindowModality(Qt::ApplicationModal);
        setupDialog.env = coreEnvironment;
        setupDialog.exec();
    }
    else
    {
        coreEnvironment->InstantiateClientDatabases(uiSettings.value("Settings/dbPath", QCoreApplication::applicationDirPath()).toString());
        coreEnvironment->InitInterfaces();
        coreEnvironment->Init();
    }



    MainWindow w;
    w.env = coreEnvironment;
    w.InitInterfaces();
    if(!w.Init())
        return 0;
    w.InitConnections();
    w.show();
    w.DisplayInitialFicSelection();
    w.StartTaskTimer();

    return a.exec();
}

