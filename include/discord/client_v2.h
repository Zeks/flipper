#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "sleepy_discord/websocketpp_websocket.h"
#include "sleepy_discord/sleepy_discord.h"
#include "sleepy_discord/message.h"
#pragma GCC diagnostic pop

#include "discord/limits.h"
#include "discord/command_creators.h"
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
class Client: public QObject , public SleepyDiscord::DiscordClient {
    Q_OBJECT
public:
    Client(const std::string token, const char numOfThreads = SleepyDiscord::USER_CONTROLED_THREADS, QObject* obj = nullptr);
    Client(QObject* obj = nullptr);
    void InitDefaultCommandSet();
    void InitIdentifiersForCommands();
    void InitHelpForCommands();
    void InitCommandExecutor();
    using SleepyDiscord::DiscordClient::DiscordClient;
    void onMessage(SleepyDiscord::Message message) override;
    void Log(const SleepyDiscord::Message);
    template <typename T>
    auto RegisterCommand(){
        parser.commandProcessors.push_back(QSharedPointer<T>(new T()));
        CommandState<T>::active = true;
    };
    CommandParser parser;
    std::regex rxCommandIdentifier;
    QSharedPointer<CommandController> executor;
protected:
    virtual void timerEvent(QTimerEvent *) override;
};

}
