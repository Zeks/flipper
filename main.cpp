#include "mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("ffnet sane search engine");

    QString path = "CrawlerDB.sqlite";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//not dbConnection
    db.setDatabaseName(path);
    db.open();


    QString createFanfics = "create table if not exists FANFICS "
                            "(FANDOM VARCHAR NOT NULL,"
            "AUTHOR VARCHAR NOT NULL,"
            "TITLE VARCHAR NOT NULL,"
            "WORDCOUNT VARCHAR NOT NULL,"
            "SUMMARY VARCHAR NOT NULL,"
            "GENRES VARCHAR,"
            "PUBLISHED DATETIME NOT NULL,"
            "UPDATED DATETIME NOT NULL,"
            "URL VARCHAR NOT NULL,"
            "TAGS VARCHAR NOT NULL,"
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


    MainWindow w;
    w.show();



    return a.exec();
}
