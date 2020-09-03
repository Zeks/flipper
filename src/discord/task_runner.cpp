#include "discord/task_runner.h"
#include "discord/task_environment.h"
namespace discord {

TaskRunner::TaskRunner(QObject *parent) : QThread(parent)
{
    environment.reset(new TaskEnvironment());
    environment->Init();
}

void TaskRunner::AddTask(CommandChain chain)
{
    busy = true;
    chainToRun = chain;
    start();
}

void TaskRunner::ClearState()
{
    result.clear();
    busy = false;
}

void TaskRunner::run()
{

}



}
