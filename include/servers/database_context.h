#pragma once
#include <QSharedPointer>

namespace database{
    class IDBWrapper;
}

class DatabaseContext{
public:
    DatabaseContext();
    QSharedPointer<database::IDBWrapper> dbInterface;
};
