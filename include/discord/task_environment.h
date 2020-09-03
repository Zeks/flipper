#pragma once
#include <QSharedPointer>
namespace interfaces{
class Fandoms;
class Authors;
class Fanfics;
};
class FicSourceGRPC;

namespace database{
class IDBWrapper;
};
namespace discord {
    class TaskEnvironment{
    public:
        void Init();


        QSharedPointer<FicSourceGRPC> ficSource;
        QSharedPointer<database::IDBWrapper> userDbInterface;
        QSharedPointer<interfaces::Fandoms> fandoms;
        QSharedPointer<interfaces::Fanfics> fanfics;
        QSharedPointer<interfaces::Authors> authors;
    };

}
