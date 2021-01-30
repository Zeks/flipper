#include "discord/command_parser.h"
#include "discord/client_v2.h"
namespace discord {

CommandChain CommandParser::Execute(const std::string& command, QSharedPointer<discord::Server> server, const SleepyDiscord::Message& message)
{
    std::lock_guard<std::mutex> guard(lock);
    bool firstCommand = true;
    CommandChain result;

    CommandCreator::EnsureUserExists(QString::fromStdString(message.author.ID), QString::fromStdString(message.author.username));
    An<Users> users;
    auto user = users->GetUser(QString::fromStdString(message.author.ID.string()));
    if(!user->GetImpersonatedId().isEmpty()){
        users->LoadUser(user->GetImpersonatedId());
        auto impersonatedtUser = users->GetUser(user->GetImpersonatedId());
        if(impersonatedtUser)
            user = impersonatedtUser;
        else
            return {};
    }
    if(user && user->GetBanned() ){
        if(!user->GetShownBannedMessage()){
            user->SetShownBannedMessage(true);
            Command command = NewCommand(server, message,ct_timeout_active);
            command.ids.push_back(3-user->secsSinceLastsEasyQuery());
            command.variantHash[QStringLiteral("reason")] = QStringLiteral("You have been banned from using the bot.\n"
                                                                           "The most likely reason for this is some kind of malicious misuse interfering with requests from other people.\n"
                                                                           "If you want to appeal the ban, join: https://discord.gg/AdDvX5H");
            command.user = user;
            result.Push(std::move(command));
            result.stopExecution = true;
            return result;
        }
        return {};
    }

    if(user->secsSinceLastsEasyQuery() < 3)
    {
        if(user->GetTimeoutWarningShown())
            return result;
        user->SetTimeoutWarningShown(true);
        Command command = NewCommand(server, message,ct_timeout_active);
        command.ids.push_back(3-user->secsSinceLastsEasyQuery());
        command.variantHash[QStringLiteral("reason")] = QStringLiteral("One command can be issued each 3 seconds. Please wait %1 more seconds.");
        command.user = user;
        result.Push(std::move(command));
        result.stopExecution = true;
        return result;
    }
    user->SetTimeoutWarningShown(false);

    for(auto& processor: commandProcessors)
    {
        if(!processor->IsThisCommand(command))
            continue;

        processor->user = user;
        auto newCommands = processor->ProcessInput(client, server, message, firstCommand);
        result += newCommands;
        if(newCommands.hasParseCommand)
            result.hasParseCommand = true;
        if(newCommands.stopExecution == true)
            break;
        if(firstCommand)
            firstCommand = false;
        break;
    }
    result.AddUserToCommands(user);
    for(const auto& command: std::as_const(result.commands)){
        if(!command.textForPreExecution.isEmpty())
            client->sendMessageWrapper(command.originalMessageToken.channelID, command.originalMessageToken.serverID,command.textForPreExecution.toStdString());
    }
    return result;
}

}
