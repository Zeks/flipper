#include "mainwindow.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"

#include "Interfaces/ffn/ffn_authors.h"
#include "Interfaces/ffn/ffn_fanfics.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/recommendation_lists.h"
#include "Interfaces/tags.h"

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
    QSharedPointer<database::IDBWrapper> portableInterface (new database::SqliteInterface());
    portableInterface->BackupDatabase("Crawler.sqlite");

    portableInterface->InitDatabase();
    portableInterface->InitDatabase("pagecache");

    portableInterface->ReadDbFile("dbcode/dbinit.sql");
    portableInterface->ReadDbFile("dbcode/pagecacheinit.sql", "pagecache");


    QSharedPointer<interfaces::Authors> authors (new interfaces::FFNAuthors());
    QSharedPointer<interfaces::Fanfics> fanfics (new interfaces::FFNFanfics());
    QSharedPointer<interfaces::RecommendationLists> recommendations (new interfaces::RecommendationLists());
    QSharedPointer<interfaces::Fandoms> fandoms (new interfaces::Fandoms());
    QSharedPointer<interfaces::Tags> tags (new interfaces::Tags());

    MainWindow w;
    w.show();

    return a.exec();
}
