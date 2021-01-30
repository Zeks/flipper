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
#include "discord/command_generators.h"
#include "discord/client_v2.h"
#include "discord/discord_user.h"
#include "discord/help_generator.h"
#include "discord/discord_server.h"
#include "Interfaces/discord/users.h"
#include "include/grpc/grpc_source.h"
#include "logger/QsLog.h"
#include "discord/type_functions.h"
#include "discord/discord_message_token.h"
#include "discord/tracked-messages/tracked_roll.h"
#include "discord/tracked-messages/tracked_help_page.h"
#include "discord/tracked-messages/tracked_similarity_list.h"
#include "discord/tracked-messages/tracked_recommendation_list.h"
#include "discord/client_storage.h"
#include "discord/send_message_command.h"
#include "GlobalHeaders/snippets_templates.h"
#include <stdexcept>

#include <QSettings>
namespace discord{

bool CheckAdminRole(Client* client, QSharedPointer<Server> server, const SleepyDiscord::Snowflake<SleepyDiscord::User>& authorID)
{
    SleepyDiscord::Server sleepyServer = client->getServer(server->GetServerId());
    const auto& member = client->getMember(server->GetServerId(), authorID).cast();
    bool isAdmin = false;
    auto roles = member.roles;
    for(auto& roleId : roles){
        auto role = sleepyServer.findRole(roleId);
        auto permissions = role->permissions;
        if(SleepyDiscord::hasPremission(permissions, SleepyDiscord::ADMINISTRATOR))
        {
            isAdmin = true;
            break;
        }
    }
    return isAdmin;
}



std::atomic<uint64_t> CommandCreator::ownerId;
QSharedPointer<User> CommandCreator::user;

CommandChain CommandCreator::ProcessInput(Client* client, QSharedPointer<discord::Server> server, const SleepyDiscord::Message& message , bool )
{
    result.Reset();
    this->client = client;
    this->server = server;
    return ProcessInputImpl(message);
}

void CommandCreator::AddFilterCommand(Command&& command)
{
    if(currentOperationRestoresActiveSet)
    {
        result.PushFront(std::move(command));
        currentOperationRestoresActiveSet = false;
    }
    else
        result.Push(std::move(command));
}

void CommandCreator::EnsureUserExists(QString userId, QString userName)
{
    An<Users> users;
    if(!users->HasUser(userId)){
        bool inDatabase = users->LoadUser(userId);
        if(!inDatabase)
        {
            QSharedPointer<discord::User> user(new discord::User(userId, QStringLiteral("-1"), userName, QUuid::createUuid().toString()));
            An<interfaces::Users> usersInterface;
            usersInterface->WriteUser(user);
            users->LoadUser(userId);
        }
    }
}

bool CommandCreator::CheckAdminRole(const SleepyDiscord::Message &message)
{
    SleepyDiscord::Server sleepyServer = client->getServer(this->server->GetServerId());
    const auto& member = client->getMember(this->server->GetServerId(), message.author.ID).cast();
    bool isAdmin = false;
    auto roles = member.roles;
    for(auto& roleId : roles){
        auto role = sleepyServer.findRole(roleId);
        auto permissions = role->permissions;
        if(SleepyDiscord::hasPremission(permissions, SleepyDiscord::ADMINISTRATOR))
        {
            isAdmin = true;
            break;
        }
    }
    return isAdmin;
}


Command NewCommand(QSharedPointer<discord::Server> server, const SleepyDiscord::Message &message, ECommandType type){
    Command command;
    command.originalMessageToken = message;
    command.server = server;
    command.type = type;
    return command;
}

Command NewCommand(QSharedPointer<discord::Server> server, const MessageIdToken& message, ECommandType type){
    Command command;
    command.originalMessageToken = message;
    command.server = server;
    command.type = type;
    return command;
}


CommandChain RecommendationsCommand::ProcessInput(Client* client, QSharedPointer<discord::Server> server, const SleepyDiscord::Message& message, bool)
{
    result.Reset();
    this->client = client;
    this->server = server;

    An<Users> users;
    auto user = users->GetUser(QString::fromStdString(message.author.ID));
    if(!user->GetImpersonatedId().isEmpty()){
        users->LoadUser(user->GetImpersonatedId());
        auto impersonatedtUser = users->GetUser(user->GetImpersonatedId());
        if(impersonatedtUser)
            user = impersonatedtUser;
        else
            return {};
    }
    if(!user->HasActiveSet()){
        if(user->FfnID().isEmpty() || user->FfnID() == QStringLiteral("-1"))
        {
            Command createRecs = NewCommand(server, message,ct_no_user_ffn);
            result.Push(std::move(createRecs));
            result.stopExecution = true;
            return std::move(result);
        }
        currentOperationRestoresActiveSet = true;

        Command createRecs = NewCommand(server, message,ct_fill_recommendations);
        createRecs.ids.push_back(user->FfnID().toInt());
        createRecs.variantHash[QStringLiteral("refresh")] = QStringLiteral("yes");
        createRecs.variantHash[QStringLiteral("keep_page")] = QStringLiteral("yes");
        createRecs.textForPreExecution = QString(QStringLiteral("Restoring recommendations for user %1 into an active set, please wait a bit")).arg(user->FfnID());
        result.hasParseCommand = true;
        result.Push(std::move(createRecs));
    }
    return ProcessInputImpl(message);
}

bool RecommendationsCommand::IsThisCommand(const std::string &)
{
    return false; //cmd == TypeStringHolder<RecommendationsCommand>::name;
}

CommandChain RecsCreationCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    static constexpr int minRecsInterval = 60;
    if(user->secsSinceLastsRecQuery() < minRecsInterval && message.author.ID.string() != std::to_string(ownerId))
    {
        Command nullCommand = NewCommand(server, message,ct_timeout_active);
        nullCommand.ids.push_back(minRecsInterval-user->secsSinceLastsEasyQuery());
        nullCommand.variantHash[QStringLiteral("reason")] = QStringLiteral("Recommendations can only be regenerated once in 60 seconds.Please wait %1 more seconds.");
        nullCommand.originalMessageToken = message;
        nullCommand.server = this->server;
        result.Push(std::move(nullCommand));
        return std::move(result);
    }

