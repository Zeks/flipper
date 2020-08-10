#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "sleepy_discord/websocketpp_websocket.h"
#include "sleepy_discord/sleepy_discord.h"
#include "sleepy_discord/message.h"
#pragma GCC diagnostic pop
#include "discord/limits.h"
#include "core/section.h"
#include <QString>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QFuture>
#include <QFutureWatcher>
namespace interfaces{
class Fandoms;
class Authors;
class Fanfics;
};
namespace database{
class IDBWrapper;
};
class FicSourceGRPC;

struct ListData{
    QString userId;
    SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID;
    core::RecommendationListFicData fics;
    QSharedPointer<core::RecommendationList> listParams;
    QString error;
};

//core::RecommendationList task;
struct RecRequest{
    QString userID;
    QString ffnID;
    QStringList requestArguments;
    SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelId;
    std::chrono::system_clock::time_point lastRecQuery = std::chrono::system_clock::now();
};

class MyClientClass : public QObject , public SleepyDiscord::DiscordClient {
    Q_OBJECT
public:
    MyClientClass(const std::string token, const char numOfThreads = SleepyDiscord::USER_CONTROLED_THREADS, QObject* obj = nullptr);
    MyClientClass(QObject* obj = nullptr);
    void RxInit();
    void UsersInit();

    using SleepyDiscord::DiscordClient::DiscordClient;
    void SendDetailedList(QSharedPointer<discord::User> , SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, int advance = 0);
    void SendLongList(QSharedPointer<discord::User> , SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, int advance = 0);
    void onMessage(SleepyDiscord::Message message) override;

    bool ProcessHelpRequest(SleepyDiscord::Message& message);
    bool IsACommand(QString content);
    QString CreateRecsHelpMessage();
    QRegularExpression recsExp;
    QRegularExpression pageExp;
    QSharedPointer<FicSourceGRPC> ficSource;
    QSharedPointer<database::IDBWrapper> userDbInterface;
    QSharedPointer<interfaces::Fandoms> fandoms;
    QSharedPointer<interfaces::Fanfics> fanfics;
    QSharedPointer<interfaces::Authors> authors;
    discord::Users users;
    QQueue<RecRequest> recRequests;
    QFutureWatcher<ListData> watcher;
public slots:
    void OnFutureFinished();
};

