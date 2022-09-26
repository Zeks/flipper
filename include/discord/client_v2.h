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
#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#include "sleepy_discord/websocketpp_websocket.h"
#include "sleepy_discord/sleepy_discord.h"
#include "sleepy_discord/message.h"
#include "sleepy_discord/client.h"
#pragma GCC diagnostic pop

#include "discord/limits.h"
#include "discord/client_storage.h"
#include "discord/command_parser.h"
#include "discord/command_generators.h"
#include "discord/discord_user.h"
#include "discord/identity_hash.h"
#include "discord/cached_message_source.h"
#include "discord/tracked-messages/tracked_message_base.h"
#include "discord/slashcommands.h"
#include "core/section.h"
#include <QString>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QFuture>
#include <QFutureWatcher>
#include <regex>

namespace  discord {
class SendMessageCommand;
class CommandController;
class Server;




struct MessageResponseWrapper{
    MessageResponseWrapper(){}
    MessageResponseWrapper(bool isValid, SleepyDiscord::ObjectResponse<SleepyDiscord::Message> message):isValid(isValid), response(message)
    {}
    MessageResponseWrapper(bool isValid):isValid(isValid)
    {}
    bool isValid = true;
    std::optional<SleepyDiscord::ObjectResponse<SleepyDiscord::Message>> response;
};

class Client: public QObject , public SleepyDiscord::DiscordClient {
    Q_OBJECT
public:
    Client(const std::string token, const char numOfThreads = SleepyDiscord::USER_CONTROLED_THREADS, QObject* obj = nullptr);
    Client(QObject* obj = nullptr);
    void InitClient();
    void onInteraction(SleepyDiscord::Interaction interaction) override;
    QSharedPointer<discord::Server> InitDiscordServerIfNecessary(SleepyDiscord::Snowflake<SleepyDiscord::Server> serverId);
    void InitCommandExecutor();
    QSharedPointer<discord::Server> GetServerInstanceForChannel(SleepyDiscord::Snowflake<SleepyDiscord::Channel>, SleepyDiscord::Snowflake<SleepyDiscord::Server>);
    QSharedPointer<Server> GetServerInstanceForChannel(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, uint64_t serverID);
    QSharedPointer<User> GetOrCreateUser(SleepyDiscord::Snowflake<SleepyDiscord::User> userID);
    using SleepyDiscord::DiscordClient::DiscordClient;
    void onMessage(SleepyDiscord::Message message) override;
    void onReaction(SleepyDiscord::Snowflake<SleepyDiscord::User> userID, SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, SleepyDiscord::Snowflake<SleepyDiscord::Message> messageID, SleepyDiscord::Emoji emoji) override;
    void onReady (SleepyDiscord::Ready readyData) override;
    MessageResponseWrapper sendMessageWrapper (SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID, const std::string &message, const SleepyDiscord::Embed &embed);
    MessageResponseWrapper sendMessageWrapper (SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID, const std::string &message);
    void Log(const SleepyDiscord::Message&);

    QSharedPointer<CommandParser> parser;
    std::regex rxCommandIdentifier;
    QSharedPointer<CommandController> executor;
    QSet<std::string> actionableEmoji;
    std::unique_ptr<slash_commands::CommandDispatcher> slashCommandDispatcher;

    An<ClientStorage> storage;

    std::string botPrefixRequest;
    static std::atomic<bool> allowMessages;
    static std::atomic<int64_t> mirrorTargetChannel;
    static std::atomic<int64_t> mirrorSourceChannel;
    static std::atomic<int64_t> botPmChannel;



protected:
    virtual void timerEvent(QTimerEvent *) override;
};
bool CheckAdminRole(Client* client, QSharedPointer<Server> server, const SleepyDiscord::Snowflake<SleepyDiscord::User>& authorID);

}