    auto match = matchCommand<RecsCreationCommand>(message.content);
    auto refresh = match.get<1>().to_string();
    auto id = match.get<2>().to_string();

    Command createRecs = NewCommand(server, message,ct_fill_recommendations);
    if(id.length() == 0){
        if(!user->FfnID().isEmpty()){
            if(message.content.find("FFN_ID") != std::string::npos){
                Command nullCommand = NewCommand(server, message,ct_null_command);
                nullCommand.variantHash[QStringLiteral("reason")] = QStringLiteral("You need to replace FFN_ID with your own ID on fanfiction.net.");
                nullCommand.originalMessageToken = message;
                nullCommand.server = this->server;
                result.Push(std::move(nullCommand));
                return std::move(result);
            }
            Command createRecs = NewCommand(server, message,ct_fill_recommendations);
            createRecs.ids.push_back(user->FfnID().toInt());
            createRecs.textForPreExecution = QString(QStringLiteral("Creating recommendations for ffn user %1. Please wait, depending on your list size, it might take a while.")).arg(user->FfnID());
            if(refresh.length() == 0)
                createRecs.variantHash[QStringLiteral("refresh")] = true;

            result.Push(std::move(createRecs));
            Command command = NewCommand(server, message,ct_display_page);
            command.ids.push_back(user->CurrentRecommendationsPage());
            result.Push(std::move(command));
            user->SetSimilarFicsId(0);
            return std::move(result);
        }
        else
        {
            Command nullCommand = NewCommand(server, message,ct_null_command);
            nullCommand.variantHash[QStringLiteral("reason")] = QStringLiteral("Not a valid ID or user url.");
            nullCommand.originalMessageToken = message;
            nullCommand.server = this->server;
            result.Push(std::move(nullCommand));
            return std::move(result);
        }
    }
    user->SetSimilarFicsId(0);

    bool isNumConvertible = true;
    try
    {
        std::stoi(id);
    }
    catch (const std::invalid_argument& e)
    {
        isNumConvertible = false;
    }
    catch (const std::out_of_range& e)
    {
        isNumConvertible = false;
    }
    if(isNumConvertible)
        createRecs.ids.push_back(std::stoi(id));
    else
    {
        QString temp = QString::fromStdString(id);
        if(temp.contains("fanfiction"))
            createRecs.variantHash[QStringLiteral("url")] = QString::fromStdString(id);
        else
            createRecs.variantHash[QStringLiteral("user")] = QString::fromStdString(id);
    }

    if(refresh.length() == 0)
    {
        createRecs.variantHash[QStringLiteral("refresh")] = true;
    }
    else{
        Command command = NewCommand(server, message,ct_force_list_params);
        command.variantHash[QStringLiteral("min")] = 0;
        command.variantHash[QStringLiteral("ratio")] = 0;
        result.Push(std::move(command));
    }
    auto prefix = QString::fromStdString(std::string(server->GetCommandPrefix()));
    QString newFicspart = QString("\nIf you have recently added new fics to your favourites, do `%1recs >refresh %2` instead, to update your recommendations.\n"
                                  "Note that fanfiction.net takes approximately 30 minutes to update your page after you add more favs. Refreshing is pointless before that happens.");
    if(refresh.length() != 0)
        newFicspart = "";
    newFicspart=newFicspart.arg(prefix,QString::fromStdString(match.get<2>().to_string()));
    createRecs.textForPreExecution = QString(QStringLiteral("Creating recommendations for ffn user %1. Please wait, depending on your list size, it might take a while.")).arg(QString::fromStdString(match.get<2>().to_string()));
    createRecs.textForPreExecution+=newFicspart;
    Command displayRecs = NewCommand(server, message,ct_display_page);
    displayRecs.ids.push_back(0);
    result.Push(std::move(createRecs));
    result.Push(std::move(displayRecs));
    result.hasParseCommand = true;
    return std::move(result);
}

bool RecsCreationCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<RecsCreationCommand>::name;
}

CommandChain PageChangeCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command command = NewCommand(server, message,ct_display_page);
    auto match = matchCommand<PageChangeCommand>(message.content);
    if(match.get<1>().to_string().length() > 0)
    {
        command.ids.push_back(std::stoi(match.get<1>().to_string()));
        result.Push(std::move(command));
    }
    else
    {
        command.ids.push_back(user->CurrentRecommendationsPage());
        result.Push(std::move(command));
    }
    return std::move(result);
}

bool PageChangeCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<PageChangeCommand>::name;
}

CommandChain NextPageCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    An<Users> users;
    auto user = users->GetUser(QString::fromStdString(message.author.ID));

    Command displayRecs = NewCommand(server, message,ct_display_page);
    displayRecs.ids.push_back(user->CurrentRecommendationsPage()+1);
    result.Push(std::move(displayRecs));
    user->AdvancePage(1);
    return std::move(result);
}

bool NextPageCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<NextPageCommand>::name;
}

