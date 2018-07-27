#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMetaType>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QPluginLoader>

#include "include/servitorwindow.h"
#include "include/Interfaces/interface_sqlite.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("servitor");
    //database::BackupDatabase();
    QString path = "CrawlerDB.sqlite";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    db.open();


    path = "PageCache.sqlite";
    QSqlDatabase pcDb = QSqlDatabase::addDatabase("QSQLITE", "pagecache");
    pcDb.setDatabaseName(path);
    pcDb.open();


    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
    auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
    dbInterface->ReadDbFile("dbcode/dbinit.sql");


    ServitorWindow w;
    w.show();


    //database::EnsureFandomsFilled();
    //database::EnsureWebIdsFilled();
//    database::CalculateFandomFicCounts();
//    database::CalculateFandomAverages();
    //database::EnsureFFNUrlsShort();
    //auto result = database::EnsureTagForRecommendations();
    //database::PassTagsIntoTagsTable();

    return a.exec();
}
