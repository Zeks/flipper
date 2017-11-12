#include "transaction.h"
#include <QReadLocker>
#include <QWriteLocker>
QReadWriteLock database::Transaction::lock;
QSet<QString>  database::Transaction::transactionSet;
database::Transaction::Transaction(QSqlDatabase db)
{
    this->db = db;
    QWriteLocker locker(&lock);
    if(!transactionSet.contains(db.connectionName()))
        start();
    transactionSet.insert(db.connectionName());
}

database::Transaction::~Transaction()
{
    if(isOpen)
    {
        db.rollback();
        QWriteLocker locker(&lock);
        transactionSet.remove(db.connectionName());
    }
}

bool database::Transaction::start()
{
    if(!db.isOpen())
        return false;
    isOpen = db.transaction();
    return isOpen;
}

bool database::Transaction::cancel()
{
    if(!db.isOpen())
        return false;
    if(!db.rollback())
        return false;
    isOpen = false;
    return true;
}

bool database::Transaction::finalize()
{
    if(!db.isOpen() || !isOpen)
        return false;
    if(!db.commit())
        return false;
    isOpen = false;
    return true;
}