CommandChain PreviousPageCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    An<Users> users;
    auto user = users->GetUser(QString::fromStdString(message.author.ID));

    Command displayRecs = NewCommand(server, message,ct_display_page);
    auto newPage = user->CurrentRecommendationsPage()-1 < 0 ? 0 : user->CurrentRecommendationsPage()-1;
    displayRecs.ids.push_back(newPage);
    result.Push(std::move(displayRecs));
    return std::move(result);
}

bool PreviousPageCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<PreviousPageCommand>::name;
}

CommandChain SetFandomCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command filteredFandoms = NewCommand(server, message,ct_set_fandoms);
    auto match = matchCommand<SetFandomCommand>(message.content);
    auto pure = match.get<1>().to_string();
    auto reset = match.get<2>().to_string();
    auto fandom = match.get<3>().to_string();

    if(pure.length() > 0)
        filteredFandoms.variantHash[QStringLiteral("allow_crossovers")] = false;
    else
        filteredFandoms.variantHash[QStringLiteral("allow_crossovers")] = true;
    if(reset.length() > 0)
        filteredFandoms.variantHash[QStringLiteral("reset")] = true;
    filteredFandoms.variantHash[QStringLiteral("fandom")] = QString::fromStdString(fandom).trimmed();

    AddFilterCommand(std::move(filteredFandoms));
    Command displayRecs = NewCommand(server, message,user->GetLastPageType());
    displayRecs.ids.push_back(0);
    displayRecs.variantHash[QStringLiteral("refresh_previous")] = true;
    result.Push(std::move(displayRecs));
    return std::move(result);
}

bool SetFandomCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<SetFandomCommand>::name;
}


CommandChain IgnoreFandomCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command ignoredFandoms = NewCommand(server, message,ct_ignore_fandoms);

    auto match = matchCommand<IgnoreFandomCommand>(message.content);
    auto full = match.get<1>().to_string();
    auto reset = match.get<2>().to_string();
    auto fandom = match.get<3>().to_string();

    if(full.length() > 0)
        ignoredFandoms.variantHash[QStringLiteral("with_crossovers")] = true;
    else{
        ignoredFandoms.variantHash[QStringLiteral("with_crossovers")] = false;
        ignoredFandoms.textForPreExecution = "If you want to also ignore crossovers, repeat the command with >full added before the fandom";
    }
    if(reset.length() > 0)
        ignoredFandoms.variantHash[QStringLiteral("reset")] = true;
    ignoredFandoms.variantHash[QStringLiteral("fandom")] = QString::fromStdString(fandom).trimmed();

    AddFilterCommand(std::move(ignoredFandoms));

    //    Command createRecs = NewCommand(server, message,ct_fill_recommendations);
    //    createRecs.ids.push_back(user->FfnID().toUInt());
    //    createRecs.variantHash[QStringLiteral("refresh")] = true;
    //    result.Push(std::move(createRecs));

    Command displayRecs = NewCommand(server, message,user->GetLastPageType());
    displayRecs.ids.push_back(0);
    displayRecs.variantHash[QStringLiteral("refresh_previous")] = true;
    result.Push(std::move(displayRecs));
    return std::move(result);
}

bool IgnoreFandomCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<IgnoreFandomCommand>::name;
}

CommandChain IgnoreFicCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command ignoredFics = NewCommand(server, message,ct_ignore_fics);
    auto match = ctre::search<TypeStringHolder<IgnoreFicCommand>::patternCommand>(message.content);
    auto full = match.get<1>().to_string();
    if(full.length() > 0){
        ignoredFics.variantHash[QStringLiteral("everything")] = true;
        ignoredFics.ids.clear();
    }
    else{
        for(auto match : ctre::range<TypeStringHolder<IgnoreFicCommand>::patternNum>(message.content)){
            auto silentStr = match.get<1>().to_string();
            auto numbers = QString::fromStdString(match.get<2>().to_string()).split(QStringLiteral(" "));
            for(const auto& number : numbers){
                auto id = number.toInt();
                if(id != 0){
                    ignoredFics.ids.push_back(id);
                }
            }
        }
    }
    bool hasIgnores = ignoredFics.ids.size() > 0 || ignoredFics.variantHash.size() > 0;
    AddFilterCommand(std::move(ignoredFics));
    if(hasIgnores)
    {
        Command displayRecs = NewCommand(server, message,user->GetLastPageType());
        displayRecs.ids.push_back(user->CurrentRecommendationsPage());
        displayRecs.variantHash[QStringLiteral("refresh_previous")] = true;
        result.Push(std::move(displayRecs));

    }
    else{
        Command nullCommand = NewCommand(server, message,ct_null_command);
        nullCommand.type = ct_null_command;
        nullCommand.variantHash[QStringLiteral("reason")] = QStringLiteral("xfic command needs IDs of fics *in the displayed list* separated with whitespaces.");
        nullCommand.originalMessageToken = message;
        nullCommand.server = this->server;
        result.Push(std::move(nullCommand));
    }
    return std::move(result);
}

bool IgnoreFicCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<IgnoreFicCommand>::name;
}

CommandChain SetIdentityCommand::ProcessInputImpl(const SleepyDiscord::Message&)
{
    //    Command command;
    //    command.type = ct_set_identity;
    //    auto match = matches.next();
    //    command.ids.push_back(match.captured(1).toUInt());
    //    command.originalMessage = message;
    //    result.Push(command);
    return std::move(result);
}

bool SetIdentityCommand::IsThisCommand(const std::string &)
{
    return false;//cmd == TypeStringHolder<SetIdentityCommand>::name;
}

