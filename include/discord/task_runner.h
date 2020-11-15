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
#include "discord/actions.h"
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
    void AddTask(CommandChain &&chain);
    void ClearState();

    virtual void run() override;

    CommandChain chainToRun;
    QSharedPointer<TaskEnvironment> environment;
    ActionChain result;
    std::atomic<bool> busy = false;

};


}
