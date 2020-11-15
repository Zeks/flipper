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
#include "discord/client_v2.h"
#include "discord/command_controller.h"
#include "discord/command_generators.h"
#include "discord/command.h"
#include "discord/actions.h"
#include "discord/discord_server.h"
#include "discord/discord_init.h"
#include "discord/type_functions.h"
#include "discord/cached_message_source.h"
#include "discord/db_vendor.h"
#include "sql/discord/discord_queries.h"
#include "logger/QsLog.h"
#include "GlobalHeaders/snippets_templates.h"
#include <QRegularExpression>
#include <string_view>
#include <QSharedPointer>
#include <QSettings>
//#include "third_party/str_concat.h"
#include "third_party/ctre.hpp"

class rapidjson_exception : public std::runtime_error
{
public:
rapidjson_exception() : std::runtime_error("json schema invalid") {}
};
#define RAPIDJSON_ASSERT(x)  if(x); else throw rapidjson_exception();

namespace discord {
std::atomic<bool> Client::allowMessages = true;

std::atomic<int64_t> Client::mirrorTargetChannel = 0;
std::atomic<int64_t> Client::mirrorSourceChannel = 0;
std::atomic<int64_t> Client::botPmChannel = 0;

Client::Client(const std::string token, const char numOfThreads, QObject *obj):QObject(obj),
    SleepyDiscord::DiscordClient(token, numOfThreads)
{
    qRegisterMetaType<QSharedPointer<core::RecommendationListFicData>>("QSharedPointer<core::RecommendationListFicData>");
    parser.reset(new CommandParser);
    parser->client = this;

}

Client::Client(QObject *obj):QObject(obj), SleepyDiscord::DiscordClient()
{
    qRegisterMetaType<QSharedPointer<core::RecommendationListFicData>>("QSharedPointer<core::RecommendationListFicData>");
    parser.reset(new CommandParser);
    parser->client = this;
}

void Client::InitClient()
{
    fictionalDMServer.reset(new discord::Server());
    discord::InitDefaultCommandSet(this->parser);
    std::vector<SleepyDiscord::Server>  sleepyServers = getServers();
    for(const auto& server : sleepyServers){
        InitDiscordServerIfNecessary(server.ID);
    }
    InitTips();

    QSettings settings(QStringLiteral("settings/settings_discord.ini"), QSettings::IniFormat);
    auto ownerId = settings.value(QStringLiteral("Login/ownerId")).toString().toULongLong();
    CommandCreator::ownerId = ownerId;
    Client::mirrorSourceChannel = 0;
    Client::mirrorTargetChannel = settings.value(QStringLiteral("Login/text_to")).toULongLong();
    Client::botPmChannel = settings.value(QStringLiteral("Login/pm_to")).toULongLong();

    actionableEmoji = {"🔁","👈","👉"};
}

QSharedPointer<discord::Server> Client::InitDiscordServerIfNecessary(SleepyDiscord::Snowflake<SleepyDiscord::Server> serverId)
{
    An<discord::Servers> servers;
    if(!servers->HasServer(serverId)){
        auto inDatabase= servers->LoadServer(serverId);
        if(!inDatabase)
        {
            auto sleepyServer = getServer(serverId);
            QSharedPointer<discord::Server> server(new discord::Server());
            server->SetServerId(serverId);
            server->SetServerName(QString::fromStdString(sleepyServer.cast().name));
            auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
            database::discord_queries::WriteServer(dbToken->db, server);
            servers->LoadServer(serverId);
        }
    }
    return servers->GetServer(serverId);
}


void Client::InitCommandExecutor()
{
    executor.reset(new CommandController());
    executor->client = this;
}


QSharedPointer<Server> Client::GetServerInstanceForChannel(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID)
{
    QSharedPointer<discord::Server> server;
    if(serverID.string().length() == 0)
        server = fictionalDMServer;
    else
        server = GetServerInstanceForChannel(channelID,serverID.number());

    return server;
}

QSharedPointer<Server> Client::GetServerInstanceForChannel(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, uint64_t serverID)
{
    QSharedPointer<discord::Server> server;
    if(serverID == 0)
        server = fictionalDMServer;
    else if(nonPmChannels.contains(channelID.number())){
        server = InitDiscordServerIfNecessary(serverID);
    }
    else{
        auto channel = getChannel(channelID).cast();
        if(channel.type != SleepyDiscord::Channel::DM){
            nonPmChannels.push(channelID.number());
            server = InitDiscordServerIfNecessary(channel.serverID);
        }
        else
            server = fictionalDMServer;
    }
    return server;
}

static constexpr auto pattern = discord::GetSimplePatternChecker();
constexpr auto matchSimple(std::string_view sv) noexcept {
    return ctre::match<pattern>(sv);
}

void Client::onMessage(SleepyDiscord::Message message) {
    try{
        Log(message);

        if(message.author.bot || !message.content.size())
            return;

        QSharedPointer<discord::Server> server = GetServerInstanceForChannel(message.channelID, message.serverID.string());
        if(server->GetBanned()){
            if(!server->GetShownBannedMessage()){
                server->SetShownBannedMessage(true);
                sendMessageWrapper(message.channelID, message.serverID, "This server has been banned from using the bot.\n"
                                                                        "The most likely reason for this happening is malicious misuse by its members.\n"
                                                                        "If you want to appeal the ban, join: https://discord.gg/AdDvX5H");
            }
            return;
        }

        std::string_view sv (message.content);

        const auto commandPrefix = server->GetCommandPrefix();
        if(sv == botPrefixRequest)
            sendMessageWrapper(message.channelID, message.serverID, "Prefix for this server is: " + std::string(commandPrefix));

        if(sv.substr(0, commandPrefix.length()) != commandPrefix)
            return;

        if(message.content.length() > 500){
            sendMessageWrapper(message.channelID, message.serverID, "Your command is too long. Perhaps you've mistyped accidentally?");
            return;
        }

        sv.remove_prefix(commandPrefix.length());

        auto result = ctre::search<pattern>(sv);;
        if(!result.matched())
            return;

        bool priorityCommand = message.content.substr(server->GetCommandPrefix().length(), 6) *in("permit", "target") || sv.substr(0, 4) == "send";
        if(!priorityCommand && server->GetServerId().length() > 0 && server->GetDedicatedChannelId().length() > 0 && message.channelID.string() != server->GetDedicatedChannelId())
            return;

        auto commands = parser->Execute(result.get<0>().to_string(), server, message);
        if(commands.Size() == 0)
            return;

        // instantiating channel -> server pairing if necessary to avoid hitting the api in onReaction needlessly
        if(message.serverID.string().length() > 0 && !channelToServerHash.contains(message.channelID.number())){
            channelToServerHash.push(message.channelID.number(), message.serverID.number());
        }

        executor->Push(std::move(commands));
    }
    catch(const rapidjson_exception& e){
        qDebug() << e.what();
    }
}

static std::string CreateMention(const std::string& string){
    return "<@" + string + ">";
}


void Client::onReaction(SleepyDiscord::Snowflake<SleepyDiscord::User> userID, SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, SleepyDiscord::Snowflake<SleepyDiscord::Message> messageID, SleepyDiscord::Emoji emoji){
    try{
        if(userID == getID())
            return;
        if(!actionableEmoji.contains(emoji.name))
            return;
        if(!messageSourceAndTypeHash.contains(messageID.number()))
            return;

        QLOG_INFO() << "entered the onReaction core body with reaction: " << QString::fromStdString(emoji.name);
        QSharedPointer<discord::Server> server = GetServerInstanceForChannel(channelID,
                                                                             channelToServerHash.contains(channelID.number())
                                                                             ? channelToServerHash.value(channelID.number()) : 0);

        bool isOriginalUser = messageSourceAndTypeHash.same_user(messageID.number(), userID.number());
        if(isOriginalUser){
            An<Users> users;
            auto user = users->GetUser(QString::fromStdString(userID.string()));
            if(!user)
                return;
            QLOG_INFO() << "bot is fetching message information";

            auto messageInfo = messageSourceAndTypeHash.value(messageID.number());
            messageInfo.token.messageID = messageID;
            if(emoji.name *in("👉", "👈")){


                bool scrollDirection = emoji.name == "👉" ? true : false;
                CommandChain command;
                if(messageInfo.sourceCommandType == ECommandType::ct_display_page)
                    command = CreateChangeRecommendationsPageCommand(user,server, messageInfo.token, scrollDirection);
                else
                    command = CreateChangeHelpPageCommand(user,server, messageInfo.token, scrollDirection);

                command += CreateRemoveReactionCommand(user,server, messageInfo.token, emoji.name == "👉" ? "%f0%9f%91%89" : "%f0%9f%91%88");
                executor->Push(std::move(command));
            }
            else if(emoji.name == "🔁")
            {
                CommandChain commands;
                commands = CreateRollCommand(user,server, messageInfo.token);
                commands += CreateRemoveReactionCommand(user,server, messageInfo.token, "%f0%9f%94%81");
                executor->Push(std::move(commands));
            }

        }
        else if(userID != getID()){
            QString str = QString(" Navigation emoji are only working for the person that the bot responded to. Perhaps you need to do one of the following:"
                                  "\n- create your own recommendations with `%1recs YOUR_FFN_ID`"
                                  "\n- repost your recommendations with `%1page`"
                                  "\n- call your own help page with `%help`");
            str=str.arg(QString::fromStdString(std::string(server->GetCommandPrefix())));
            sendMessageWrapper(channelID, server->GetServerId(), CreateMention(userID.string()) + str.toStdString());
        }
    }
    catch(const rapidjson_exception& e){
        qDebug() << e.what();
    }
}

void Client::Log(const SleepyDiscord::Message& message)
{
    if(message.content.length() > 100)
    {
        auto pos = message.content.find(' ', 100);
        QLOG_INFO() << QString::fromStdString(message.serverID.string() + " " + message.channelID.string() + " " + message.author.username + "#" + message.author.discriminator + " " + message.author.ID.string() + " " + message.content.substr(0, pos) + "...");
    }
    else
        QLOG_INFO() << QString::fromStdString(message.serverID.string() + " " + message.channelID.string() + " " + message.author.username + "#" + message.author.discriminator + " " + message.author.ID.string() + " " + message.content);

    try{
    if(message.channelID.number() == mirrorSourceChannel)
        sendMessage(SleepyDiscord::Snowflake<SleepyDiscord::Channel>(mirrorTargetChannel), message.author.username + "#" + message.author.discriminator + " says: " + message.content);

    //qDebug()  << "mention: " << QString::fromStdString(CreateMention(getID().string()));
    thread_local std::string botMention = getID().string();
    if(message.author.ID != getID() && !message.author.bot && (message.content.find(botMention) != std::string::npos || message.content.find("ocrates") != std::string::npos))
        sendMessage(SleepyDiscord::Snowflake<SleepyDiscord::Channel>(botPmChannel), message.author.username + "#" + message.author.discriminator + " says: " + message.content);
    }
    catch (const SleepyDiscord::ErrorCode& error){
        QLOG_INFO() << "Discord error:" << error;
    }
}

void Client::timerEvent(QTimerEvent *)
{
    An<discord::Users> users;
    users->ClearInactiveUsers();
}

void discord::Client::onReady(SleepyDiscord::Ready )
{
    botPrefixRequest = "<@!" + getID().string() + "> prefix";
}

MessageResponseWrapper Client::sendMessageWrapper(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID,
                                                                          SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID,
                                                                          const std::string& message,
                                                                          const SleepyDiscord::Embed& embed)
{

    QLOG_INFO() << "bot is sending response message";
    QSharedPointer<discord::Server> server = GetServerInstanceForChannel(channelID, serverID.string());
    try{
    if(allowMessages){
        if(server && server->IsAllowedChannelForSendMessage(channelID)){
            return {true, SleepyDiscord::DiscordClient::sendMessage(channelID, message, embed)};
        }
        else{
            QLOG_INFO() << "Message sending prevented due to lack of permissions";
        }
    }
    }
    catch (const SleepyDiscord::ErrorCode& error){
        if(error != 403)
            QLOG_INFO() << "Discord error:" << error;
        else{
            // we don't have permissions to send messages on this server and channel, preventing this from happening again
            if(server){
                server->AddForbiddenChannelForSendMessage(channelID.string());
            }
        }
    }
    return {false};
}

MessageResponseWrapper Client::sendMessageWrapper(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID,
                                                  SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID,
                                                  const std::string& message)
{
    QLOG_INFO() << "bot is sending response message";
    QSharedPointer<discord::Server> server = GetServerInstanceForChannel(channelID, serverID.string());
    try{
        if(allowMessages){
            if(server && server->IsAllowedChannelForSendMessage(channelID)){
                return {true, SleepyDiscord::DiscordClient::sendMessage(channelID, message)};
            }
            else{
                QLOG_INFO() << "Message sending prevented due to lack of permissions";
            }
        }
    }
    catch (const SleepyDiscord::ErrorCode& error){
        if(error != 403)
            QLOG_INFO() << "Discord error:" << error;
        else{
            // we don't have permissions to send messages on this server and channel, preventing this from happening again
            if(server){
                server->AddForbiddenChannelForSendMessage(channelID.string());
            }
        }
    }
    return {false};
}
}






