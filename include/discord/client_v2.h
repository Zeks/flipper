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

struct BotMessageSet{
    inline void push(int64_t messageId, int64_t userId){
        QWriteLocker locker(&lock);
        hash.insert(messageId, userId);
    }
    inline bool contains(int64_t messageId){
        QReadLocker locker(&lock);
        return hash.contains(messageId);
    }
    inline bool same_user(int64_t messageId, int64_t userId){
        QReadLocker locker(&lock);
        return hash.value(messageId) == userId;
    }
    QHash<int64_t,int64_t> hash;
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
    QSharedPointer<discord::Server> GetServerInstanceForChannel(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID);
    using SleepyDiscord::DiscordClient::DiscordClient;
    void onMessage(SleepyDiscord::Message message) override;
    void onReaction(SleepyDiscord::Snowflake<SleepyDiscord::User> userID, SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, SleepyDiscord::Snowflake<SleepyDiscord::Message> messageID, SleepyDiscord::Emoji emoji) override;

    void Log(const SleepyDiscord::Message);

    QSharedPointer<CommandParser> parser;
    std::regex rxCommandIdentifier;
    QSharedPointer<CommandController> executor;
    QSharedPointer<discord::Server> fictionalDMServer;
    QSet<std::string> actionableEmoji;
    BotMessageSet messageHash;
protected:
    virtual void timerEvent(QTimerEvent *) override;
};

}
