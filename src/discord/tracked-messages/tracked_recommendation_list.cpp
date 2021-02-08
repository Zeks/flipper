#include "discord/tracked-messages/tracked_recommendation_list.h"
#include "discord/client_v2.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#include "sleepy_discord/websocketpp_websocket.h"
#include "sleepy_discord/sleepy_discord.h"
#include "sleepy_discord/message.h"
#pragma GCC diagnostic pop

#include "fmt/format.h"
#include "GlobalHeaders/snippets_templates.h"


namespace discord{

TrackedRecommendationList::TrackedRecommendationList(QSharedPointer<User> user):TrackedMessageBase(user)
{
    actionableEmoji = {"👈","👉"};
}

int TrackedRecommendationList::GetDataExpirationIntervalS()
{
    return 300;
}

std::chrono::system_clock::time_point TrackedRecommendationList::GetDataExpirationPoint()
{
    return ficData.expirationPoint;
}

void TrackedRecommendationList::RetireData()
{
    QLOG_INFO() << "Retiring data of size: " << ficData.data->fics.size();
    ficData = {};
}

std::string TrackedRecommendationList::GetOtherUserErrorMessage(Client *client)
{
    An<Servers> servers;
    auto server = servers->GetServer(token.serverID);
    auto prefix = QString::fromStdString(std::string(server->GetCommandPrefix()));
    QString str = QString(" Navigation emoji are only working for the person that the bot responded to. Perhaps you need to do one of the following:"
                      "\n- create your own recommendations with `%1recs YOUR_FFN_ID`"
                      "\n- repost your recommendations with `%1page`"
                      "\n- call your own help page with `%1help`");
    return str.arg(prefix).toStdString();
}

CommandChain TrackedRecommendationList::CloneForOtherUser()
{
    return {};
}

CommandChain TrackedRecommendationList::ProcessReactionImpl(Client *client, QSharedPointer<User> user, SleepyDiscord::Emoji emoji)
{
    QLOG_INFO() << "bot is fetching message information";

    auto server = client->GetServerInstanceForChannel(token.channelID,token.serverID);
    bool scrollDirection = emoji.name == "👉" ? true : false;
    CommandChain commands;
    commands = CreateChangeRecommendationsPageCommand(user,server, token, scrollDirection);
    commands += CreateRemoveReactionCommand(user,server, token, emoji.name == "👉" ? "%f0%9f%91%89" : "%f0%9f%91%88");
    return commands;
}

QStringList TrackedRecommendationList::GetEmojiSet()
{
    static const QStringList emoji = {QStringLiteral("%f0%9f%91%88"), QStringLiteral("%f0%9f%91%89")};
    return emoji;
}

void TrackedRecommendationList::SetDataExpirationPoint(std::chrono::system_clock::time_point point)
{
    ficData.expirationPoint = point;
}

void TrackedRecommendationList::SetFicdata(QSharedPointer<core::RecommendationListFicData> data)
{
    ficData.data = data;
}

QSharedPointer<core::RecommendationListFicData> TrackedRecommendationList::GetFicData()
{
    return ficData.data;
}

bool TrackedRecommendationList::FicDataEmpty() const
{
    return !ficData.data;
}




}
