#include "discord/command_creators.h"
#include "discord/client_v2.h"
#include "discord/discord_user.h"
#include "discord/discord_server.h"
#include "Interfaces/discord/users.h"
#include "include/grpc/grpc_source.h"
#include "logger/QsLog.h"
#include "discord/type_functions.h"
#include <stdexcept>

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

CommandChain CommandCreator::ProcessInput(Client* client, QSharedPointer<discord::Server> server, SleepyDiscord::Message message , bool )
{
    result.Reset();
    this->client = client;
    this->server = server;
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

CommandChain RecommendationsCommand::ProcessInput(Client* client, QSharedPointer<discord::Server> server, SleepyDiscord::Message message, bool)
{
    result.Reset();
    this->client = client;
    this->server = server;

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
        createRecs.variantHash["refresh"] = "yes";
        createRecs.textForPreExecution = QString("Restoring recommendations for user %1 into an active set, please wait a bit").arg(user->FfnID());
        result.hasParseCommand = true;
        result.Push(createRecs);
    }
    return ProcessInputImpl(message);
}

bool RecommendationsCommand::IsThisCommand(const std::string &)
{
    return false; //cmd == TypeStringHolder<RecommendationsCommand>::name;
}

RecsCreationCommand::RecsCreationCommand()
{
}

CommandChain RecsCreationCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    QSettings settings("settings/settings_discord.ini", QSettings::IniFormat);
    auto ownerId = settings.value("Login/ownerId").toString().toStdString();

    if(user->secsSinceLastsRecQuery() < 60 && message.author.ID.string() != ownerId)
    {
        nullCommand.type = Command::ct_timeout_active;
        nullCommand.ids.push_back(60-user->secsSinceLastsEasyQuery());
        nullCommand.variantHash["reason"] = "Recommendations can only be regenerated once on 60 seconds.Please wait %1 more seconds.";
        nullCommand.originalMessage = message;
        result.Push(nullCommand);
        return result;
    }
    auto match = matchCommand<RecsCreationCommand>(message.content);
    Command createRecs;
    createRecs.type = Command::ct_fill_recommendations;
    auto id = match.get<1>().to_string();
    if(id.length() == 0){
        createRecs.textForPreExecution = QString("Not a valid ID.");
        createRecs.type = Command::ct_null_command;
        return result;
    }
    createRecs.ids.push_back(std::stoi(match.get<1>().to_string()));
    createRecs.originalMessage = message;
    createRecs.textForPreExecution = QString("Creating recommendations for ffn user %1. Please wait, depending on your list size, it might take a while.").arg(QString::fromStdString(match.get<1>().to_string()));
    Command displayRecs;
    displayRecs.type = Command::ct_display_page;
    displayRecs.ids.push_back(0);
    displayRecs.originalMessage = message;
    result.Push(createRecs);
    result.Push(displayRecs);
    result.hasParseCommand = true;
    return result;
}

bool RecsCreationCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<RecsCreationCommand>::name;
}





PageChangeCommand::PageChangeCommand()
{
}

CommandChain PageChangeCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command command;
    command.type = Command::ct_display_page;
    auto match = matchCommand<PageChangeCommand>(message.content);
    command.ids.push_back(std::stoi(match.get<1>().to_string()));
    command.originalMessage = message;
    result.Push(command);
    return result;
}

bool PageChangeCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<PageChangeCommand>::name;
}

NextPageCommand::NextPageCommand()
{
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

bool NextPageCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<NextPageCommand>::name;
}

PreviousPageCommand::PreviousPageCommand()
{
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

bool PreviousPageCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<PreviousPageCommand>::name;
}

SetFandomCommand::SetFandomCommand()
{
}

CommandChain SetFandomCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command filteredFandoms;
    filteredFandoms.type = Command::ct_set_fandoms;
    auto match = matchCommand<SetFandomCommand>(message.content);
    auto pure = match.get<1>().to_string();
    auto reset = match.get<2>().to_string();
    auto fandom = match.get<3>().to_string();

    if(pure.length() > 0)
        filteredFandoms.variantHash["allow_crossovers"] = false;
    else
        filteredFandoms.variantHash["allow_crossovers"] = true;
    if(reset.length() > 0)
        filteredFandoms.variantHash["reset"] = true;
    filteredFandoms.variantHash["fandom"] = QString::fromStdString(fandom).trimmed();

    filteredFandoms.originalMessage = message;
    result.Push(filteredFandoms);
    Command displayRecs;
    displayRecs.type = Command::ct_display_page;
    displayRecs.ids.push_back(0);
    displayRecs.originalMessage = message;
    result.Push(displayRecs);
    return result;
}

bool SetFandomCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<SetFandomCommand>::name;
}

IgnoreFandomCommand::IgnoreFandomCommand()
{
}

