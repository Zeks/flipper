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

static std::string CreateMention(const std::string& string){
    return "<@" + string + ">";
}

void CommandController::Push(CommandChain chain)
{
    if(chain.commands.size() == 0 )
        return;
    const auto& message = chain.commands.first().originalMessage;
    auto userId = QString::fromStdString(message.author.ID.string());

    std::lock_guard<std::mutex> guard(lock);
    if(activeUsers.contains(userId))
    {
        client->sendMessage(message.channelID, CreateMention(message.author.ID.string())+ ", your previous command is still executing, please wait");
        return;
    }
    else if(chain.hasParseCommand && activeParseCommand)
    {
        client->sendMessage(message.channelID, CreateMention(message.author.ID.string()) + ", another recommendation list is being created at the moment. Putting your request into the queue, please wait a bit.");
        queue.push_back(chain);
        return;
    }
    activeUsers.insert(userId);
    auto runner = FetchFreeRunner();
    if(!runner){
        client->sendMessage(message.channelID, CreateMention(message.author.ID.string()) + ", there are no free command runners, putting your command on the queue. Your position is: " + QString::number(queue.size()).toStdString());
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
    for(auto& command : result.actions)
        command->Invoke(client);
    if(activeParseCommand && result.performedParseCommand)
        activeParseCommand = false;

    auto userId = QString::fromStdString(message.author.ID.string());
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
