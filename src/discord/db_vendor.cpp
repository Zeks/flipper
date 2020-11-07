#include "discord/db_vendor.h"
#include "logger/QsLog.h"
#include <QUuid>
#include "sql_abstractions/sql_query.h"
#include <QSqlError>

namespace discord {

QSharedPointer<LockedDatabase> DatabaseVendor::GetDatabase(QString name)
{
    QSharedPointer<LockedDatabase> databaseWrapper;
    if(name == QStringLiteral("users"))
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

void DatabaseVendor::AddConnectionToken(QString name, const SqliteConnectionToken& token)
{
    if(name == QStringLiteral("users"))
        users = token;
    else
        pageCache = token;
}

sql::Database DatabaseVendor::InstantiateDatabase(const SqliteConnectionToken & token)
{
    sql::Database db;
    db = sql::Database::addDatabase(QStringLiteral("QSQLITE"), token.databaseName + QUuid::createUuid().toString());
    QString filename = token.folder.isEmpty() ? token.databaseName : token.folder + "/" + token.databaseName;
    db.setDatabaseName(filename + QStringLiteral(".sqlite"));
    db.open();
    return db;
}

LockedDatabase::~LockedDatabase()
{
    db.close();
}

}
