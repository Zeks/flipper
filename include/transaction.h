#pragma once

#include <QSqlDatabase>

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
};

}
