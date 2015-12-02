#include "mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMetaType>
#include <QSqlDriver>
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

    QString path = "CrawlerDB.sqlite";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE_R");
    db.setDatabaseName(path);
    db.open();

    QString createFanfics = "create table if not exists FANFICS "
                            "(FANDOM VARCHAR NOT NULL,"
            "AUTHOR VARCHAR NOT NULL,"
            "TITLE VARCHAR NOT NULL,"
            "SUMMARY VARCHAR NOT NULL,"
            "GENRES VARCHAR,"
            "CHARACTERS VARCHAR,"
            "RATED VARCHAR,"
            "PUBLISHED DATETIME NOT NULL,"
            "UPDATED DATETIME NOT NULL,"
            "URL VARCHAR NOT NULL,"
            "TAGS VARCHAR NOT NULL DEFAULT ' none ',"
            "WORDCOUNT INTEGER NOT NULL,"
            "FAVOURITES INTEGER NOT NULL,"
            "REVIEWS INTEGER NOT NULL,"
            "CHAPTERS INTEGER NOT NULL,"
            "ORIGIN VARCHAR NOT NULL)";

    QSqlQuery q(db);
    q.prepare(createFanfics);
    q.exec();
    qDebug() << q.lastError();


    QString createFandoms = "create table if not exists fandoms "
                            "(FANDOM VARCHAR NOT NULL,"
            " SECTION VARCHAR NOT NULL, "
            "URL VARCHAR NOT NULL)";

    q.prepare(createFandoms);
    q.exec();
    qDebug() << q.lastError();



    CreateIndex("CREATE INDEX if not exists  \"main\".\"I_FANFICS_IDENTITY\" ON \"FANFICS\" (\"AUTHOR\" ASC, \"TITLE\" ASC)");
    CreateIndex("CREATE  INDEX  if not exists  \"main\".\"I_FANFICS_FANDOM\" ON \"FANFICS\" (\"FANDOM\" ASC)");
    CreateIndex("CREATE  INDEX if  not exists \"main\".\"I_FANFICS_WORDCOUNT\" ON \"FANFICS\" (\"WORDCOUNT\" ASC)");
    CreateIndex("CREATE  INDEX if  not exists \"main\".\"I_FANFICS_TAGS\" ON \"FANFICS\" (\"TAGS\" ASC)");
    CreateIndex("CREATE  INDEX if  not exists \"main\".\"I_FANFICS_GENRES\" ON \"FANFICS\" (\"GENRES\" ASC)");
    CreateIndex(" CREATE  INDEX if  not exists \"main\".\"I_FANDOMS_FANDOM\" ON \"FANDOMS\" (\"FANDOM\" ASC)");
    CreateIndex(" CREATE  INDEX if  not exists \"main\".\"I_FANDOMS_SECTION\" ON \"FANDOMS\" (\"SECTION\" ASC)");



    MainWindow w;
    w.show();



    return a.exec();
}