CommandChain DisplayHelpCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command command = NewCommand(server, message,ct_display_help);

    auto match = ctre::search<TypeStringHolder<DisplayHelpCommand>::pattern>(message.content);
    auto helpTarget = match.get<2>().to_string();
    auto view = std::string_view(helpTarget);
    auto prefix = server->GetCommandPrefix();
    if(view.substr(0, prefix.length()) == prefix)
        view.remove_prefix(server->GetCommandPrefix().length());
    if(view.length() > 0)
        command.ids.push_back(GetHelpPage(view));
    else
        command.ids.push_back(0);
    result.Push(std::move(command));
    return std::move(result);
}

bool DisplayHelpCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<DisplayHelpCommand>::name;
}


CommandChain RngCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command command = NewCommand(server, message,ct_display_rng);
    auto match = ctre::search<TypeStringHolder<RngCommand>::pattern>(message.content);
    command.variantHash[QStringLiteral("quality")] = QString::fromStdString(match.get<1>().to_string()).trimmed();
    result.Push(std::move(command));
    user->SetSimilarFicsId(0);
    return std::move(result);
}

bool RngCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<RngCommand>::name;
}

CommandChain ChangeServerPrefixCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    SleepyDiscord::Server sleepyServer = client->getServer(this->server->GetServerId());
    bool isAdmin = CheckAdminRole(message);
    if(isAdmin || sleepyServer.ownerID == message.author.ID ||  message.author.ID.string() == std::to_string(ownerId))
    {
        Command command = NewCommand(server, message,ct_change_server_prefix);
        auto match = ctre::search<TypeStringHolder<ChangeServerPrefixCommand>::pattern>(message.content);
        auto newPrefix = QString::fromStdString(match.get<1>().to_string()).trimmed();
        thread_local QRegularExpression invalidSymbolsRx("[^a-z0-9!~.&#$%:]");
        auto invalidSymbols = invalidSymbolsRx.match(newPrefix);
        if(invalidSymbols.hasMatch()){
            command.type = ct_null_command;
            command.textForPreExecution = QStringLiteral("prefix cannot contain symbols other than lowercase english letters, numbers and `!~.&#$%:`");
            result.Push(std::move(command));
            return std::move(result);
        }
        if(newPrefix.isEmpty())
        {
            command.type = ct_null_command;
            command.textForPreExecution = QStringLiteral("prefix cannot be empty");
            result.Push(std::move(command));
            return std::move(result);
        }
        if(newPrefix.length() > 10)
        {
            command.type = ct_null_command;
            command.textForPreExecution = QStringLiteral("prefix cannot be longer than 10 characters");
            result.Push(std::move(command));
            return std::move(result);
        }
        command.variantHash[QStringLiteral("prefix")] = newPrefix;
        command.textForPreExecution = QStringLiteral("Changing prefix for this server to: ") + newPrefix;
        result.Push(std::move(command));
    }
    else
    {
        Command command = NewCommand(server, message,ct_insufficient_permissions);
        result.Push(std::move(command));
    }
    return std::move(result);
}

bool ChangeServerPrefixCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ChangeServerPrefixCommand>::name;
}

CommandChain ChangePermittedChannelCommand::ProcessInputImpl(const SleepyDiscord::Message & message)
{
    if(this->server->GetServerId().length() == 0)
        return std::move(result);

    SleepyDiscord::Server sleepyServer = client->getServer(this->server->GetServerId());
    bool isAdmin = CheckAdminRole(message);
    if(isAdmin || sleepyServer.ownerID == message.author.ID ||  message.author.ID.string() == std::to_string(ownerId))
    {
        Command command = NewCommand(server, message,ct_set_permitted_channel);
        command.variantHash[QStringLiteral("channel")] = QString::fromStdString(message.channelID.string());
        result.Push(std::move(command));
    }
    else
    {
        Command command = NewCommand(server, message,ct_insufficient_permissions);
        result.Push(std::move(command));
    }
    return std::move(result);
}

bool ChangePermittedChannelCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ChangePermittedChannelCommand>::name;
}

CommandChain FilterLikedAuthorsCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command command = NewCommand(server, message,ct_filter_liked_authors);
    auto match = ctre::search<TypeStringHolder<FilterLikedAuthorsCommand>::pattern>(message.content);
    if(match)
    {
        command.variantHash[QStringLiteral("liked")] = true;
        AddFilterCommand(std::move(command));
        Command displayRecs = NewCommand(server, message,user->GetLastPageType());
        displayRecs.ids.push_back(0);
        displayRecs.variantHash[QStringLiteral("refresh_previous")] = true;
        result.Push(std::move(displayRecs));
    }
    return std::move(result);
}

bool FilterLikedAuthorsCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<FilterLikedAuthorsCommand>::name;
}

CommandChain ShowFreshRecsCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command command = NewCommand(server, message,ct_filter_fresh);
    auto match = ctre::search<TypeStringHolder<ShowFreshRecsCommand>::pattern>(message.content);
    auto strict = match.get<1>().to_string();
    if(strict.length() > 0)
        command.variantHash[QStringLiteral("strict")] = true;
    AddFilterCommand(std::move(command));
    //user->SetSimilarFicsId(0);

    Command displayRecs = NewCommand(server, message,ct_display_page);
    displayRecs.ids.push_back(0);
    displayRecs.variantHash[QStringLiteral("refresh_previous")] = true;
    result.Push(std::move(displayRecs));
    return std::move(result);
}

bool ShowFreshRecsCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ShowFreshRecsCommand>::name;
}

CommandChain ShowCompletedCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command command = NewCommand(server, message,ct_filter_complete);
    AddFilterCommand(std::move(command));

    Command displayRecs = NewCommand(server, message,user->GetLastPageType());
    displayRecs.ids.push_back(0);
    displayRecs.variantHash[QStringLiteral("refresh_previous")] = true;
    result.Push(std::move(displayRecs));
    return std::move(result);
}

