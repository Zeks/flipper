#include "discord/tracked-messages/tracked_review.h"
#include "discord/client_v2.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"
#include "fmt/format.h"
#include "GlobalHeaders/snippets_templates.h"


namespace discord{

TrackedReview::TrackedReview(std::vector<std::string> reviews, QSharedPointer<User> user):TrackedMessageBase(user)
{
    otherUserBehaviour = TrackedMessageBase::noub_legal;
    retireIsFinal = true;
    this->reviews = reviews;
    actionableEmoji = {"👈","👉","❌"};
}

int TrackedReview::GetDataExpirationIntervalS()
{
    return 600;
}

void TrackedReview::RetireData()
{
    reviews.clear();
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
        if(!IsOriginaluser(user->UserID())){
            client->sendMessageWrapper(token.channelID, token.serverID, CreateMention(user->UserID().toStdString()) + ", only the user that requested a review list can scroll it.");
            return commands;
        }
        bool scrollDirection = emoji.name == "👉" ? true : false;
        commands = CreateChangeReviewPageCommand(user,server, token, scrollDirection);
        commands += CreateRemoveReactionCommand(user,server, token, emoji.name == "👉" ? "%f0%9f%91%89" : "%f0%9f%91%88");
    }
    else{
        auto isAdmin = CheckAdminRole(client, server, user->UserID().toStdString());
        if(!IsOriginaluser(user->UserID()) && !isAdmin){
            client->sendMessageWrapper(token.channelID, token.serverID, CreateMention(user->UserID().toStdString()) + ", only the original review author or an admin can delete reviews");
            return commands;
        }

        commands = CreateRemoveEntityConfirmationCommand(user,server, token, "review", QString::fromStdString(reviews[currentPosition]));
    }

    return commands;
}

QStringList TrackedReview::GetEmojiSet()
{
    static const QStringList emoji = {QStringLiteral("%f0%9f%91%88"), QStringLiteral("%f0%9f%91%89"),QStringLiteral("%E2%9D%8C")};
    return emoji;
}




}
