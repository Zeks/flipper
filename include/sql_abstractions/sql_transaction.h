#pragma once

#include <QReadWriteLock>
#include <set>
#include "sql_abstractions/sql_database.h"

namespace sql{
class Transaction{
public:
    Transaction(SqlDatabase);
    Transaction(const Transaction&) = default;
    ~Transaction();
    bool start();
    bool cancel();
    bool finalize();
    SqlDatabase db;
    bool isOpen = false;
    static QReadWriteLock lock;
    static std::set<std::string> transactionSet;
};
}
