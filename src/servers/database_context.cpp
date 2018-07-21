#include "servers/database_context.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"

DatabaseContext::DatabaseContext(){
    dbInterface.reset(new database::SqliteInterface());
    dbInterface->InitDatabase("CrawlerDB", true);
}
