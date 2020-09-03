#pragma once
#include "discord/commands.h"

#include <QSharedPointer>
#include <QSet>
#include <QQueue>

namespace discord {
class TaskRunner;
class Client;

class CommandController : public QObject{
Q_OBJECT
public:
    CommandController(QObject* parent = nullptr);
    void Init(int runnerAmount);
    void Push(CommandChain);

    QSharedPointer<TaskRunner> FetchFreeRunner();

    QSet<QString> activeUsers;
    QList<QSharedPointer<TaskRunner>> runners;
    QQueue<CommandChain> queue;
    Client* client = nullptr;
public slots:
    void OnTaskFinished();
protected:
    void timerEvent(QTimerEvent *event) override;
};

}
