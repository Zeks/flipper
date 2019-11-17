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
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "include/sqlitefunctions.h"
#include "include/db_fixers.h"

#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QSettings>
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

    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
    QSharedPointer<database::IDBWrapper> userDbInterface (new database::SqliteInterface());
    QSharedPointer<database::IDBWrapper> tasksInterface (new database::SqliteInterface());
    QSharedPointer<database::IDBWrapper> pageCacheInterface (new database::SqliteInterface());
    int threads =  sqlite3_threadsafe();
    QSettings settings("settings/settings.ini", QSettings::IniFormat);
    if(settings.value("Settings/doBackups", true).toBool())
        dbInterface->BackupDatabase("CrawlerDB");
    qDebug() << "current appPath is: " << QDir::currentPath();

    bool mainDBIsCrawler= settings.value("Settings/thinClient").toBool();
    QSqlDatabase mainDb, userDb;
    if(mainDBIsCrawler)
    {
//        QLOG_INFO() << "Init CrawlerDB";
//        mainDb = dbInterface->InitDatabase("CrawlerDB", true);
        QLOG_INFO() << "Init UserDB";
        userDb = userDbInterface->InitDatabase("UserDB", false);
        userDbInterface->EnsureUUIDForUserDatabase();
    }
    else
    {
//        QLOG_INFO() << "Init CrawlerDB";
//        mainDb = dbInterface->InitDatabase("CrawlerDB", false);
        QLOG_INFO() << "Init UserDB";
        userDb = userDbInterface->InitDatabase("UserDB", true);
        userDbInterface->EnsureUUIDForUserDatabase();
    }
    auto pageCacheDb = pageCacheInterface->InitDatabase("PageCache");

//    QLOG_INFO() << "reading CrawlerDB";
//    dbInterface->ReadDbFile("dbcode/dbinit.sql", "CrawlerDB");
    QLOG_INFO() << "reading UserDB";
    userDbInterface->ReadDbFile("dbcode/user_db_init.sql", "UserDB");
    QLOG_INFO() << "reading PageCache";
    pageCacheInterface->ReadDbFile("dbcode/pagecacheinit.sql", "PageCache");
    if(mainDBIsCrawler)
    {
//        QLOG_INFO() << "reinit CrawlerDB";
//        mainDb = dbInterface->InitDatabase("CrawlerDB", true);
        QLOG_INFO() << "reinit UserDB";
        userDb = userDbInterface->InitDatabase("UserDB", false);
        userDbInterface->EnsureUUIDForUserDatabase();
    }
    else
    {
//        QLOG_INFO() << "reinit CrawlerDB";
//        mainDb = dbInterface->InitDatabase("CrawlerDB", false);
        QLOG_INFO() << "reinit UserDB";
        userDb = userDbInterface->InitDatabase("UserDB", true);
        userDbInterface->EnsureUUIDForUserDatabase();
    }
    QSqlDatabase tasksDb;
    QLOG_INFO() << "Init Tasks";
    tasksDb = tasksInterface->InitDatabase("Tasks");
    QLOG_INFO() << "reading Tasks";
    tasksInterface->ReadDbFile("dbcode/tasksinit.sql", "Tasks");
    QLOG_INFO() << "finished db";

    MainWindow w;
    w.env.interfaces.db = dbInterface;
    w.env.interfaces.userDb = userDbInterface;
    w.env.interfaces.pageCache= pageCacheInterface;
    w.env.interfaces.tasks = tasksInterface;
    w.InitInterfaces();
    if(!w.Init())
        return 0;
    w.InitConnections();
    w.show();
    w.DisplayInitialFicSelection();
    w.StartTaskTimer();

    return a.exec();
}

