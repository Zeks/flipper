#include "discord/command_creators.h"
#include "discord/client_v2.h"
#include "discord/discord_user.h"
#include "Interfaces/discord/users.h"
#include "include/grpc/grpc_source.h"
#include <QSettings>
namespace discord{

void CommandChain::Push(Command command)
{
    commands.push_back(command);
}

Command CommandChain::Pop()
{
    auto command = commands.last();
    commands.pop_back();
    return command;
}


CommandCreator::~CommandCreator()
{

}

CommandChain CommandCreator::ProcessInput(SleepyDiscord::Message message , bool verifyUser)
{
    if(verifyUser)
        EnsureUserExists(QString::fromStdString(message.author.ID), QString::fromStdString(message.author.username));
    if(user->secsSinceLastsEasyQuery() < 3)
    {
        nullCommand.type = Command::ct_timeout_ative;
        nullCommand.ids.push_back(3-user->secsSinceLastsEasyQuery());
        nullCommand.variantHash["reason"] = "One command can be issued each 3 seconds. Please wait %1 more seconds.";
        result.Push(nullCommand);
        return result;
    }
    matches = rx.globalMatch(QString::fromStdString(message.content));
    if(!matches.hasNext())
    {
        nullCommand.type = Command::ct_display_invalid_command;
        result.Push(nullCommand);
        return result;
    }
    return ProcessInputImpl(message);
}

void CommandCreator::EnsureUserExists(QString userId, QString userName)
{
    this->userId = userId;
    An<Users> users;
    if(!users->HasUser(userId)){
        bool inDatabase = users->LoadUser(userId);
        if(!inDatabase)
        {
            QSharedPointer<discord::User> user(new discord::User(userId, "-1", userName));
            users->userInterface->WriteUser(user);
            users->LoadUser(userId);
        }
    }
    this->user = users->GetUser(userId);
}


RecommendationsCommand::RecommendationsCommand()
{

}

CommandChain RecommendationsCommand::ProcessInput(SleepyDiscord::Message message, bool verifyUser)
{
    CommandCreator::ProcessInput(message, verifyUser);
    An<Users> users;
    auto user = users->GetUser(QString::fromStdString(message.author.ID));
    if(!user->HasActiveSet()){
        Command createRecs;
        createRecs.type = Command::ct_fill_recommendations;
        auto match = matches.next();
        createRecs.ids.push_back(match.captured(1).toUInt());
        createRecs.originalMessage = message;
        result.hasParseCommand = true;
        result.Push(createRecs);
    }
    return result;
}

RecsCreationCommand::RecsCreationCommand()
{

    rx = QRegularExpression("^!recs\\s(\\d{4,10})");
}

CommandChain RecsCreationCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    if(user->secsSinceLastsRecQuery() < 60)
    {
        nullCommand.type = Command::ct_timeout_ative;
        nullCommand.ids.push_back(60-user->secsSinceLastsEasyQuery());
        nullCommand.variantHash["reason"] = "Recommendations can only be regenerated once on 60 seconds.Please wait %1 more seconds.";
        result.Push(nullCommand);
        return result;
    }


    Command createRecs;
    createRecs.type = Command::ct_fill_recommendations;
    auto match = matches.next();
    createRecs.ids.push_back(match.captured(1).toUInt());
    createRecs.originalMessage = message;
    Command displayRecs;
    displayRecs.type = Command::ct_display_page;
    displayRecs.ids.push_back(0);
    displayRecs.originalMessage = message;
    result.Push(createRecs);
    result.Push(displayRecs);
    result.hasParseCommand = true;
    return result;
}





PageChangeCommand::PageChangeCommand()
{
    rx = QRegularExpression("^!page\\s(\\d{1,10})");
}

CommandChain PageChangeCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command command;
    command.type = Command::ct_display_page;
    auto match = matches.next();
    command.ids.push_back(match.captured(1).toUInt());
    command.originalMessage = message;
    result.Push(command);
    return result;
}

NextPageCommand::NextPageCommand()
{
    rx = QRegularExpression("^!next");
}

CommandChain NextPageCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    An<Users> users;
    auto user = users->GetUser(QString::fromStdString(message.author.ID));

    Command displayRecs;
    displayRecs.type = Command::ct_display_page;
    displayRecs.ids.push_back(user->CurrentPage()+1);
    displayRecs.originalMessage = message;
    result.Push(displayRecs);
    user->AdvancePage(1);
    return result;
}

PreviousPageCommand::PreviousPageCommand()
{
    rx = QRegularExpression("^!prev");
}

