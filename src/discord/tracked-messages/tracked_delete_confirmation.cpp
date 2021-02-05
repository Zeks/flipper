#include "discord/tracked-messages/tracked_delete_confirmation.h"
#include "discord/client_v2.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"
#include "fmt/format.h"


namespace discord{

TrackedDeleteConfirmation::TrackedDeleteConfirmation(QString identifier, QString type):TrackedMessageBase()
{
    otherUserBehaviour = TrackedMessageBase::noub_legal;
    actionableEmoji = {"✅"};
}

CommandChain TrackedDeleteConfirmation::ProcessReactionImpl(Client* client, QSharedPointer<User> user, SleepyDiscord::Emoji emoji)
{

    CommandChain commands;
    auto token = this->token;
    token.authorID = user->UserID().toStdString();
    auto server = client->GetServerInstanceForChannel(token.channelID,token.serverID);
    if(emoji == "🔍"){
        commands = CreateSimilarListCommand(user,server, token, ficId);
        commands += CreateRemoveReactionCommand(user,server, token, GetEmojiSet().at(0).toStdString());
    }
    else{
        commands = CreateRemoveBotMessageCommand(client, user,server, token);
        if(commands.commands.front().type == ct_null_command)
            commands += CreateRemoveReactionCommand(user,server, token, GetEmojiSet().at(1).toStdString());

    }
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
    return "Only the intended user can use this command";
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
