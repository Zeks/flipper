/*
Flipper is a recommendation and search engine for fanfiction.net
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
#include "include/sqlitefunctions.h"
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

    auto db = database::sqlite::InitAndUpdateSqliteDatabaseForFile("database","CrawlerDB","dbcode/dbinit.sql", "CrawlerDB", true);
    auto pcDb = database::sqlite::InitAndUpdateSqliteDatabaseForFile("database","PageCache","dbcode/pagecacheinit.sql", "PageCache", false);
    auto tasksDb = database::sqlite::InitAndUpdateSqliteDatabaseForFile("database","Tasks","dbcode/tasksinit.sql", "Tasks", false);

    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
    QSharedPointer<database::IDBWrapper> tasksInterface (new database::SqliteInterface());
    QSharedPointer<database::IDBWrapper> pageCacheInterface (new database::SqliteInterface());
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
    return a.exec();
}
