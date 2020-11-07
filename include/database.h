#pragma once
#include "sql_abstractions/sql_database.h"

namespace core{
struct Database{
    Database(){}
    virtual ~Database(){}
    sql::Database db;
};
}
