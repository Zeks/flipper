/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
#include <QApplication>
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_query.h"
#include <QSqlError>
#include <QMetaType>
#include <QSqlDriver>
#include "sql_abstractions/sql_query.h"
#include <QSettings>
#include <QPluginLoader>

#include "include/ui/servitorwindow.h"
#include "include/Interfaces/interface_sqlite.h"

void SetupLogger()
{
    QSettings settings("settings/settings_server.ini", QSettings::IniFormat);

    An<QsLogging::Logger> logger;
    logger->setLoggingLevel(static_cast<QsLogging::Level>(settings.value("Logging/loglevel").toInt()));
    QString logFile = "servitor.log";
    QsLogging::DestinationPtr fileDestination(

                QsLogging::DestinationFactory::MakeFileDestination(logFile,
                                                                   settings.value("Logging/rotate", true).toBool(),
                                                                   settings.value("Logging/filesize", 5120).toInt()*1000000,
                                                                   settings.value("Logging/amountOfFilesToKeep", 50).toInt()));

    QsLogging::DestinationPtr debugDestination(
                QsLogging::DestinationFactory::MakeDebugOutputDestination() );
    logger->addDestination(debugDestination);
    logger->addDestination(fileDestination);
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("servitor");
    //database::BackupDatabase();
    SetupLogger();
    std::string path = "CrawlerDB.sqlite";
    sql::Database db = sql::Database::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    db.open();


    path = "PageCache.sqlite";
    sql::Database pcDb = sql::Database::addDatabase("QSQLITE", "PageCache");
    pcDb.setDatabaseName(path);
    pcDb.open();


//    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
//    auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
//    dbInterface->ReadDbFile("dbcode/dbinit.sql");



    ////////////////////////////////////
    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
    QSharedPointer<database::IDBWrapper> tasksInterface (new database::SqliteInterface());
    QSharedPointer<database::IDBWrapper> pageCacheInterface (new database::SqliteInterface());

    sql::Database mainDb;
    mainDb = dbInterface->InitDatabase("CrawlerDB", true);

    auto pageCacheDb = pageCacheInterface->InitDatabase("PageCache");

    dbInterface->ReadDbFile("dbcode/dbinit.sql", "CrawlerDB");

    pageCacheInterface->ReadDbFile("dbcode/pagecacheinit.sql", "PageCache");
    sql::Database tasksDb;
    tasksDb = tasksInterface->InitDatabase("Tasks");
    tasksInterface->ReadDbFile("dbcode/tasksinit.sql", "Tasks");
    ////////////////////////////////////
    QSharedPointer<database::IDBWrapper> userDbInterface (new database::SqliteInterface());
    auto userDb = userDbInterface->InitDatabase("UserDB");
    userDbInterface->ReadDbFile("dbcode/user_db_init.sql", "UserDB");
    userDbInterface->EnsureUUIDForUserDatabase();

    ServitorWindow w;
    w.dbInterface = dbInterface;
    w.env.interfaces.userDb = userDbInterface;
    w.env.interfaces.db = dbInterface;
    w.env.interfaces.pageCache= pageCacheInterface;
    w.env.interfaces.tasks = tasksInterface;
    w.env.InitInterfaces();
    w.env.userToken = QUuid::createUuid().toString();

    w.show();


    //database::EnsureFandomsFilled();
    //database::EnsureWebIdsFilled();
//    database::CalculateFandomFicCounts();
//    database::CalculateFandomAverages();
    //database::EnsureFFNUrlsShort();
    //auto result = database::EnsureTagForRecommendations();
    //database::PassTagsIntoTagsTable();

    return a.exec();
}
