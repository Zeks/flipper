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
            "COMPLETE INTEGER NOT NULL DEFAULT 0,"
            "AT_CHAPTER INTEGER NOT NULL,"
            "ORIGIN VARCHAR NOT NULL"
            "ID INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL  UNIQUE,)";

    QSqlQuery q(db);
    q.prepare(createFanfics);
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();


    QString createFandoms = "create table if not exists fandoms "
                            "(FANDOM VARCHAR NOT NULL,"
            " SECTION VARCHAR NOT NULL, "
            "NORMAL_URL VARCHAR NOT NULL,"
            "CROSSOVER_URL VARCHAR NOT NULL)";

    q.prepare(createFandoms);
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();



    QString createTags = "create table if not exists tags "
                            "(tag VARCHAR unique NOT NULL)";


    q.prepare(createTags);
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();

    CreateIndex("CREATE INDEX if not exists  \"main\".\"I_FANFICS_IDENTITY\" ON \"FANFICS\" (\"AUTHOR\" ASC, \"TITLE\" ASC)");
    CreateIndex("CREATE  INDEX  if not exists  \"main\".\"I_FANFICS_FANDOM\" ON \"FANFICS\" (\"FANDOM\" ASC)");
    CreateIndex("CREATE  INDEX if  not exists \"main\".\"I_FANFICS_WORDCOUNT\" ON \"FANFICS\" (\"WORDCOUNT\" ASC)");
    CreateIndex("CREATE  INDEX if  not exists \"main\".\"I_FANFICS_TAGS\" ON \"FANFICS\" (\"TAGS\" ASC)");
    CreateIndex("CREATE  INDEX if  not exists \"main\".\"I_FANFICS_GENRES\" ON \"FANFICS\" (\"GENRES\" ASC)");
    CreateIndex(" CREATE  INDEX if  not exists \"main\".\"I_FANDOMS_FANDOM\" ON \"FANDOMS\" (\"FANDOM\" ASC)");
    CreateIndex(" CREATE  INDEX if  not exists \"main\".\"I_FANDOMS_SECTION\" ON \"FANDOMS\" (\"SECTION\" ASC)");



    QString pageCache = "CREATE TABLE if not exists \"PageCache\" (\"URL\" VARCHAR PRIMARY KEY  NOT NULL , \"GENERATION_DATE\" DATETIME,"
    " \"CONTENT\" BLOB, \"NEXT\" VARCHAR, \"FANDOM\" VARCHAR, \"CROSSOVER\" INTEGER, \"PREVIOUS\" VARCHAR, \"REFERENCED_FICS\" BLOB, \"PAGE_TYPE\" INTEGER)";
    q.prepare(pageCache);
    q.exec();
    if(q.lastError().isValid())
        qDebug() << q.lastError();

    MainWindow w;
    w.show();
    w.CheckSectionAvailability();


    return a.exec();
}
