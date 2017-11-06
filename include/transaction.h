#pragma once

#include <QSqlDatabase>
#include <QReadWriteLock>
#include <QSet>

namespace database{
class Transaction{
public:
    Transaction(QSqlDatabase);
    ~Transaction();
    bool start();
    bool cancel();
    bool finalize();
    QSqlDatabase db;
    bool isOpen = false;
    static QReadWriteLock lock;
    static QSet<QString> transactionSet;
};

}
