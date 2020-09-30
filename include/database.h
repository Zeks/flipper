#pragma once
#include <QSqlDatabase>

namespace core{
struct Database{
    Database(){}
    virtual ~Database(){}
    QSqlDatabase db;
};
}
