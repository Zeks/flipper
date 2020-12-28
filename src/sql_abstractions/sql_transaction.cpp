#include "sql_abstractions/sql_transaction.h"
namespace sql{

Transaction::Transaction(Database db)
{
    this->db = db;
    start();
}

Transaction::~Transaction()
{
    if(ownTransaction)
        finalize();
}

bool Transaction::start()
{
    if(!db.hasOpenTransaction())
    {
        auto result = db.transaction();
        if(result){
            ownTransaction = true;
            return true;
        }
        else
            return false;
    }
    ownTransaction = false;
    return true;
}

bool Transaction::cancel()
{
    ownTransaction = false;
    return db.rollback();
}

bool Transaction::finalize()
{
    bool result = false;
    if(ownTransaction)
        result = db.commit();
    ownTransaction = false;
    return result;
}


}
