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
    startTimer(300);
    for(int i =0; i < runnerAmount; i++){
        runners.push_back(QSharedPointer<TaskRunner>(new TaskRunner()));
        connect(runners.last().data(), &TaskRunner::finished, this, &CommandController::OnTaskFinished);
    }

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

    std::lock_guard<std::recursive_mutex> guard(lock);
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
    else if(chain.hasFullParseCommand && activeFullParseCommand)
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
        if(chain.hasFullParseCommand)
            activeFullParseCommand = true;
        if(chain.hasParseCommand)
            activeParseCommand = true;
        runner->AddTask(chain);
    }
}

QSharedPointer<TaskRunner> CommandController::FetchFreeRunner(ECommandParseRequirement parseType)
{
    if((parseType == cpr_full && activeFullParseCommand)
            || (parseType == cpr_quick && activeParseCommand) )
        return nullptr;

    for(auto runner: runners)
        if(!runner->busy)
            return runner;
    return nullptr;
}

void CommandController::OnTaskFinished()
{
    QLOG_TRACE() << "task chain finished";
    std::lock_guard<std::recursive_mutex> guard(lock);

    auto senderTask = dynamic_cast<TaskRunner*>(sender());
    auto result = senderTask->result;
    senderTask->ClearState();
    const auto& message = result.actions.first()->originalMessage;
    QList<CommandChain> newCommands;
    for(auto& command : result.actions)
    {
        command->Invoke(client);
        if(command->commandsToReemit.size() > 0){
            newCommands += command->commandsToReemit;
        }
    }
    if(activeParseCommand && result.performedParseCommand)
        activeParseCommand = false;
    if(activeFullParseCommand && result.performedFullParseCommand)
        activeFullParseCommand = false;

    auto userId = QString::fromStdString(message.author.ID.string());
    activeUsers.remove(userId);

    for(auto& command : newCommands)
        Push(command);
}

void CommandController::timerEvent(QTimerEvent *)
{
    std::lock_guard<std::recursive_mutex> guard(lock);
    QQueue<CommandChain> newQueue;
    forever{
        if(queue.size() == 0)
            break;

        for(int i = 0; i < queue.size(); i++){
            auto command = queue.front();
            queue.pop_front();
            if(activeUsers.contains(command.user->GetUuid()))
            {
                newQueue.push_back(command);
                continue;
            }

            ECommandParseRequirement cpr = ECommandParseRequirement::cpr_none;
            if(command.hasFullParseCommand)
                cpr = ECommandParseRequirement::cpr_full;
            else if(command.hasParseCommand)
                cpr = ECommandParseRequirement::cpr_quick;

            auto runner = FetchFreeRunner(cpr);
            if(!runner)
            {
                newQueue.push_back(command);
                continue;
            }
            if(cpr == ECommandParseRequirement::cpr_full)
                activeFullParseCommand = true;
            else if(cpr == ECommandParseRequirement::cpr_quick)
                activeParseCommand = true;

            runner->AddTask(command);
        }
    }
    queue=newQueue;
}

}
