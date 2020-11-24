#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_database_impl_base.h"
#include "sql_abstractions/sql_database_impl_null.h"
#include "sql_abstractions/sql_database_impl_sqlite.h"
namespace sql {
static std::unordered_map<std::string, Database> databases;

Database::Database():d(new DatabaseImplNull())
{

}

Database Database::addDatabase(std::string driver, std::string name)
{
    Q_UNUSED(driver);
    Database db;
    auto impl = std::make_shared<DatabaseImplSqlite>();
    impl->db =  impl->addDatabase(driver, name);
    db.d = impl;
    databases.insert_or_assign(name, db);
    return db;
}

Database Database::database(std::string name)
{
    auto it = databases.find(name);
    if(it != databases.end())
        return it->second;
    return Database();
}

void Database::setDatabaseName(std::string name)
{
    d->setDatabaseName(name);
}

std::string Database::driverType() const
{
    return d->driverType();
}

bool Database::open()
{
    return d->open();
}

bool Database::isOpen() const
{
    return d->isOpen();
}

bool Database::transaction()
{
    return d->transaction();
}

bool Database::commit()
{
    return d->commit();
}

bool Database::rollback()
{
    return d->rollback();
}

void *Database::internalPointer()
{
    return d->internalPointer();
}

void Database::close()
{
    d->close();
}

std::string Database::connectionName()
{
    return d->connectionName();
}

}