CommandChain PreviousPageCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    An<Users> users;
    auto user = users->GetUser(QString::fromStdString(message.author.ID));

    Command displayRecs;
    displayRecs.type = Command::ct_display_page;
    auto newPage = user->CurrentPage()-1 < 0 ? 0 : user->CurrentPage()-1;
    displayRecs.ids.push_back(newPage);
    displayRecs.originalMessage = message;
    result.Push(displayRecs);
    return result;
}

SetFandomCommand::SetFandomCommand()
{
    rx = QRegularExpression("^!fandom(\\s#pure){,1}(\\s#reset){,1}(\\s([A-Za-z0-9\\s]{1,30})){,1}}");
}

CommandChain SetFandomCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command filteredFandoms;
    filteredFandoms.type = Command::ct_set_fandoms;
    while(matches.hasNext()){
        auto match = matches.next();
        if(match.captured(1).size() > 0)
            filteredFandoms.variantHash["allow_crossovers"] = false;
        else
            filteredFandoms.variantHash["allow_crossovers"] = true;
        if(match.captured(2).size() > 0)
            filteredFandoms.variantHash["reset"] = true;
        filteredFandoms.variantHash["fandom"] = match.captured(3);
    }
    filteredFandoms.originalMessage = message;
    result.Push(filteredFandoms);
    Command displayRecs;
    displayRecs.type = Command::ct_display_page;
    displayRecs.ids.push_back(0);
    displayRecs.originalMessage = message;
    result.Push(displayRecs);
    return result;
}

//CommandState<IgnoreFandomCommand>::help = "!xfandom X to permanently ignore fics just from this fandom"
//                                          "\n!xfandom #full X to also ignore crossovers from this fandom,"
//                                          "\n!xfandom #reset X to unignore";
IgnoreFandomCommand::IgnoreFandomCommand()
{
    rx = QRegularExpression("^!xfandom(\\s#full){,1}(\\s#reset){,1}(\\s([A-Za-z0-9\\s]{1,30})){,1}}");
}

CommandChain IgnoreFandomCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command ignoredFandoms;
    ignoredFandoms.type = Command::ct_ignore_fandoms;
    while(matches.hasNext()){
        auto match = matches.next();
        if(match.captured(1).size() > 0)
            ignoredFandoms.variantHash["with_crossovers"] = true;
        else
            ignoredFandoms.variantHash["with_crossovers"] = false;
        if(match.captured(2).size() > 0)
            ignoredFandoms.variantHash["reset"] = true;
        ignoredFandoms.variantHash["fandom"] = match.captured(3);
    }
    ignoredFandoms.originalMessage = message;
    result.Push(ignoredFandoms);
    Command displayRecs;
    displayRecs.type = Command::ct_display_page;
    displayRecs.ids.push_back(0);
    displayRecs.originalMessage = message;
    result.Push(displayRecs);
    return result;
}

IgnoreFicCommand::IgnoreFicCommand()
{
    rx = QRegularExpression("^!xfic\\s(\\d{1,15}){1,}");
}

CommandChain IgnoreFicCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command ignoredFics;
    ignoredFics.type = Command::ct_ignore_fics;
    while(matches.hasNext()){
        auto match = matches.next();
        ignoredFics.ids.push_back(match.captured(1).toUInt());
    }
    ignoredFics.originalMessage = message;
    result.Push(ignoredFics);
    return result;
}

SetIdentityCommand::SetIdentityCommand()
{
    rx = QRegularExpression("^!me\\s(\\d{1,15})");
}

CommandChain SetIdentityCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command command;
    command.type = Command::ct_set_identity;
    auto match = matches.next();
    command.ids.push_back(match.captured(1).toUInt());
    command.originalMessage = message;
    result.Push(command);
    return result;
}

CommandChain CommandParser::Execute(SleepyDiscord::Message message)
{
    bool firstCommand = true;
    CommandChain result;
    for(auto& processor: commandProcessors)
    {
        result += processor->ProcessInput(message, firstCommand);
        if(firstCommand)
            firstCommand = false;
    }
    return result;
}

DisplayHelpCommand::DisplayHelpCommand()
{
    rx = QRegularExpression("^!help");
}

CommandChain DisplayHelpCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command command;
    command.type = Command::ct_display_help;
    command.originalMessage = message;
    result.Push(command);
    return result;
}

void SendMessageCommand::Invoke(Client * client)
{
    if(embed.empty())
        client->sendMessage(originalMessage.channelID, text.toStdString());
    else
        client->sendMessage(originalMessage.channelID, text.toStdString(), embed);
}




}
