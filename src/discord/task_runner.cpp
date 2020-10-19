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
    }
}

}
