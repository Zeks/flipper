#include "mainwindow.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "include/sqlitefunctions.h"
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
    QSharedPointer<database::IDBWrapper> portableInterface (new database::SqliteInterface()); //! todo needs to be sqlite database with funcs
    portableInterface->BackupDatabase("Crawler.sqlite");

    portableInterface->InitDatabase();
    portableInterface->InitDatabase("pagecache");

    portableInterface->ReadDbFile("dbcode/dbinit.sql");
    portableInterface->ReadDbFile("dbcode/pagecacheinit.sql", "pagecache");

    MainWindow w;
    w.show();

    return a.exec();
}
