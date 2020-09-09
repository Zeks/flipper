#include "discord/command_creators.h"
#include "discord/client_v2.h"
#include "discord/discord_user.h"
#include "Interfaces/discord/users.h"
#include "include/grpc/grpc_source.h"
#include "logger/QsLog.h"
#include <QSettings>
namespace discord{
QSharedPointer<User> CommandCreator::user;
void CommandChain::Push(Command command)
{
    commands.push_back(command);
}

void CommandChain::AddUserToCommands(QSharedPointer<User> user)
{
    for(auto& command : commands)
        command.user = user;
}

Command CommandChain::Pop()
{
    auto command = commands.last();
    commands.pop_back();
    return command;
}

void CommandChain::RemoveEmptyCommands()
{
    commands.erase(std::remove_if(commands.begin(), commands.end(), [](const auto& item){return item.type == Command::ECommandType::ct_display_invalid_command;}));
}


CommandCreator::~CommandCreator()
{

}

CommandChain CommandCreator::ProcessInput(SleepyDiscord::Message message , bool )
{
    result.Reset();
    matches = rx.globalMatch(QString::fromStdString(message.content));
    //QLOG_INFO() << "testing pattern: " << rx.pattern();
    if(!matches.hasNext())
        return result;
    return ProcessInputImpl(message);
}

void CommandCreator::EnsureUserExists(QString userId, QString userName)
{
    An<Users> users;
    if(!users->HasUser(userId)){
        bool inDatabase = users->LoadUser(userId);
        if(!inDatabase)
        {
            QSharedPointer<discord::User> user(new discord::User(userId, "-1", userName));
            An<interfaces::Users> usersInterface;
            usersInterface->WriteUser(user);
            users->LoadUser(userId);
        }
    }
}


RecommendationsCommand::RecommendationsCommand()
{

}

CommandChain RecommendationsCommand::ProcessInput(SleepyDiscord::Message message, bool)
{
    result.Reset();
    matches = rx.globalMatch(QString::fromStdString(message.content));
    //QLOG_INFO() << "checking pattern: " << rx.pattern();
    if(!matches.hasNext())
        return result;

    An<Users> users;
    auto user = users->GetUser(QString::fromStdString(message.author.ID));
    if(!user->HasActiveSet()){
        if(user->FfnID().isEmpty() || user->FfnID() == "-1")
        {
            Command createRecs;
            createRecs.type = Command::ct_no_user_ffn;
            createRecs.originalMessage = message;
            result.Push(createRecs);
            result.stopExecution = true;
            return result;
        }

        Command createRecs;
        createRecs.type = Command::ct_fill_recommendations;
        createRecs.ids.push_back(user->FfnID().toInt());
        createRecs.originalMessage = message;
        createRecs.textForPreExecution = QString("Restoring recommendations for user %1 into an active set, please wait a bit").arg(user->FfnID());
        result.hasParseCommand = true;
        result.Push(createRecs);
    }
    return ProcessInputImpl(message);
}

RecsCreationCommand::RecsCreationCommand()
{

    rx = QRegularExpression("^!recs\\s{1,}(\\d{4,10})");
}

CommandChain RecsCreationCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    if(user->secsSinceLastsRecQuery() < 60)
    {
        nullCommand.type = Command::ct_timeout_active;
        nullCommand.ids.push_back(60-user->secsSinceLastsEasyQuery());
        nullCommand.variantHash["reason"] = "Recommendations can only be regenerated once on 60 seconds.Please wait %1 more seconds.";
        nullCommand.originalMessage = message;
        result.Push(nullCommand);
        return result;
    }

    Command createRecs;
    createRecs.type = Command::ct_fill_recommendations;
    auto match = matches.next();
    createRecs.ids.push_back(match.captured(1).toUInt());
    createRecs.originalMessage = message;
    createRecs.textForPreExecution = QString("Creating recommendations for ffn user %1. Please wait, depending on your list size, it might take a while.").arg(match.captured(1));
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
    rx = QRegularExpression("^!page\\s{1,}(\\d{1,10})");
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
    rx = QRegularExpression("^!fandom(\\s{1,}>pure){0,1}(\\s{1,}>reset){0,1}(\\s{1,}.+){0,1}");
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
    rx = QRegularExpression("^!xfandom(\\s{1,}>full){0,1}(\\s{1,}>reset){0,1}(\\s{1,}.+){0,1}");
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
    rx = QRegularExpression("^!xfic((\\s{1,}\\d{1,2}){1,10})|(\\s{1,}>all)");
}

CommandChain IgnoreFicCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command ignoredFics;
    ignoredFics.type = Command::ct_ignore_fics;
    while(matches.hasNext()){
        auto match = matches.next();
        if(!match.captured(3).isEmpty()){
            ignoredFics.variantHash["everything"] = true;
            ignoredFics.ids.clear();
            break;
        }
        auto list = match.captured(1).split(" ");
        list.removeAll("");
        for(auto id : list)
            ignoredFics.ids.push_back(id.trimmed().toUInt());
    }
    ignoredFics.originalMessage = message;
    result.Push(ignoredFics);
    return result;
}

SetIdentityCommand::SetIdentityCommand()
{
    rx = QRegularExpression("^!me\\s{1,}(\\d{1,15})");
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
    std::lock_guard<std::mutex> guard(lock);
    bool firstCommand = true;
    CommandChain result;

    CommandCreator::EnsureUserExists(QString::fromStdString(message.author.ID), QString::fromStdString(message.author.username));
    An<Users> users;
    auto user = users->GetUser(QString::fromStdString(message.author.ID.string()));
    if(user->secsSinceLastsEasyQuery() < 3)
    {
        Command command;
        command.type = Command::ct_timeout_active;
        command.ids.push_back(3-user->secsSinceLastsEasyQuery());
        command.variantHash["reason"] = "One command can be issued each 3 seconds. Please wait %1 more seconds.";
        command.originalMessage = message;
        command.user = user;
        result.Push(command);
        result.stopExecution = true;
        return result;
    }

    for(auto& processor: commandProcessors)
    {
        processor->user = user;
        auto newCommands = processor->ProcessInput(message, firstCommand);
        result += newCommands;
        if(newCommands.stopExecution == true)
            break;
        if(firstCommand)
            firstCommand = false;
    }
    result.AddUserToCommands(user);
    for(auto command: result.commands){
        if(!command.textForPreExecution.isEmpty())
            client->sendMessage(command.originalMessage.channelID, command.textForPreExecution.toStdString());
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
    {
        SleepyDiscord::Embed embed;
        if(text.length() > 0)
            client->sendMessage(originalMessage.channelID, text.toStdString(), embed);
    }
    else
        client->sendMessage(originalMessage.channelID, text.toStdString(), embed);
}

RngCommand::RngCommand()
{
    rx = QRegularExpression("^!rng\\s(perfect|good|all)");
}

CommandChain RngCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command command;
    command.type = Command::ct_display_rng;
    auto match = matches.next();
    command.variantHash["quality"] = match.captured(1);
    command.originalMessage = message;
    result.Push(command);
    return result;
}




}

