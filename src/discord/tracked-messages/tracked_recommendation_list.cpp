#include "discord/tracked-messages/tracked_recommendation_list.h"
#include "discord/client_v2.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"
#include "fmt/format.h"
#include "GlobalHeaders/snippets_templates.h"


namespace discord{

TrackedRecommendationList::TrackedRecommendationList()
{
    actionableEmoji = {"👈","👉"};
}

int TrackedRecommendationList::GetDataExpirationIntervalS()
{
    return 5;
}

std::chrono::system_clock::time_point TrackedRecommendationList::GetDataExpirationPoint()
{
    return ficData.expirationPoint;
}

void TrackedRecommendationList::RetireData()
{
    ficData = {};
}

void TrackedRecommendationList::FillMemo(QSharedPointer<User> user)
{
    An<ClientStorage> storage;
    this->memo.page = user->CurrentRecommendationsPage();
    this->memo.filter = user->GetLastUsedStoryFilter();
    this->memo.ficFavouritesCutoff = user->GetRecommendationsCutoff();
    this->ficData.data = user->FicList();
    this->ficData.expirationPoint = std::chrono::system_clock::now() + std::chrono::seconds(this->GetDataExpirationIntervalS());
    this->memo.userFFNId = user->FfnID();
    this->memo.sourceFics = user->FicList()->sourceFicsFFN;
    this->token = token;
    storage->messageData.push(token.messageID.number(),this->shared_from_this());
    storage->timedMessageData.push(token.messageID.number(),this->shared_from_this());
}

std::string TrackedRecommendationList::GetOtherUserErrorMessage(Client *)
{
    return "";
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




}
