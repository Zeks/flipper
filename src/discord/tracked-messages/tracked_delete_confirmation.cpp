#include "discord/tracked-messages/tracked_delete_confirmation.h"
#include "discord/client_v2.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"
#include "fmt/format.h"


namespace discord{

TrackedDeleteConfirmation::TrackedDeleteConfirmation(QString type, QString identifier, QSharedPointer<User> user):TrackedMessageBase(user)
{
    otherUserBehaviour = TrackedMessageBase::noub_error;
    actionableEmoji = {"✅"};
    entityType = type;
    entityId = identifier;
    deleteOnExpiration = true;
    deleteOnReaction = true;
}

CommandChain TrackedDeleteConfirmation::ProcessReactionImpl(Client* client, QSharedPointer<User> user, SleepyDiscord::Emoji)
{
    CommandChain commands;

    auto token = this->token;
    token.authorID = user->UserID().toStdString();
    auto server = client->GetServerInstanceForChannel(token.channelID,token.serverID);
    commands = CreateRemoveEntityCommand(user,server, token,entityType, entityId);
    commands += CreateRemoveBotMessageCommand(client, user,server, token);
    commands += CreateRemoveMessageTextCommand(user,server, spawnerToken);
    return commands;
}

int TrackedDeleteConfirmation::GetDataExpirationIntervalS()
{
    return 60;
}

QStringList TrackedDeleteConfirmation::GetEmojiSet()
{
    static const QStringList emoji = {QStringLiteral("%E2%9C%85")};
    return emoji;
}

std::string TrackedDeleteConfirmation::GetOtherUserErrorMessage(Client*)
{
    return " Only the intended user can use this command";
}

CommandChain TrackedDeleteConfirmation::CloneForOtherUser()
{
    return {}; // intentionally empty for now, this is for the future
}

std::chrono::system_clock::time_point TrackedDeleteConfirmation::GetDataExpirationPoint()
{
    return expirationPoint;
}

}
