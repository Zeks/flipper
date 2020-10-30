#pragma once
#include "discord/command_generators.h"

#include <QSharedPointer>
#include <QSet>
#include <QQueue>
#include <mutex>
#include <deque>

namespace discord {
class TaskRunner;
class Client;


enum ECommandParseRequirement{
    cpr_none = 0,
    cpr_quick = 1,
    cpr_full = 2,
};

class CommandController : public QObject{
Q_OBJECT
public:
    CommandController(QObject* parent = nullptr);
    void Init(int runnerAmount);
    void Push(CommandChain &&);

    QSharedPointer<TaskRunner> FetchFreeRunner(ECommandParseRequirement = cpr_none);

    bool activeParseCommand = false;
    bool activeFullParseCommand = false;
    QSet<QString> activeUsers;
    QList<QSharedPointer<TaskRunner>> runners;
    std::deque<CommandChain> queue;
    Client* client = nullptr;
    std::recursive_mutex lock;
public slots:
    void OnTaskFinished();
protected:
    void timerEvent(QTimerEvent *event) override;
};

}

