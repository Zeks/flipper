#include "mainwindow.h"
#include "Interfaces/db_interface.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMetaType>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QPluginLoader>

void CreateIndex(QString value)
{
    QSqlDatabase db = QSqlDatabase::database("CrawlerDB.sqlite");
    QSqlQuery q(db);
    q.prepare(value);
    q.exec();
    qDebug() << q.lastError();
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("ffnet sane search engine");
    QSharedPointer<database::IDBWrapper> portableInterface; //! todo needs to be sqlite database with funcs
    portableInterface->BackupDatabase("Crawler.sqlite");

// these go to initdatabase
//    QString path = "CrawlerDB.sqlite";
//    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
//    db.setDatabaseName(path);
//    db.open();
//    path = "PageCache.sqlite";
//    QSqlDatabase pcDb = QSqlDatabase::addDatabase("QSQLITE", "pagecache");
//    pcDb.setDatabaseName(path);
//    pcDb.open();
//   database::InstallCustomFunctions();

    portableInterface->InitDatabase();
    portableInterface->InitDatabase("pagecache");

    portableInterface->ReadDbFile("dbcode/dbinit.sql");
    portableInterface->ReadDbFile("dbcode/pagecacheinit.sql", "pagecache");

    MainWindow w;
    w.show();
    //!todo rethink
    //w.CheckSectionAvailability();

    return a.exec();
}