CommandChain IgnoreFandomCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command ignoredFandoms;
    ignoredFandoms.type = Command::ct_ignore_fandoms;


    auto match = matchCommand<IgnoreFandomCommand>(message.content);
    auto full = match.get<1>().to_string();
    auto reset = match.get<2>().to_string();
    auto fandom = match.get<3>().to_string();

    if(full.length() > 0)
        ignoredFandoms.variantHash["with_crossovers"] = true;
    else
        ignoredFandoms.variantHash["with_crossovers"] = false;
    if(reset.length() > 0)
        ignoredFandoms.variantHash["reset"] = true;
    ignoredFandoms.variantHash["fandom"] = QString::fromStdString(fandom).trimmed();
    ignoredFandoms.originalMessage = message;
    result.Push(ignoredFandoms);
    Command displayRecs;
    displayRecs.type = Command::ct_display_page;
    displayRecs.ids.push_back(0);
    displayRecs.originalMessage = message;
    result.Push(displayRecs);
    return result;
}

bool IgnoreFandomCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<IgnoreFandomCommand>::name;
}

IgnoreFicCommand::IgnoreFicCommand()
{
}

CommandChain IgnoreFicCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command ignoredFics;
    ignoredFics.type = Command::ct_ignore_fics;
    auto match = ctre::search<TypeStringHolder<IgnoreFicCommand>::patternCommand>(message.content);
    auto full = match.get<1>().to_string();
    bool silent = false;
    if(full.length() > 0){
        ignoredFics.variantHash["everything"] = true;
        ignoredFics.ids.clear();
    }
    else{
        for(auto match : ctre::range<TypeStringHolder<IgnoreFicCommand>::patternNum>(message.content)){
            auto silentStr = match.get<1>().to_string();
            if(silentStr.length() != 0)
                silent = true;
            auto numbers = QString::fromStdString(match.get<2>().to_string()).split(" ");
            for(auto number : numbers){
                auto id = number.toInt();
                if(number != 0){
                    ignoredFics.ids.push_back(id);
                }
            }

        }
    }

    ignoredFics.originalMessage = message;
    result.Push(ignoredFics);
    if(!silent && (ignoredFics.variantHash.size() > 0 || ignoredFics.ids.size() > 0))
    {
        Command displayRecs;
        displayRecs.type = Command::ct_display_page;
        displayRecs.ids.push_back(0);
        displayRecs.originalMessage = message;
        result.Push(displayRecs);

    }
    return result;
}

bool IgnoreFicCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<IgnoreFicCommand>::name;
}

SetIdentityCommand::SetIdentityCommand()
{
}

CommandChain SetIdentityCommand::ProcessInputImpl(SleepyDiscord::Message )
{
//    Command command;
//    command.type = Command::ct_set_identity;
//    auto match = matches.next();
//    command.ids.push_back(match.captured(1).toUInt());
//    command.originalMessage = message;
//    result.Push(command);
    return result;
}

bool SetIdentityCommand::IsThisCommand(const std::string &)
{
    return false;//cmd == TypeStringHolder<SetIdentityCommand>::name;
}

CommandChain CommandParser::Execute(std::string command, QSharedPointer<discord::Server> server,SleepyDiscord::Message message)
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
        if(!processor->IsThisCommand(command))
            continue;

        processor->user = user;
        auto newCommands = processor->ProcessInput(client, server, message, firstCommand);
        result += newCommands;
        if(newCommands.stopExecution == true)
            break;
        if(firstCommand)
            firstCommand = false;
        break;
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
}

CommandChain DisplayHelpCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command command;
    command.type = Command::ct_display_help;
    command.originalMessage = message;
    result.Push(command);
    return result;
}

bool DisplayHelpCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<DisplayHelpCommand>::name;
}

void SendMessageCommand::Invoke(Client * client)
{

    try{
    if(embed.empty())
    {
        SleepyDiscord::Embed embed;
        if(text.length() > 0)
            client->sendMessage(originalMessage.channelID, text.toStdString(), embed);
    }
    else
        client->sendMessage(originalMessage.channelID, text.toStdString(), embed);
    }
    catch (const SleepyDiscord::ErrorCode& error){
        QLOG_INFO() << error;
        QLOG_INFO() << QString::fromStdString(this->embed.description);
    }
}

RngCommand::RngCommand()
{
}

CommandChain RngCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command command;
    command.type = Command::ct_display_rng;
    auto match = ctre::search<TypeStringHolder<RngCommand>::pattern>(message.content);
    command.variantHash["quality"] = QString::fromStdString(match.get<1>().to_string()).trimmed();
    command.originalMessage = message;
    result.Push(command);
    return result;
}

bool RngCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<RngCommand>::name;
}

