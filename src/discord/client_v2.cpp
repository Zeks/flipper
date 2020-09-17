#include "discord/client_v2.h"
#include "discord/command_controller.h"
#include "discord/command_creators.h"
#include "discord/command.h"
#include "discord/discord_server.h"
#include "discord/discord_init.h"
#include "discord/type_functions.h"
#include "discord/db_vendor.h"
#include "sql/discord/discord_queries.h"
#include "logger/QsLog.h"
#include <QRegularExpression>
#include <string_view>
#include <QSharedPointer>
//#include "third_party/str_concat.h"
#include "third_party/ctre.hpp"

namespace discord {
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
    auto regex = GetSimpleCommandIdentifierPrefixless();
    fictionalDMServer.reset(new discord::Server());
    fictionalDMServer->SetQuickCommandIdentifier(std::regex((fictionalDMServer->GetCommandPrefix() + regex).toStdString()));
    discord::InitDefaultCommandSet(this->parser);
    std::vector<SleepyDiscord::Server>  sleepyServers = getServers();
    for(auto server : sleepyServers){
        InitDiscordServerIfNecessary(server.ID);
    }
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
            auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
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

static constexpr auto pattern = discord::GetSimplePatternChecker();
constexpr auto matchSimple(std::string_view sv) noexcept {
    return ctre::match<pattern>(sv);
}

void Client::onMessage(SleepyDiscord::Message message) {
    if(message.author.bot)
        return;

    Log(message);

    QSharedPointer<discord::Server> server;
    auto channel = getChannel(message.channelID);
    if(channel.cast().type != SleepyDiscord::Channel::DM){
        server = InitDiscordServerIfNecessary(message.serverID);
    }
    else
        server = fictionalDMServer;
    std::string_view sv (message.content);

    if(sv == "<@!" + getID().string() + "> prefix")
        sendMessage(message.channelID, "Prefix for this server is: " + server->GetCommandPrefix().toStdString());

    if(sv.substr(0, server->GetCommandPrefix().length()) != server->GetCommandPrefix().toStdString())
        return;
    sv.remove_prefix(server->GetCommandPrefix().length());

    auto result = ctre::search<pattern>(sv);;
    if(!message.content.size() || (message.content.size() && !result.matched()))
        return;
    std::string cap = result.get<0>().to_string();

    auto commands = parser->Execute(cap, server, message);
    if(commands.Size() == 0)
        return;
    executor->Push(commands);
}

void Client::Log(const SleepyDiscord::Message message)
{
    QLOG_INFO() << " " << message.channelID.number() << " " << QString::fromStdString(message.author.username) << QString::number(message.author.ID.number()) << " " << QString::fromStdString(message.content);
}

void Client::timerEvent(QTimerEvent *)
{
    An<discord::Users> users;
    users->ClearInactiveUsers();
}

}




