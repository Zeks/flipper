#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_database_impl_base.h"
#include "sql_abstractions/sql_database_impl_null.h"
#include "sql_abstractions/sql_database_impl_sqlite.h"
#include <locale>
namespace sql {
static std::unordered_map<std::string, Database> databases;

Database::Database():d(new DatabaseImplNull())
{

}

static void toUpper(std::string& s){
    std::locale locale;
    auto to_upper = [&locale] (char ch) { return std::use_facet<std::ctype<char>>(locale).toupper(ch); };
    std::transform(s.begin(), s.end(), s.begin(), to_upper);
}

Database Database::addDatabase(std::string driver, std::string name)
{
    toUpper(driver);toUpper(name);
    Database db;
    auto impl = std::make_shared<DatabaseImplSqlite>();
    impl->db =  impl->addDatabase(driver, name);
    if(impl->connectionName() != name)
        throw std::runtime_error("database wasn't created properly");
    db.d = impl;
    databases.insert_or_assign(name, db);
    return db;
}

void Database::removeDatabase(std::string name)
{
    toUpper(name);
    databases.erase(name);
}

Database Database::database(std::string name)
{
    toUpper(name);
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
