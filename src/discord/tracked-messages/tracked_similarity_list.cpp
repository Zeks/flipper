#include "discord/tracked-messages/tracked_similarity_list.h"
#include "discord/client_v2.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"
#include "fmt/format.h"
#include "GlobalHeaders/snippets_templates.h"


namespace discord{

TrackedSimilarityList::TrackedSimilarityList()
{
    actionableEmoji = {"👈","👉"};
}

CommandChain TrackedSimilarityList::ProcessReactionImpl(Client* client, QSharedPointer<User> user, SleepyDiscord::Emoji emoji)
{
    QLOG_INFO() << "bot is fetching message information";

    auto server = client->GetServerInstanceForChannel(token.channelID,token.serverID);
    bool scrollDirection = emoji.name == "👉" ? true : false;
    CommandChain commands;
    commands = CreateChangeRecommendationsPageCommand(user,server, token, scrollDirection);
    commands += CreateRemoveReactionCommand(user,server, token, emoji.name == "👉" ? "%f0%9f%91%89" : "%f0%9f%91%88");
    return commands;
}

QStringList TrackedSimilarityList::GetEmojiSet()
{
    static const QStringList emoji = {QStringLiteral("%f0%9f%91%88"), QStringLiteral("%f0%9f%91%89")};
    return emoji;
}

std::string TrackedSimilarityList::GetOtherUserErrorMessage(Client* client)
{
    return fmt::format("You need to spawn your own help page with {0}help", client->GetServerInstanceForChannel(token.channelID,token.serverID)->GetCommandPrefix());
}

CommandChain TrackedSimilarityList::CloneForOtherUser()
{
    return {}; // intentionally empty for now, this is for the future
}

int TrackedSimilarityList::GetDataExpirationIntervalS()
{
    return 5;
}

std::chrono::system_clock::time_point TrackedSimilarityList::GetDataExpirationPoint()
{
    return ficData.expirationPoint;
}

void TrackedSimilarityList::RetireData()
{
    ficData = {};
}

void TrackedSimilarityList::FillMemo(QSharedPointer<User> user)
{
    An<ClientStorage> storage;
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




}
