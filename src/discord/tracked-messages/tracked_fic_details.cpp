#include "discord/tracked-messages/tracked_fic_details.h"
#include "discord/client_v2.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"
#include "fmt/format.h"


namespace discord{

TrackedFicDetails::TrackedFicDetails()
{
    otherUserBehaviour = TrackedMessageBase::noub_legal;
    actionableEmoji = {"🔍"};
}

CommandChain TrackedFicDetails::ProcessReactionImpl(Client* client, QSharedPointer<User> user, SleepyDiscord::Emoji)
{
    CommandChain commands;
    auto token = this->token;
    token.authorID = user->UserID().toStdString();
    auto server = client->GetServerInstanceForChannel(token.channelID,token.serverID);
    commands = CreateSimilarListCommand(user,server, token, ficId);
    commands += CreateRemoveReactionCommand(user,server, token, GetEmojiSet().at(0).toStdString());
    return commands;
}

int TrackedFicDetails::GetDataExpirationIntervalS()
{
    return 5;
}

QStringList TrackedFicDetails::GetEmojiSet()
{
    static const QStringList emoji = {QStringLiteral("%F0%9F%94%8D")};
    return emoji;
}

std::string TrackedFicDetails::GetOtherUserErrorMessage(Client*)
{
    return ""; // intentionally empty, this command can be used by anyone
}

CommandChain TrackedFicDetails::CloneForOtherUser()
{
    return {}; // intentionally empty for now, this is for the future
}

std::chrono::system_clock::time_point TrackedFicDetails::GetDataExpirationPoint()
{
    return std::chrono::high_resolution_clock::now();
}

void TrackedFicDetails::RetireData()
{
    // empty
}




}