bool ShowCompletedCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ShowCompletedCommand>::name;
}

CommandChain HideDeadCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command command = NewCommand(server, message,ct_filter_out_dead);
    auto match = ctre::search<TypeStringHolder<HideDeadCommand>::pattern>(message.content);
    auto strict = match.get<1>().to_string();
    if(strict.length() > 0)
    {
        auto number = std::stoi(match.get<1>().to_string());
        if(number > 0)
        {
            static const int reasonableDeadLimits = 36500;
            // 100 years should be enough, lmao
            if(number > reasonableDeadLimits)
                number = reasonableDeadLimits;
            command.variantHash[QStringLiteral("days")] = number;
        }
        else
        {
            Command nullCommand = NewCommand(server, message,ct_null_command);
            nullCommand.type = ct_null_command;
            nullCommand.variantHash[QStringLiteral("reason")] = QStringLiteral("Number of days must be greater than 0");
            nullCommand.originalMessageToken = message;
            nullCommand.server = this->server;
            result.Push(std::move(nullCommand));
            return std::move(result);
        }
    }

    AddFilterCommand(std::move(command));

    Command displayRecs = NewCommand(server, message,user->GetLastPageType());
    displayRecs.ids.push_back(0);
    displayRecs.variantHash[QStringLiteral("refresh_previous")] = true;
    result.Push(std::move(displayRecs));
    return std::move(result);
}

bool HideDeadCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<HideDeadCommand>::name;
}

CommandChain PurgeCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command command = NewCommand(server, message,ct_purge);
    result.Push(std::move(command));
    return std::move(result);
}

bool PurgeCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<PurgeCommand>::name;
}


CommandChain ResetFiltersCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command command = NewCommand(server, message,ct_reset_filters);
    AddFilterCommand(std::move(command));
    Command displayRecs = NewCommand(server, message,user->GetLastPageType());
    displayRecs.ids.push_back(0);
    displayRecs.variantHash[QStringLiteral("refresh_previous")] = true;
    result.Push(std::move(displayRecs));
    return std::move(result);
}

bool ResetFiltersCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ResetFiltersCommand>::name;
}



CommandChain CreateRollCommand(QSharedPointer<User> user, QSharedPointer<Server> server, const MessageIdToken& message){
    CommandChain result;
    Command command = NewCommand(server, message,ct_display_rng);
    command.variantHash[QStringLiteral("quality")] = user->GetLastUsedRoll();
    command.targetMessage = message.messageID;
    command.reactedMessageToken = message;
    command.reactionCommand = true;
    command.user = user;
    result.Push(std::move(command));
    return result;
}

CommandChain CreateSimilarListCommand(QSharedPointer<User> user, QSharedPointer<Server> server, const MessageIdToken & message, int ficId)
{
    CommandChain result;
    Command command = NewCommand(server, message,ct_create_similar_fics_list);
    command.targetMessage = message.messageID;
    command.user = user;
    user->SetSimilarFicsId(ficId);
    command.ids.push_back(ficId);
    result.Push(std::move(command));
    Command displayRecs = NewCommand(server, message,ct_display_page);
    displayRecs.ids.push_back(0);
    displayRecs.user = user;
    displayRecs.variantHash["similar"] = QString::number(ficId);
    if(server->GetDedicatedChannelId().length() > 0)
        displayRecs.targetChannelID = server->GetDedicatedChannelId();
    result.Push(std::move(displayRecs));
    return result;
}

CommandChain CreateRemoveBotMessageCommand(Client* client ,QSharedPointer<User> user, QSharedPointer<Server> server, const MessageIdToken & message)
{
    CommandChain result;

    auto pushNullCommand = [&](const QString& reason){
        Command nullCommand = NewCommand(server, message,ct_null_command);
        nullCommand.type = ct_null_command;
        nullCommand.variantHash[QStringLiteral("reason")] = reason;
        nullCommand.originalMessageToken = message;
        nullCommand.server = server;
        nullCommand.user = user;
        result.Push(std::move(nullCommand));
    };
    An<ClientStorage> storage;
    Command command = NewCommand(server, message, ct_delete_bot_message);
    command.user = user;

    bool isAdmin = CheckAdminRole(client, server, message.authorID);
    //if(isAdmin || sleepyServer.ownerID == message.author.ID ||  message.author.ID.string() == std::to_string(ownerId))

    bool sameUser = storage->messageData.value(message.messageID.number())->originalUser->UserID() != user->UserID();
    if(!sameUser && !isAdmin)
    {
        pushNullCommand(QString::fromStdString(user->CreateMention() + ", you need to be the original user or the server admin to request the deletion."));
        return result;
    }
    command.targetMessage = message.messageID;
    result.Push(std::move(command));
    return result;
}


CommandChain CreateChangeRecommendationsPageCommand(QSharedPointer<User> user, QSharedPointer<Server> server, const MessageIdToken& message, bool shiftRight)
{
    CommandChain result;
    An<ClientStorage> storage;
    if(!storage->messageData.contains(message.messageID.number()))
        return result;

    auto listData = std::dynamic_pointer_cast<TrackedRecommendationList>(storage->messageData.value(message.messageID.number()));


    Command command = NewCommand(server, message,ct_display_page);
    if(shiftRight)
        command.ids.push_back(listData->memo.page + 1);
    else if(listData->memo.page != 0)
        command.ids.push_back(listData->memo.page - 1);
    else
        return result;

    command.targetMessage = message.messageID;
    command.reactedMessageToken = message;
    command.reactionCommand = true;
    command.user = user;
    result.Push(std::move(command));
    return result;
}

