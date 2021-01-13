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
#include "discord/actions.h"
#include "discord/task_runner.h"
#include "discord/task_environment.h"
#include "discord/client_v2.h"
#include <third_party/nanobench/nanobench.h>

namespace discord {

TaskRunner::TaskRunner(QObject *parent) : QThread(parent), environment(new TaskEnvironment())
{
    environment->Init();
}

void TaskRunner::AddTask(CommandChain&& chain)
{
    busy = true;
    chainToRun = std::move(chain);
    start();
}

void TaskRunner::ClearState()
{
    result.Clear();
    chainToRun = {};
    busy = false;
}

void TaskRunner::run()
{
    if(chainToRun.hasParseCommand)
        result.performedParseCommand = true;
    if(chainToRun.hasFullParseCommand)
        result.performedFullParseCommand = true;
    for(auto&& command : chainToRun.commands){
        auto action = GetAction(command.type);
        auto actionResult = action->Execute(environment, std::move(command));
        result.Push(actionResult);
        if(actionResult->stopChain)
            break;
        if(chainToRun.delayMs > 0)
            QThread::msleep(chainToRun.delayMs);
    }
}

}
