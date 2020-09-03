#pragma once
#include "discord/commands.h"
#include <QSet>
#include <QThread>

#include <atomic>

namespace discord {
class TaskEnvironment;
class SendMessageCommand;
class TaskRunner : public  QThread{
    Q_OBJECT
    public:
    TaskRunner(QObject *parent = nullptr);

    virtual void run() override;

    CommandChain chainToRun;
    QSharedPointer<TaskEnvironment> environment;
    QList<QSharedPointer<SendMessageCommand>> result;
    std::atomic<bool> busy = false;
};


}
