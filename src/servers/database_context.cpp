#include "servers/database_context.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
static QString GetDbNameFromCurrentThread(){
    std::stringstream ss;
    ss << std::this_thread::get_id();
    std::string id = ss.str();
    return QString("Crawler_") + QString::fromStdString(id);
}

DatabaseContext::DatabaseContext(){
    dbInterface.reset(new database::SqliteInterface());
    QString name = GetDbNameFromCurrentThread();
    qDebug() << "OPENING CONNECTION:" << name;
    dbInterface->InitDatabase2("CrawlerDB", name, false);
}
