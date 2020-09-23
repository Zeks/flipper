#include "discord/client_v2.h"
#include "discord/command_controller.h"
#include "discord/command_generators.h"
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
    fictionalDMServer.reset(new discord::Server());
    discord::InitDefaultCommandSet(this->parser);
    std::vector<SleepyDiscord::Server>  sleepyServers = getServers();
    for(auto server : sleepyServers){
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

QSharedPointer<Server> Client::GetServerInstanceForChannel(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, int64_t serverID)
{
    QSharedPointer<discord::Server> server;
    if(nonPmChannels.contains(channelID.number())){
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
    if(message.author.bot || !message.content.size())
        return;

    QSharedPointer<discord::Server> server = GetServerInstanceForChannel(message.channelID, message.serverID.number());
    std::string_view sv (message.content);

    const auto commandPrefix = server->GetCommandPrefix();
    if(sv == botPrefixRequest)
        sendMessage(message.channelID, "Prefix for this server is: " + commandPrefix);

    if(sv.substr(0, commandPrefix.length()) != commandPrefix)
        return;

    Log(message);
    sv.remove_prefix(commandPrefix.length());

    auto result = ctre::search<pattern>(sv);;
    if(!result.matched())
        return;

    auto commands = parser->Execute(result.get<0>().to_string(), server, message);
    if(commands.Size() == 0)
        return;

    // instantiating channel -> server pairing if necessary to avoid hitting the api in onReaction needlessly
    if(!channelToServerHash.contains(message.channelID.number())){
        channelToServerHash.push(message.channelID.number(), message.serverID.number());
    }
    executor->Push(commands);
}

static std::string CreateMention(const std::string& string){
    return "<@" + string + ">";
}


void Client::onReaction(SleepyDiscord::Snowflake<SleepyDiscord::User> userID, SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, SleepyDiscord::Snowflake<SleepyDiscord::Message> messageID, SleepyDiscord::Emoji emoji){
    if(!actionableEmoji.contains(emoji.name))
        return;
    if(!messageToUserHash.contains(messageID.number()))
        return;


    QSharedPointer<discord::Server> server = GetServerInstanceForChannel(channelID, channelToServerHash.value(channelID.number()));

    bool isOriginalUser = messageToUserHash.same_user(messageID.number(), userID.number());
    if(isOriginalUser){
        An<Users> users;
        auto user = users->GetUser(QString::fromStdString(userID.string()));
        if(!user)
            return;

        auto message = getMessage(channelID, messageID);
        if(emoji.name == "👉"){
            auto changePage = CreateChangePageCommand(user,server, message, true);
            executor->Push(changePage);
        }
        else if(emoji.name == "🔁")
        {
            auto newRoll = CreateRollCommand(user,server, message);
            executor->Push(newRoll);
        }
        else if (emoji.name == "👈"){
            auto changePage = CreateChangePageCommand(user,server, message, false);
            executor->Push(changePage);
        }

    }
    else{
        sendMessage(channelID, CreateMention(userID.string()) + " Navigation commands are only working for the person that the bot responded to. If you want your own copy of those, repeat their `sorecs` command or spawn a new list with your own FFN id.");
    }
}

void Client::Log(const SleepyDiscord::Message& message)
{
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
}






