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
#include "discord/type_functions.h"
#include "sql/discord/discord_queries.h"
#include "logger/QsLog.h"
#include "GlobalHeaders/snippets_templates.h"
#include <QRegularExpression>
#include <QRegExp>
#include <string_view>
#include <QSharedPointer>
#include <QSettings>
#include <QTextCodec>
//#include "third_party/str_concat.h"
#include "third_party/ctre.hpp"

class rapidjson_exception : public std::runtime_error
{
public:
rapidjson_exception() : std::runtime_error("json schema invalid") {}
};
#define RAPIDJSON_ASSERT(x)  if(x); else throw rapidjson_exception();

namespace discord {

std::size_t replace_all(std::string* inout, std::string_view what, std::string_view with)
{
    std::size_t count{};
    for (std::string::size_type pos{};
         inout->npos != (pos = inout->find(what.data(), pos, what.length()));
         pos += with.length(), ++count) {
        inout->replace(pos, what.length(), with.data(), with.length());
    }
    return count;
}

std::atomic<bool> Client::allowMessages = true;

std::atomic<int64_t> Client::mirrorTargetChannel = 0;
std::atomic<int64_t> Client::mirrorSourceChannel = 0;
std::atomic<int64_t> Client::botPmChannel = 0;

bool CheckAdminRole(Client* client, QSharedPointer<Server> server, const SleepyDiscord::Snowflake<SleepyDiscord::User>& authorID)
{
    SleepyDiscord::Server sleepyServer = client->getServer(server->GetServerId());
    const auto& member = client->getMember(server->GetServerId(), authorID).cast();
    bool isAdmin = sleepyServer.ownerID == authorID || (authorID.string() == "102212539609280512");

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
    startTimer(1000);

    setIntents(SleepyDiscord::Intent::SERVER_MESSAGES,
               SleepyDiscord::Intent::SERVER_MESSAGE_REACTIONS,
               SleepyDiscord::Intent::DIRECT_MESSAGES,
               SleepyDiscord::Intent::DIRECT_MESSAGE_REACTIONS,
               SleepyDiscord::Intent::SERVERS);

    storage->fictionalDMServer.reset(new discord::Server());
    discord::InitDefaultCommandSet(this->parser);

    InitTips();

    QSettings settings(QStringLiteral("settings/settings_discord.ini"), QSettings::IniFormat);
    auto ownerId = settings.value(QStringLiteral("Login/ownerId")).toString().toULongLong();
    CommandCreator::ownerId = ownerId;
    Client::mirrorSourceChannel = 0;
    Client::mirrorTargetChannel = settings.value(QStringLiteral("Login/text_to")).toULongLong();
    Client::botPmChannel = settings.value(QStringLiteral("Login/pm_to")).toULongLong();

    actionableEmoji = {"🔁","👈","👉","🔍","❌","✅"};
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


QSharedPointer<Server> Client::GetServerInstanceForChannel(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID,
                                                           SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID)
{
    QSharedPointer<discord::Server> server;
    if(serverID.string().length() == 0)
        server = storage->fictionalDMServer;
    else
        server = GetServerInstanceForChannel(channelID,serverID.number());

    return server;
}

QSharedPointer<Server> Client::GetServerInstanceForChannel(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, uint64_t serverID)
{
    An<ClientStorage> storage;
    storage.getData();
    QSharedPointer<discord::Server> server;
    if(serverID == 0)
        server = storage->fictionalDMServer;
    else if(storage->nonPmChannels.contains(channelID.number())){
        server = InitDiscordServerIfNecessary(serverID);
    }
    else{
        auto channel = getChannel(channelID).cast();
        if(channel.type != SleepyDiscord::Channel::DM){
            storage->nonPmChannels.push(channelID.number());
            server = InitDiscordServerIfNecessary(channel.serverID);
        }
        else
            server = storage->fictionalDMServer;
    }
    return server;
}



static constexpr auto pattern = discord::GetSimplePatternChecker();
constexpr auto matchSimple(std::string_view sv) noexcept {
    return ctre::match<pattern>(sv);
}
static constexpr std::string_view ffnUrlPattern  = "fanfiction.net/s/(\\d{1,16})";

std::vector<std::string> FetchFFNUrls(const std::string& message){
    std::vector<std::string> result;
    for(auto match : ctre::range<ffnUrlPattern>(message)){
        auto rangeStr = match.get<1>().to_string();
        result.push_back(rangeStr);
    }
    return result;
}

void Client::onMessage(SleepyDiscord::Message message) {
    try{
        auto logRest = [&]{Log(message);};
        if(message.author.bot || !message.content.size())
            return logRest();

        QSharedPointer<discord::Server> server = GetServerInstanceForChannel(message.channelID, message.serverID.string());
        if(server->GetBanned()){
            if(!server->GetShownBannedMessage()){
                server->SetShownBannedMessage(true);
                sendMessageWrapper(message.channelID, message.serverID, "This server has been banned from using the bot.\n"
                                                                        "The most likely reason for this happening is malicious misuse by its members.\n"
                                                                        "If you want to appeal the ban, join: https://discord.gg/AdDvX5H");
            }
            return logRest();
        }

        replace_all(&message.content, "<@742050608235937916>", "");
        replace_all(&message.content, "<@528328968089108510>", "");

        std::string_view sv (message.content);

        const auto commandPrefix = server->GetCommandPrefix();
        if(sv == botPrefixRequest)
            sendMessageWrapper(message.channelID, message.serverID, "Prefix for this server is: " + std::string(commandPrefix));

        if(message.author.ID.string() == "643561873626628249" && (message.content.find("http") != -1 || message.content.find("www") != -1 ))
        {
            QSettings settings(QStringLiteral("settings/settings_discord.ini"), QSettings::IniFormat);
            settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
            auto varno = settings.value(QStringLiteral("Varno/description")).toString();
            sendMessageWrapper(message.channelID, message.serverID, varno.toStdString());
        }

        bool explaining = false;
        if(server->GetExplanationAllowed()){
            auto result = FetchFFNUrls(message.content);
            std::string command = std::string(commandPrefix) + "show";
            if(result.size() > 0 && message.content.find(std::string(commandPrefix) + "review") == std::string::npos)
            {
                explaining = true;
                for(auto bit: result)
                {
                    command+= " " + bit;
                }
                message.content = command;
            }
        }


        if(sv.substr(0, commandPrefix.length()) != commandPrefix)
            return logRest();

        if(message.content.length() > 500 && message.content.find("review add") == std::string::npos){
            sendMessageWrapper(message.channelID, message.serverID, "Your command is too long. Perhaps you've mistyped accidentally?");
            return logRest();
        }

        sv.remove_prefix(commandPrefix.length());

        auto result = ctre::search<pattern>(sv);;
        if(!result.matched())
            return logRest();

        bool priorityCommand = sv.substr(0, 6) *in("permit", "target", "review") || sv.substr(0, 4) == "send";
        if(!priorityCommand && server->GetServerId().length() > 0 && server->GetDedicatedChannelId().length() > 0 && (!explaining && message.channelID.string() != server->GetDedicatedChannelId()))
            return logRest();

        auto user = GetOrCreateUser(message.author.ID);
        if(!user)
            return;

        auto commands = parser->Execute(result.get<0>().to_string(), server, message);
        if(commands.Size() == 0)
            return logRest();

        // instantiating channel -> server pairing if necessary to avoid hitting the api in onReaction needlessly
        if(message.serverID.string().length() > 0 && !storage->channelToServerHash.contains(message.channelID.number())){
            storage->channelToServerHash.push(message.channelID.number(), message.serverID.number());
        }
        QLOG_INFO() << "\nCOMMAND:" << QString::fromStdString(message.serverID.string() + " " + message.channelID.string() + " " + message.author.username + "#" + message.author.discriminator + " " + message.author.ID.string() + " " + message.content + "\n");
        executor->Push(std::move(commands));
    }
    catch(const rapidjson_exception& e){
        qDebug() << e.what();
    }
}


QSharedPointer<User> Client::GetOrCreateUser(SleepyDiscord::Snowflake<SleepyDiscord::User> userID)
{
    An<Users> users;
    auto userId = QString::fromStdString(userID.string());
    An<interfaces::Users> usersInterface;
    if(!users->HasUser(userId)){

        bool inDatabase = users->LoadUser(userId);
        if(!inDatabase)
        {
            SleepyDiscord::User sleepyUser = getUser(userID);
            QSharedPointer<discord::User> user(new discord::User(userId, QStringLiteral("-1"), QString::fromStdString(sleepyUser.username), QUuid::createUuid().toString()));
            usersInterface->WriteUser(user);
            users->LoadUser(userId);
        }
    }
    auto user = users->GetUser(userId);
    if(user && !user->UserName().contains(QRegExp("#\\d{4}"))){
        auto sleepyUser = getUser(userID);
        usersInterface->UpdateUsername(QString::fromStdString(userID.string()),
                                       QString::fromStdString(sleepyUser.cast().username  + "#" + sleepyUser.cast().discriminator));
    }

    return user;
}

void Client::onReaction(SleepyDiscord::Snowflake<SleepyDiscord::User> userID, SleepyDiscord::Snowflake<SleepyDiscord::Channel>, SleepyDiscord::Snowflake<SleepyDiscord::Message> messageID, SleepyDiscord::Emoji emoji){
    try{
        //QLOG_INFO() << "entered the onReaction core body with reaction: " << QString::fromStdString(emoji.name);
        if(userID == getID())
            return;

        if(!actionableEmoji.contains(emoji.name))
            return;
        //QLOG_INFO() <<  "emoji is valid, testing message: " << messageID.number();

        if(!storage->messageData.contains(messageID.number()))
            return;
        //QLOG_INFO() <<  "messageid is valid:" << messageID.number();
        auto user = GetOrCreateUser(userID);
        if(!user)
            return;

        //QLOG_INFO() <<  "user is valid";
        QLOG_INFO() << "in reaction body" << QString::fromStdString(emoji.name);
        auto message = storage->messageData.value(messageID.number());
        auto result = message->ProcessReaction(this,user,emoji);
        executor->Push(std::move(result));
        //QLOG_INFO() << "leaving reaction body";
    }
    catch(const rapidjson_exception& e){
        QLOG_INFO() <<  "error in onreaction" << e.what();
        qDebug() << e.what();
    }
}



void Client::Log(const SleepyDiscord::Message& message)
{
    // I really don't need mudae anywhere in my logs
    if(message.content.length() == 0 || message.content.substr(0,2) *in("$m","$w"))
        return;

    if(message.content.length() == 0 || message.content.substr(0,2) *in("$m","$w") || message.author.ID.string() == "432610292342587392"
                || message.channelID.string() *in("766287922860130306", "769018158836875294", "766274737349197824")
                || message.author.ID.string() *in("338511469769654274"))
        return;

    if(message.author.bot && message.author.ID != getID())
        return;

    //replace_all(&message.content, "<@742050608235937916>", "");

    if(message.content.length() > 100)
    {
        auto pos = message.content.find(' ', 100);
        QLOG_INFO() << QString::fromStdString(message.serverID.string() + " " + message.channelID.string() + " " + message.author.username + "#" + message.author.discriminator + " " + message.author.ID.string() + " " + message.content.substr(0, pos) + "...");
    }
    else
        QLOG_INFO() << QString::fromStdString(message.serverID.string() + " " + message.channelID.string() + " " + message.author.username + "#" + message.author.discriminator + " " + message.author.ID.string() + " " + message.content);

    try{
    if(message.channelID.number() == mirrorSourceChannel)
        sendMessage(SleepyDiscord::Snowflake<SleepyDiscord::Channel>(mirrorTargetChannel), "channel: " + message.channelID.string() + " " + message.author.username + "#" + message.author.discriminator + " says: " + message.content);

    //qDebug()  << "mention: " << QString::fromStdString(CreateMention(getID().string()));
    thread_local std::string botMention = getID().string();
    if(message.author.ID != getID() && !message.author.bot && (message.content.find(botMention) != std::string::npos || message.content.find("ocrates") != std::string::npos))
        sendMessage(SleepyDiscord::Snowflake<SleepyDiscord::Channel>(botPmChannel), "channel: " + message.channelID.string() + " " + message.author.username + "#" + message.author.discriminator + " says: " + message.content);
    }
    catch (const SleepyDiscord::ErrorCode& error){
        QLOG_INFO() << "Discord error:" << error;
    }
}

void Client::timerEvent(QTimerEvent *)
{
    An<Users> users;
    users->ClearInactiveUsers();
    for(auto key: storage->timedMessageData.keys()){
        auto value = storage->timedMessageData.value(key);
        if(!value)
            continue;
        auto lock = value->LockResult();
        if(value->GetDataExpirationPoint() < std::chrono::high_resolution_clock::now()){
            value->RetireData();
            storage->timedMessageData.remove(key);
            if(value->GetRetireIsFinal()){
                storage->messageData.remove(key);
            }
        }
    }
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







