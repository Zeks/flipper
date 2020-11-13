#include "discord/client_v2.h"
#include "discord/command_controller.h"
#include "discord/command_generators.h"
#include "discord/command.h"
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

        if(message.content != (std::string(server->GetCommandPrefix()) + "permit") && server->GetServerId().length() > 0 && server->GetDedicatedChannelId().length() > 0 && message.channelID.string() != server->GetDedicatedChannelId())
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
            QString str = QString(" Navigation commands are only working for the person that the bot responded to. If you want your own copy of those, repeat their `%1recs` or `%1help` command or spawn a new list with your own FFN id.");
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
        QLOG_INFO() << QString::fromStdString(message.channelID.string() + " " + message.author.username + message.author.ID.string() + " " + message.content.substr(0, pos) + "...");
    }
    else
        QLOG_INFO() << QString::fromStdString(message.channelID.string() + " " + message.author.username + message.author.ID.string() + " " + message.content);
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






