#include "discord/db_vendor.h"
#include "logger/QsLog.h"
#include <QUuid>
#include <QSqlQuery>
#include <QSqlError>

namespace discord {

QSharedPointer<LockedDatabase> DatabaseVendor::GetDatabase(QString name)
{
    QSharedPointer<LockedDatabase> databaseWrapper;
    if(name == "users")
    {
        databaseWrapper.reset(new LockedDatabase(usersLock));
        auto db = InstantiateDatabase(users);
        databaseWrapper->db = db;
    }
    else{
        databaseWrapper.reset(new LockedDatabase(pageCacheLock));
        auto db = InstantiateDatabase(pageCache);
        databaseWrapper->db = db;
    }
    return databaseWrapper;
}

void DatabaseVendor::AddConnectionToken(QString name, SqliteConnectionToken token)
{
    if(name == "users")
        users = token;
    else
        pageCache = token;
}

QSqlDatabase DatabaseVendor::InstantiateDatabase(const SqliteConnectionToken & token)
{
    QSqlDatabase db;
    db = QSqlDatabase::addDatabase("QSQLITE", token.databaseName + QUuid::createUuid().toString());
    QString filename = token.folder.isEmpty() ? token.databaseName : token.folder + "/" + token.databaseName;
    db.setDatabaseName(filename + ".sqlite");
    db.open();
    return db;
}

LockedDatabase::~LockedDatabase()
{
    db.close();
}

}