ChangeServerPrefixCommand::ChangeServerPrefixCommand()
{
}

CommandChain ChangeServerPrefixCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    SleepyDiscord::Server sleepyServer = client->getServer(this->server->GetServerId());
    //std::list<SleepyDiscord::ServerMember>::iterator member = sleepyServer.findMember(message.author.ID);
    const auto& member = client->getMember(this->server->GetServerId(), message.author.ID).cast();
    bool isAdmin = false;
    auto roles = member.roles;
    for(auto& roleId : roles){
        std::list<SleepyDiscord::Role>::iterator role = sleepyServer.findRole(roleId);
        auto permissions = role->permissions;
        if(SleepyDiscord::hasPremission(permissions, SleepyDiscord::ADMINISTRATOR))
        {
            isAdmin = true;
            break;
        }
    }
    if(isAdmin || sleepyServer.ownerID == message.author.ID)
    {
        Command command;
        command.type = Command::ct_change_server_prefix;
        command.originalMessage = message;
        auto match = ctre::search<TypeStringHolder<ChangeServerPrefixCommand>::pattern>(message.content);
        auto newPrefix = QString::fromStdString(match.get<1>().to_string()).trimmed();
        if(newPrefix.isEmpty())
        {
            command.type = Command::ct_null_command;
            command.textForPreExecution = "prefix cannot be empty";
            command.server = this->server;
            result.Push(command);
            return result;
        }
        command.variantHash["prefix"] = newPrefix;
        command.textForPreExecution = "Changing prefix for this server to: " + newPrefix;
        command.server = this->server;
        result.Push(command);
    }
    else
    {
        Command command;
        command.type = Command::ct_insufficient_permissions;
        command.originalMessage = message;
        command.server = this->server;
        result.Push(command);
    }
    return result;
}

bool ChangeServerPrefixCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ChangeServerPrefixCommand>::name;
}

ForceListParamsCommand::ForceListParamsCommand()
{

}

CommandChain ForceListParamsCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command command;
    command.type = Command::ct_force_list_params;
    auto match = ctre::search<TypeStringHolder<ForceListParamsCommand>::pattern>(message.content);
    auto min = match.get<1>().to_string();
    auto ratio = match.get<2>().to_string();
    if(min.length() == 0 || ratio.length() == 0)
        return result;

    An<Users> users;
    auto user = users->GetUser(QString::fromStdString(message.author.ID));
    if(user->FfnID().isEmpty() || user->FfnID() == "-1")
    {
        Command createRecs;
        createRecs.type = Command::ct_no_user_ffn;
        createRecs.originalMessage = message;
        result.Push(createRecs);
        result.stopExecution = true;
        return result;
    }

    command.variantHash["min"] = std::stoi(min);
    command.variantHash["ratio"] = std::stoi(ratio);
    command.originalMessage = message;
    result.Push(command);

    Command createRecs;
    createRecs.type = Command::ct_fill_recommendations;
    createRecs.ids.push_back(user->FfnID().toInt());
    createRecs.originalMessage = message;
    createRecs.variantHash["refresh"] = "yes";
    createRecs.textForPreExecution = QString("Recreating recommendations for user %1 with new settings, please wait a bit").arg(user->FfnID());
    result.hasParseCommand = true;
    result.Push(createRecs);
    Command displayRecs;
    displayRecs.type = Command::ct_display_page;
    displayRecs.ids.push_back(0);
    displayRecs.originalMessage = message;
    result.Push(displayRecs);
    return result;
}

bool ForceListParamsCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ForceListParamsCommand>::name;
}

FilterLikedAuthorsCommand::FilterLikedAuthorsCommand()
{

}

CommandChain FilterLikedAuthorsCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command command;
    command.type = Command::ct_filter_liked_authors;
    auto match = ctre::search<TypeStringHolder<FilterLikedAuthorsCommand>::pattern>(message.content);
    if(match)
    {
        command.variantHash["liked"] = true;
        command.originalMessage = message;
        result.Push(command);
        Command displayRecs;
        displayRecs.type = Command::ct_display_page;
        displayRecs.ids.push_back(0);
        displayRecs.originalMessage = message;
        result.Push(displayRecs);
    }
    return result;
}

bool FilterLikedAuthorsCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<FilterLikedAuthorsCommand>::name;
}

CommandChain ShowFullFavouritesCommand::ProcessInputImpl(SleepyDiscord::Message message)
{
    Command command;
    command.type = Command::ct_show_favs;
    auto match = ctre::search<TypeStringHolder<ShowFullFavouritesCommand>::pattern>(message.content);
    if(match)
    {
        command.originalMessage = message;
        result.Push(command);
    }
    return result;
}

bool ShowFullFavouritesCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ShowFullFavouritesCommand>::name;
}




}

