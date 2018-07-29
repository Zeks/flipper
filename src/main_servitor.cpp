#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMetaType>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSettings>
#include <QPluginLoader>

#include "include/servitorwindow.h"
#include "include/Interfaces/interface_sqlite.h"

void SetupLogger()
{
    QSettings settings("settings_server.ini", QSettings::IniFormat);

    An<QsLogging::Logger> logger;
    logger->setLoggingLevel(static_cast<QsLogging::Level>(settings.value("Logging/loglevel").toInt()));
    QString logFile = settings.value("Logging/filename").toString();
    QsLogging::DestinationPtr fileDestination(

                QsLogging::DestinationFactory::MakeFileDestination(logFile,
                                                                   settings.value("Logging/rotate", true).toBool(),
                                                                   settings.value("Logging/filesize", 512).toInt()*1000000,
                                                                   settings.value("Logging/amountOfFilesToKeep", 50).toInt()));

    QsLogging::DestinationPtr debugDestination(
                QsLogging::DestinationFactory::MakeDebugOutputDestination() );
    logger->addDestination(debugDestination);
    logger->addDestination(fileDestination);
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("servitor");
    //database::BackupDatabase();
    SetupLogger();
    QString path = "CrawlerDB.sqlite";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    db.open();


    path = "PageCache.sqlite";
    QSqlDatabase pcDb = QSqlDatabase::addDatabase("QSQLITE", "PageCache");
    pcDb.setDatabaseName(path);
    pcDb.open();


    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
    auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
    dbInterface->ReadDbFile("dbcode/dbinit.sql");


    ServitorWindow w;
    w.dbInterface = dbInterface;
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
