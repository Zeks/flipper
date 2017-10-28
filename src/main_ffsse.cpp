#include "mainwindow.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "include/sqlitefunctions.h"

#include <QApplication>

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

    MainWindow w;
    w.portableInterface = portableInterface;
    w.InitInterfaces();
    w.show();

    return a.exec();
}
