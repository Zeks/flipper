#include "transaction.h"

database::Transaction::Transaction(QSqlDatabase db)
{
    this->db = db;
    start();
}

database::Transaction::~Transaction()
{
    if(isOpen)
        db.rollback();
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
    if(!db.isOpen())
        return false;
    if(!db.commit())
        return false;
    isOpen = true;
}
