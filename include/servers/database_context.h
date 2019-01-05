#pragma once
#include <QSharedPointer>

namespace database{
    class IDBWrapper;
}
namespace interfaces {
class Fanfics;
class Fandoms;
class Authors;
}

class DatabaseContext{
public:
    DatabaseContext();
    void InitFanfics();
    void InitFandoms();
    void InitAuthors();
    QSharedPointer<database::IDBWrapper> dbInterface;
    QSharedPointer<interfaces::Fanfics>  fanfics;
    QSharedPointer<interfaces::Fandoms>  fandoms;
    QSharedPointer<interfaces::Authors>  authors;
};
