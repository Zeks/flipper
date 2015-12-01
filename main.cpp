#include "mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMetaType>
#include <QSqlDriver>
#include <QPluginLoader>
#include <sqlite3.h>

void qtregexp(sqlite3_context* ctx, int argc, sqlite3_value** argv)
{
    QRegExp regex;
    QString str1((const char*)sqlite3_value_text(argv[0]));
    QString str2((const char*)sqlite3_value_text(argv[1]));

    regex.setPattern(str1);
    regex.setCaseSensitivity(Qt::CaseInsensitive);

    bool b = str2.contains(regex);

    if (b)
    {
        sqlite3_result_int(ctx, 1);
    }
    else
    {
        sqlite3_result_int(ctx, 0);
    }
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
