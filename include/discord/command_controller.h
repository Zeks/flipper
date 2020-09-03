#pragma once
#include <QSet>

namespace discord {
class TaskRunner;
class CommandController{
    CommandController();
    QSet<QString> activeUsers;
    void Push();
};

}
