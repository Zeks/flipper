#include "sql_abstractions/sql_transaction.h"
namespace sql{

Transaction::Transaction(Database)
{

}

Transaction::~Transaction()
{

}

bool Transaction::start()
{
    return true;
}

bool Transaction::cancel()
{
    return true;
}

bool Transaction::finalize()
{
    return true;
}


}
