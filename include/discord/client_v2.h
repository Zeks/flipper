#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#include "sleepy_discord/websocketpp_websocket.h"
#include "sleepy_discord/sleepy_discord.h"
#include "sleepy_discord/message.h"
#pragma GCC diagnostic pop

#include "discord/limits.h"
#include "discord/command_generators.h"
#include "discord/discord_user.h"
#include "discord/identity_hash.h"
#include "discord/cached_message_source.h"
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


struct ChannelSet{
    inline void push(int64_t channelId){
        QWriteLocker locker(&lock);
        set.insert(channelId);
    }
    inline bool contains(int64_t channelId){
        QReadLocker locker(&lock);
        return set.contains(channelId);
    }
    QSet<int64_t> set;
    QReadWriteLock lock;
};


class Client: public QObject , public SleepyDiscord::DiscordClient {
    Q_OBJECT
public:
    Client(const std::string token, const char numOfThreads = SleepyDiscord::USER_CONTROLED_THREADS, QObject* obj = nullptr);
    Client(QObject* obj = nullptr);
    void InitClient();
    QSharedPointer<discord::Server> InitDiscordServerIfNecessary(SleepyDiscord::Snowflake<SleepyDiscord::Server> serverId);
    void InitCommandExecutor();
    QSharedPointer<discord::Server> GetServerInstanceForChannel(SleepyDiscord::Snowflake<SleepyDiscord::Channel>, SleepyDiscord::Snowflake<SleepyDiscord::Server>);
    QSharedPointer<Server> GetServerInstanceForChannel(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, uint64_t serverID);
    using SleepyDiscord::DiscordClient::DiscordClient;
    void onMessage(SleepyDiscord::Message message) override;
    void onReaction(SleepyDiscord::Snowflake<SleepyDiscord::User> userID, SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, SleepyDiscord::Snowflake<SleepyDiscord::Message> messageID, SleepyDiscord::Emoji emoji) override;
    void onReady (SleepyDiscord::Ready readyData) override;
    SleepyDiscord::ObjectResponse<SleepyDiscord::Message> sendMessage (SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, const std::string &message, const SleepyDiscord::Embed &embed);
    SleepyDiscord::ObjectResponse<SleepyDiscord::Message> sendMessage (SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, const std::string &message);

    void Log(const SleepyDiscord::Message&);

    QSharedPointer<CommandParser> parser;
    std::regex rxCommandIdentifier;
    QSharedPointer<CommandController> executor;
    QSharedPointer<discord::Server> fictionalDMServer;
    QSet<std::string> actionableEmoji;
    BotIdentityMatchingHash<CachedMessageSource> messageSourceAndTypeHash;
    BotIdentityMatchingHash<int64_t> channelToServerHash;
    std::string botPrefixRequest;
    ChannelSet nonPmChannels;
    static std::atomic<bool> allowMessages;
protected:
    virtual void timerEvent(QTimerEvent *) override;
};

}