CommandChain CreateRemoveReactionCommand(QSharedPointer<User> user, QSharedPointer<Server> server, const MessageIdToken & message, const std::string& reaction)
{
    CommandChain result;
    Command command = NewCommand(server, message,ct_remove_reactions);
    command.variantHash["to_remove"]=QStringList{QString::fromStdString(reaction)};

    command.targetMessage = message.messageID;
    command.user = user;
    result.Push(std::move(command));
    return result;
}



CommandChain CreateChangeHelpPageCommand(QSharedPointer<User> user, QSharedPointer<Server> server, const MessageIdToken& message, bool shiftRight)
{
    An<ClientStorage> storage;
    CommandChain result;
    if(!storage->messageData.contains(message.messageID.number()))
        return result;

    auto helpData = std::dynamic_pointer_cast<TrackedHelpPage>(storage->messageData.value(message.messageID.number()));

    Command command = NewCommand(server, message,ct_display_help);
    int maxHelpPages = static_cast<int>(discord::EHelpPages::last_help_page) + 1;
    int newPage = 0;
    if(shiftRight)
    {
        newPage = helpData->currenHelpPage + 1;
        if(newPage == maxHelpPages)
            newPage = 0;
    }
    else {
        newPage = helpData->currenHelpPage - 1;
        if(newPage < 0)
            newPage = maxHelpPages - 1;

    }
    command.ids.push_back(newPage);
    command.targetMessage = message.messageID;
    command.user = user;
    command.reactionCommand = true;
    command.reactedMessageToken = message;
    result.Push(std::move(command));
    return result;
}

CommandChain SimilarFicsCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    CommandChain result;
    Command command = NewCommand(server, message,ct_create_similar_fics_list);
    auto match = matchCommand<SimilarFicsCommand>(message.content);
    auto ficId = match.get<1>().to_string();
    if(ficId.length() == 0)
    {
        if(user->GetSimilarFicsId() != 0){
            user->SetSimilarFicsId(0);

            Command createRecs = NewCommand(server, message,ct_fill_recommendations);
            createRecs.ids.push_back(user->FfnID().toUInt());
            createRecs.variantHash[QStringLiteral("refresh")] = true;
            createRecs.textForPreExecution = QString(QStringLiteral("Creating recommendations for ffn user %1. Please wait, depending on your list size, it might take a while.")).arg(user->FfnID());
            result.Push(std::move(createRecs));
            Command displayRecs = NewCommand(server, message,ct_display_page);
            displayRecs.ids.push_back(0);
            result.Push(std::move(displayRecs));
            return result;
        }
    }
    //user->SetSimilarFicsId(std::stoi(match.get<1>().to_string()));
    command.ids.push_back(std::stoi(match.get<1>().to_string()));
    command.variantHash["similar"] = QString::fromStdString(match.get<1>().to_string());
    result.Push(std::move(command));
    Command displayRecs = NewCommand(server, message,ct_display_page);
    displayRecs.ids.push_back(0);
    displayRecs.variantHash["similar"] = QString::fromStdString(match.get<1>().to_string());
    result.Push(std::move(displayRecs));
    return result;

}

bool SimilarFicsCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<SimilarFicsCommand>::name;
}


CommandChain WordcountCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    Command command = NewCommand(server, message,ct_set_wordcount_limit);
    auto match = ctre::search<TypeStringHolder<WordcountCommand>::pattern>(message.content);
    auto filterType = match.get<2>().to_string();
    auto rangeBegin = match.get<3>().to_string();
    auto rangeEnd = match.get<4>().to_string();

    if(filterType.length() == 0)
        filterType = "reset";

    auto pushNullCommand = [&](const QString& reason){
        Command nullCommand = NewCommand(server, message,ct_null_command);
        nullCommand.type = ct_null_command;
        nullCommand.variantHash[QStringLiteral("reason")] = reason;
        nullCommand.originalMessageToken = message;
        nullCommand.server = this->server;
        result.Push(std::move(nullCommand));
    };
    if((filterType == "less" || filterType == "more") && rangeBegin.length() == 0)
    {
        pushNullCommand(QStringLiteral("`less` and `more` commands require a desired wordcount after them."));
        return std::move(result);
    }
    if(filterType == "between" && (rangeBegin.length() == 0 || rangeEnd.length() == 0))
    {
        pushNullCommand(QStringLiteral("`between` command requires both being and end of the desied wordcount range."));
        return std::move(result);
    }

    if(filterType == "less"){
        command.ids.push_back(0);
        command.ids.push_back(std::stoi(rangeBegin));
    }
    else if(filterType == "more")
    {
        command.ids.push_back(std::stoi(rangeBegin));
        command.ids.push_back(std::numeric_limits<int>::max());
    }
    else if(filterType == "between"){
        command.ids.push_back(std::stoi(rangeBegin));
        command.ids.push_back(std::stoi(rangeEnd));
    }
    else{
        command.ids.push_back(0);
        command.ids.push_back(0);
    }
    command.variantHash["filter_type"] = QString::fromStdString(filterType);
    AddFilterCommand(std::move(command));
    Command displayRecs = NewCommand(server, message,ct_display_page);
    displayRecs.variantHash[QStringLiteral("refresh_previous")] = true;
    displayRecs.ids.push_back(0);
    result.Push(std::move(displayRecs));
    return std::move(result);
}



bool WordcountCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<WordcountCommand>::name;
}

