#include "servers/database_context.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"

DatabaseContext::DatabaseContext(){
    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
    auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
    //dbInterface->ReadDbFile("dbcode/dbinit.sql");
}
