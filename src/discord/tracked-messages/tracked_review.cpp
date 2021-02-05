#include "discord/tracked-messages/tracked_review.h"
#include "discord/client_v2.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"
#include "fmt/format.h"
#include "GlobalHeaders/snippets_templates.h"


namespace discord{

TrackedReview::TrackedReview(std::vector<std::string> reviews, QSharedPointer<User> user):TrackedMessageBase(user)
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
    return ",navigation only works for the original user.";
}

CommandChain TrackedReview::CloneForOtherUser()
{
    return {}; // not for now
}

CommandChain TrackedReview::ProcessReactionImpl(Client *client, QSharedPointer<User> user, SleepyDiscord::Emoji emoji)
{
    QLOG_INFO() << "bot is fetching message information";

    CommandChain commands;
    auto token = this->token;
    token.authorID = user->UserID().toStdString();
    auto server = client->GetServerInstanceForChannel(token.channelID,token.serverID);

    if(emoji.name *in("👈","👉")){
        bool scrollDirection = emoji.name == "👉" ? true : false;
        commands = CreateChangeReviewPageCommand(user,server, token, scrollDirection);
        commands += CreateRemoveReactionCommand(user,server, token, emoji.name == "👉" ? "%f0%9f%91%89" : "%f0%9f%91%88");
    }
    else{
        commands = CreateRemoveEntityConfirmationCommand(user,server, token, "review", QString::fromStdString(reviews[currentPosition]));
    }

    return commands;
}

QStringList TrackedReview::GetEmojiSet()
{
    static const QStringList emoji = {QStringLiteral("%f0%9f%91%88"), QStringLiteral("%E2%9D%8C"), QStringLiteral("%f0%9f%91%89")};
    return emoji;
}




}
