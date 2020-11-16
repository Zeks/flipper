/*Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
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

