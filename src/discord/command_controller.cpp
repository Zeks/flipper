#include "discord/command_controller.h"
#include "discord/task_runner.h"
#include "discord/task_environment.h"
#include "discord/client_v2.h"
#include "discord/task.h"

namespace discord {

CommandController::CommandController(QObject *parent) : QObject(parent)
{

}

void CommandController::Init(int runnerAmount)
{
    for(int i =0; i < runnerAmount; i++){
        runners.push_back(QSharedPointer<TaskRunner>(new TaskRunner()));
        connect(runners.last().data(), &TaskRunner::finished, this, &CommandController::OnTaskFinished);
    }
    startTimer(300);
}

void CommandController::Push(CommandChain chain)
{
    std::lock_guard<std::mutex> guard(lock);
    const auto& message = chain.commands.first().originalMessage;
    auto userId = QString::fromStdString(message.author.ID.string());
    if(activeUsers.contains(userId))
    {
        client->sendMessage(message.channelID, "Your previous command is still executing, please wait");
        return;
    }
    activeUsers.insert(userId);
    auto runner = FetchFreeRunner();
    if(!runner){
        queue.push_back(chain);
        return;
    }
    else{
        runner->AddTask(chain);
    }
}

QSharedPointer<TaskRunner> CommandController::FetchFreeRunner()
{
    for(auto runner: runners)
        if(!runner->busy)
            return runner;
    return nullptr;
}

void CommandController::OnTaskFinished()
{
    QLOG_TRACE() << "task chain finished";
    std::lock_guard<std::mutex> guard(lock);
    auto senderTask = dynamic_cast<TaskRunner*>(sender());
    auto result = senderTask->result;
    senderTask->ClearState();
    const auto& message = result.actions.first()->originalMessage;
    auto userId = QString::fromStdString(message.author.ID.string());
    for(auto command : result.actions)
        command->Invoke(client);
    if(activeParseCommand && result.performedParseCommand)
        activeParseCommand = false;
    activeUsers.remove(userId);
}

void CommandController::timerEvent(QTimerEvent *)
{
    std::lock_guard<std::mutex> guard(lock);
    forever{
        if(queue.size() == 0)
            break;

        auto runner = FetchFreeRunner();
        if(!runner)
            break;

        runner->AddTask(queue.front());
        queue.pop_front();
    }
}

}