CommandChain GemsCommand::ProcessInputImpl(const SleepyDiscord::Message &message)
{
    Command command = NewCommand(server, message,ct_show_gems);
    AddFilterCommand(std::move(command));
    //user->SetSimilarFicsId(0);

    Command displayRecs = NewCommand(server, message,ct_display_page);
    displayRecs.ids.push_back(0);
    displayRecs.variantHash[QStringLiteral("refresh_previous")] = true;
    result.Push(std::move(displayRecs));
    return std::move(result);
}

bool GemsCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<GemsCommand>::name;
}

CommandChain CutoffCommand::ProcessInputImpl(const SleepyDiscord::Message & message)
{
    Command command = NewCommand(server, message,ct_set_cutoff);
    auto match = matchCommand<CutoffCommand>(message.content);
    if(match.get<1>().to_string().length() > 0)
    {
        command.variantHash["cutoff"] = QString::fromStdString(match.get<1>().to_string());
        result.Push(std::move(command));
    }
    else
    {
        result.Push(std::move(command));
    }

    Command createRecs = NewCommand(server, message,ct_fill_recommendations);
    createRecs.ids.push_back(user->FfnID().toUInt());
    createRecs.variantHash[QStringLiteral("refresh")] = true;
    createRecs.textForPreExecution = QString(QStringLiteral("Creating recommendations for ffn user %1. Please wait, depending on your list size, it might take a while.")).arg(user->FfnID());
    result.Push(std::move(createRecs));
    Command displayRecs = NewCommand(server, message,ct_display_page);
    displayRecs.ids.push_back(0);
    result.Push(std::move(displayRecs));
    return std::move(result);
}

bool CutoffCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<CutoffCommand>::name;
}


CommandChain YearCommand::ProcessInputImpl(const SleepyDiscord::Message & message)
{
    Command command = NewCommand(server, message,ct_set_year);
    auto match = matchCommand<YearCommand>(message.content);
    // have the type detected
    if(match.get<1>().to_string().length() > 0)
    {
        // have the year detected
        if(match.get<4>().to_string().length() > 0){
            command.variantHash["type"] = QString::fromStdString(match.get<1>().to_string());
            command.variantHash["year"] = QString::fromStdString(match.get<4>().to_string()).trimmed();
            result.Push(std::move(command));
        }
        else{
            // this wil lswitch mode or disable, in action code
            command.variantHash["type"] = QString::fromStdString(match.get<1>().to_string());
            result.Push(std::move(command));
        }
    }
    else
    {
        // this will just diable the mode
        result.Push(std::move(command));
    }

    //    Command createRecs = NewCommand(server, message,ct_fill_recommendations);
    //    createRecs.ids.push_back(user->FfnID().toUInt());
    //    createRecs.variantHash[QStringLiteral("refresh")] = true;
    //    createRecs.textForPreExecution = QString(QStringLiteral("Creating recommendations for ffn user %1. Please wait, depending on your list size, it might take a while.")).arg(user->FfnID());
    //    result.Push(std::move(createRecs));
    Command displayRecs = NewCommand(server, message,ct_display_page);
    displayRecs.ids.push_back(0);
    result.Push(std::move(displayRecs));
    return std::move(result);
}

bool YearCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<YearCommand>::name;
}


CommandChain ShowCommand::ProcessInputImpl(const SleepyDiscord::Message & message)
{

    //auto match = ctre::search<TypeStringHolder<ShowCommand>::pattern>(message.content);
    QList<uint64_t> ids;
    static const int ficDisplayLimit = 10;
    int counter = 1;

    for(auto match : ctre::range<TypeStringHolder<ShowCommand>::pattern>(message.content)){
        if(counter > ficDisplayLimit)
            break;
        auto id = match.get<0>().to_string();
        id = trim_copy(id);
        ids.push_back(std::stoll(id));
        counter++;
    }
    if(ids.size() > 0)
    {
        for(auto id: ids){
            Command queuedFics = NewCommand(server, message,ct_show_fic);
            queuedFics.ids.push_back(id);
            result.Push(std::move(queuedFics));
        }
        result.delayMs = 200;
    }
    else{
        Command nullCommand = NewCommand(server, message,ct_null_command);
        nullCommand.type = ct_null_command;
        nullCommand.variantHash[QStringLiteral("reason")] = QStringLiteral("`show` command requites fic ids after it.");
        nullCommand.originalMessageToken = message;
        nullCommand.server = this->server;
        result.Push(std::move(nullCommand));
    }
    return std::move(result);
}

bool ShowCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ShowCommand>::name;
}


CommandChain ChangeTargetCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    if(message.author.ID.string() != std::to_string(ownerId))
        return std::move(result);

    Command command = NewCommand(server, message,ct_set_target_channel);
    auto match = ctre::search<TypeStringHolder<ChangeTargetCommand>::pattern>(message.content);
    auto channel = match.get<1>().to_string();
    if(channel.length() == 0){
        command.variantHash["channel"] = QString::fromStdString(channel);
        result.Push(std::move(command));
        return std::move(result);
    }
    if(channel == "1"){
        command.variantHash["channel"] = QString::fromStdString(message.channelID.string());
        result.Push(std::move(command));
        return std::move(result);
    }
    command.variantHash["channel"] = QString::fromStdString(channel);
    result.Push(std::move(command));
    return std::move(result);
}



bool ChangeTargetCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ChangeTargetCommand>::name;
}


