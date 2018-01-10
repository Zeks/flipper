/*
FFSSE is a replacement search engine for fanfiction.net search results
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
#include "mainwindow.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "include/sqlitefunctions.h"
#include "include/db_fixers.h"

#include <QApplication>
#include <QDir>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("ffnet sane search engine");
    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
    QSharedPointer<database::IDBWrapper> pageCacheInterface (new database::SqliteInterface());
    QSettings settings("settings.ini", QSettings::IniFormat);
    if(settings.value("Settings/doBackups", true).toBool())
        dbInterface->BackupDatabase("CrawlerDB");
    qDebug() << "current appPath is: " << QDir::currentPath();
    auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
    if(settings.value("Settings/storeCache", false).toBool())
    {
        auto pageCacheDb = pageCacheInterface->InitDatabase("Service");
        pageCacheInterface->ReadDbFile("dbcode/pagecacheinit.sql", "Service");
    }
    dbInterface->ReadDbFile("dbcode/dbinit.sql");

    //dbfix::EnsureFandomIndexExists(mainDb);
//    /dbfix::FillFFNId(mainDb);
    MainWindow w;
    w.dbInterface = dbInterface;
    w.pageCacheInterface = pageCacheInterface;
    w.InitInterfaces();
    w.Init();
    w.show();
    w.StartTaskTimer();

    return a.exec();
}

