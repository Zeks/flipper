#include "discord/tracked-messages/tracked_review.h"
#include "discord/client_v2.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"
#include "fmt/format.h"
#include "GlobalHeaders/snippets_templates.h"


namespace discord{

TrackedReview::TrackedReview(std::vector<std::string> reviews):TrackedMessageBase()
{
    this->reviews = reviews;
    actionableEmoji = {"👈","❌","👉"};
}

int TrackedReview::GetDataExpirationIntervalS()
{
    return 300;
}

std::chrono::system_clock::time_point TrackedReview::GetDataExpirationPoint()
{
    return std::chrono::high_resolution_clock::now();
}

void TrackedReview::RetireData()
{
    // not much point for now
}

std::string TrackedReview::GetOtherUserErrorMessage(Client *)
{
    return "";
}

CommandChain TrackedReview::CloneForOtherUser()
{
    return {};
}

CommandChain TrackedReview::ProcessReactionImpl(Client *client, QSharedPointer<User> user, SleepyDiscord::Emoji emoji)
{
    QLOG_INFO() << "bot is fetching message information";

    CommandChain commands;
    auto token = this->token;
    token.authorID = user->UserID().toStdString();

    if(emoji.name *in("👈","👉")){
        auto server = client->GetServerInstanceForChannel(token.channelID,token.serverID);
        bool scrollDirection = emoji.name == "👉" ? true : false;
        CommandChain commands;
        commands = CreateChangeReviewPageCommand(user,server, token, scrollDirection);
        commands += CreateRemoveReactionCommand(user,server, token, emoji.name == "👉" ? "%f0%9f%91%89" : "%f0%9f%91%88");
    }
    else{

    }

    return commands;
}

QStringList TrackedReview::GetEmojiSet()
{
    static const QStringList emoji = {QStringLiteral("%f0%9f%91%88"), QStringLiteral("%f0%9f%91%89")};
    return emoji;
}




}
