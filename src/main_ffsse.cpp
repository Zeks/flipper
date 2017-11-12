#include "mainwindow.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "include/sqlitefunctions.h"

#include <QApplication>
#include <QDir>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("ffnet sane search engine");
    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
    QSharedPointer<database::IDBWrapper> pageCacheInterface (new database::SqliteInterface());
    //dbInterface->BackupDatabase("Crawler.sqlite");
    qDebug() << "current appPath is: " << QDir::currentPath();
    auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
    auto pageCacheDb = pageCacheInterface->InitDatabase("PageCache");
    dbInterface->ReadDbFile("dbcode/dbinit.sql");

    pageCacheInterface->ReadDbFile("dbcode/pagecacheinit.sql", "PageCache");


    MainWindow w;
    w.dbInterface = dbInterface;
    w.pageCacheInterface = pageCacheInterface;
    w.InitInterfaces();
    w.Init();
    w.show();

    return a.exec();
}
