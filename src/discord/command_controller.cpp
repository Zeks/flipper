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
#include "discord/command_controller.h"
#include "discord/task_runner.h"
#include "discord/task_environment.h"
#include "discord/client_v2.h"
#include "discord/send_message_command.h"
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
        connect(std::as_const(runners).last().data(), &TaskRunner::finished, this, &CommandController::OnTaskFinished);
    }

}

static std::string CreateMention(const std::string& string){
    return "<@" + string + ">";
}

void CommandController::Push(CommandChain&& chain)
{
    if(chain.commands.size() == 0 )
        return;
    const auto& message = chain.commands.front().originalMessageToken;
    auto userId = QString::fromStdString(message.authorID.string());

    std::lock_guard<std::recursive_mutex> guard(lock);
    if(activeUsers.contains(userId))
    {
        client->sendMessageWrapper(message.channelID, message.serverID, CreateMention(message.authorID.string())+ ", your previous command is still executing, please wait");
        return;
    }
    else if(activeParseCommand)
    {
        if(chain.hasParseCommand || chain.hasFullParseCommand){
            client->sendMessageWrapper(message.channelID,  message.serverID, CreateMention(message.authorID.string()) + ", another recommendation list is being created at the moment. Putting your request into the queue, please wait a bit.");
            if(chain.Size() > 0 )
                chain.user = (*chain.commands.begin()).user;
            queue.emplace_back(std::move(chain));
            return;
        }
    }
    activeUsers.insert(userId);
    auto runner = FetchFreeRunner();
    if(!runner){
        client->sendMessageWrapper(message.channelID, message.serverID, CreateMention(message.authorID.string()) + ", there are no free command runners, putting your command on the queue. Your position is: " + QString::number(queue.size()).toStdString());
        queue.emplace_back(std::move(chain));
        return;
    }
    else{
        if(chain.hasFullParseCommand)
            activeFullParseCommand = true;
        if(chain.hasParseCommand)
            activeParseCommand = true;
        runner->AddTask(std::move(chain));
    }
}

QSharedPointer<TaskRunner> CommandController::FetchFreeRunner(ECommandParseRequirement parseType)
{
    if((parseType == cpr_full && activeFullParseCommand)
            || (parseType == cpr_quick && activeParseCommand) )
        return nullptr;

    for(auto runner: std::as_const(runners))
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
    const auto& message = result.actions.first()->originalMessageToken;
    std::list<CommandChain> newCommands;
    for(auto& command : result.actions)
    {
        command->Invoke(client);
        if(command->commandsToReemit.size() > 0){
            newCommands.splice(newCommands.end(), command->commandsToReemit);
        }
    }
    if(activeParseCommand && result.performedParseCommand)
        activeParseCommand = false;
    if(activeFullParseCommand && result.performedFullParseCommand)
        activeFullParseCommand = false;

    auto userId = QString::fromStdString(message.authorID.string());
    activeUsers.remove(userId);

    for(auto&& command : newCommands)
        Push(std::move(command));
}

void CommandController::timerEvent(QTimerEvent *)
{
    std::lock_guard<std::recursive_mutex> guard(lock);
    std::deque<CommandChain> newQueue;
    forever{
        if(queue.size() == 0)
            break;

        for(size_t i = 0; i < queue.size(); i++){
            auto command = std::move(queue.front());
            queue.pop_front();
            if(activeUsers.contains(command.user->GetUuid()))
            {
                newQueue.emplace_back(std::move(command));
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
                newQueue.emplace_back(std::move(command));
                continue;
            }
            if(cpr == ECommandParseRequirement::cpr_full)
                activeFullParseCommand = true;
            else if(cpr == ECommandParseRequirement::cpr_quick)
                activeParseCommand = true;

            runner->AddTask(std::move(command));
        }
    }
    queue=std::move(newQueue);
}

}
