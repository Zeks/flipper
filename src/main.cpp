#include "mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMetaType>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QPluginLoader>
#include "include/init_database.h"
#include "../../Qt/qt-everywhere-opensource-src-5.6.0/qtbase/src/3rdparty/sqlite/sqlite3.h"


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
    database::BackupDatabase();
    QString path = "CrawlerDB.sqlite";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    db.open();


    database::ReadDbFile();
    database::ReindexTable("tags");
    MainWindow w;
    w.show();
    w.CheckSectionAvailability();

    database::InstallCustomFunctions();
    database::EnsureFandomsFilled();


    return a.exec();
}