CommandChain SendMessageToChannelCommand::ProcessInputImpl(const SleepyDiscord::Message& message)
{
    if(message.author.ID.string() != std::to_string(ownerId))
        return std::move(result);
    Command command = NewCommand(server, message,ct_send_to_channel);
    auto match = ctre::search<TypeStringHolder<SendMessageToChannelCommand>::pattern>(message.content);
    auto messageText = match.get<1>().to_string();

    if(messageText.length() == 0)
    {
        auto pushNullCommand = [&](const QString& reason){
            Command nullCommand = NewCommand(server, message,ct_null_command);
            nullCommand.type = ct_null_command;
            nullCommand.variantHash[QStringLiteral("reason")] = reason;
            nullCommand.originalMessageToken = message;
            nullCommand.server = this->server;
            result.Push(std::move(nullCommand));
        };
        pushNullCommand("No message to send");
        return std::move(result);
    }
    command.variantHash["messageText"] = QString::fromStdString(messageText);
    result.Push(std::move(command));
    return std::move(result);
}



bool SendMessageToChannelCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<SendMessageToChannelCommand>::name;
}

CommandChain ToggleBanCommand::ProcessInputImpl(const SleepyDiscord::Message & message)
{
    if(message.author.ID.string() != std::to_string(ownerId))
        return std::move(result);
    Command command = NewCommand(server, message,ct_toggle_ban);
    auto match = ctre::search<TypeStringHolder<ToggleBanCommand>::pattern>(message.content);
    auto type = match.get<1>().to_string();
    auto id = match.get<2>().to_string();

    if(type.length() == 0 || id.length() == 0)
    {
        auto pushNullCommand = [&](const QString& reason){
            Command nullCommand = NewCommand(server, message,ct_null_command);
            nullCommand.type = ct_null_command;
            nullCommand.variantHash[QStringLiteral("reason")] = reason;
            nullCommand.originalMessageToken = message;
            nullCommand.server = this->server;
            result.Push(std::move(nullCommand));
        };
        pushNullCommand("Either type or ID is empty.");
        return std::move(result);
    }
    command.variantHash["type"] = QString::fromStdString(type);
    command.variantHash["id"] = QString::fromStdString(id);
    result.Push(std::move(command));
    return std::move(result);
}

bool ToggleBanCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ToggleBanCommand>::name;
}

CommandChain SusCommand::ProcessInputImpl(const SleepyDiscord::Message & message)
{
    if(message.author.ID.string() != std::to_string(ownerId))
        return std::move(result);
    Command command = NewCommand(server, message,ct_sus_user);
    auto match = matchCommand<SusCommand>(message.content);
    if(match.get<1>().to_string().length() > 0)
    {
        command.variantHash["sus"] = QString::fromStdString(match.get<1>().to_string());
        result.Push(std::move(command));
    }
    else
    {
        result.Push(std::move(command));
    }
    return std::move(result);
}

bool SusCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<SusCommand>::name;
}



CommandChain ReviewCommand::ProcessInputImpl(const SleepyDiscord::Message & message)
{
    auto createNullCommand = [&](QString reason){
        Command nullCommand = NewCommand(server, message,ct_null_command);
        nullCommand.type = ct_null_command;
        nullCommand.variantHash[QStringLiteral("reason")] = reason;
        nullCommand.originalMessageToken = message;
        nullCommand.server = this->server;
        result.Push(std::move(nullCommand));
    };

    auto match = ctre::search<TypeStringHolder<ReviewCommand>::pattern>(message.content);
    auto mode = match.get<1>().to_string();
    std::string identifier,sign,score,text;
    if(mode *in("delete", "remove")){
        identifier = match.get<2>().to_string();
        if(identifier.length() == 0 ){
            createNullCommand("You need to provide an identifier you want to delete");
            return std::move(result);
        }
        Command command = NewCommand(server, message, ct_delete_review);
        command.variantHash["identifier"] = QString::fromStdString(identifier);
        result.Push(std::move(command));

    }else{
        identifier = match.get<4>().to_string();
        std::string id;
        static constexpr std::string_view pattern = "(\\d{1,20}+)/";
        for(auto rangeMatch : ctre::range<pattern>(identifier)){
            id = rangeMatch.get<1>().to_string();
            break;
        }
        sign = match.get<6>().to_string();
        score = match.get<7>().to_string();
        text = match.get<8>().to_string();
        if(identifier.length() == 0 || score.length() == 0 || text.length() == 0){
            if(identifier.length() == 0)
                createNullCommand("You need to provide a link you are reviewing");
            else if(score.length() == 0)
                createNullCommand("You need to provide a score for the fic (it can range between -5 and +5)");
            else if(text.length() == 0)
                createNullCommand("You need to provide a review for the fic");
            return std::move(result);
        }
        Command command = NewCommand(server, message, ct_add_review);
        command.variantHash["identifier"] = QString::fromStdString(identifier);
        command.variantHash["review"] = QString::fromStdString(text);
        command.variantHash["id"] = QString::fromStdString(id);
        int intScore = QString::fromStdString(score).toInt();
        command.variantHash["score"] = sign == "-" ? -1*intScore : intScore;
        result.Push(std::move(command));
    }
    return std::move(result);
}

bool ReviewCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<ReviewCommand>::name;
}


CommandChain StatsCommand::ProcessInputImpl(const SleepyDiscord::Message & message)
{
    if(message.author.ID.string() != std::to_string(ownerId))
        return std::move(result);
    try{
        auto servers = client->getServers();
        client->sendMessage(message.channelID, "Bot is on: " + QString::number(servers.list().size()).toStdString() + " servers");
    }
    catch (const SleepyDiscord::ErrorCode& error){
        QLOG_INFO() << "Discord error:" << error;
    }
    return std::move(result);
}

bool StatsCommand::IsThisCommand(const std::string &cmd)
{
    return cmd == TypeStringHolder<StatsCommand>::name;
}












}



