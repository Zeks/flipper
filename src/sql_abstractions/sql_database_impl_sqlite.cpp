#include "sql_abstractions/sql_database_impl_sqlite.h"

QSqlDatabase sql::DatabaseImplSqlite::addDatabase(std::string type, std::string name)
{
   return QSqlDatabase::addDatabase(QString::fromStdString(type),QString::fromStdString(name)) ;
}

QSqlDatabase sql::DatabaseImplSqlite::database(std::string name)
{
    return QSqlDatabase::addDatabase(QString::fromStdString(name)) ;
}

void sql::DatabaseImplSqlite::setConnectionToken(ConnectionToken token)
{
    db.setDatabaseName(QString::fromStdString(token.serviceName));
}

bool sql::DatabaseImplSqlite::open()
{
    return db.open();
}

bool sql::DatabaseImplSqlite::isOpen() const
{
    return db.isOpen();
}

bool sql::DatabaseImplSqlite::transaction()
{
    _hasOpenTransaction = db.transaction();;
    return _hasOpenTransaction;
}

bool sql::DatabaseImplSqlite::hasOpenTransaction() const
{
    return _hasOpenTransaction;
}

bool sql::DatabaseImplSqlite::commit()
{
    _hasOpenTransaction = false;
    return db.commit();
}

bool sql::DatabaseImplSqlite::rollback()
{
    _hasOpenTransaction = false;
    return db.rollback();
}

void sql::DatabaseImplSqlite::close()
{
    _hasOpenTransaction = false;
    db.close();
}

std::string sql::DatabaseImplSqlite::connectionName()
{
    return db.connectionName().toStdString();
}

void *sql::DatabaseImplSqlite::internalPointer()
{
    return static_cast<void*>(&db);
}

bool sql::DatabaseImplSqlite::isNull()
{
    return false;
}

std::string sql::DatabaseImplSqlite::driverType() const
{
    return "QSQLITE";
}
