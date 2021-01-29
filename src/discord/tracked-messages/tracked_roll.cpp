#include "discord/tracked-messages/tracked_roll.h"
#include "discord/client_v2.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"
#include "fmt/format.h"
#include "GlobalHeaders/snippets_templates.h"


namespace discord{

TrackedRoll::TrackedRoll(QSharedPointer<User> user):TrackedRecommendationList(user)
{
    actionableEmoji = {"🔁"};
}

CommandChain TrackedRoll::ProcessReactionImpl(Client* client, QSharedPointer<User> user, SleepyDiscord::Emoji)
{
    QLOG_INFO() << "bot is fetching message information";

    auto server = client->GetServerInstanceForChannel(token.channelID,token.serverID);
    CommandChain commands;
    commands = CreateRollCommand(user,server, token);
    commands += CreateRemoveReactionCommand(user,server, token, "%f0%9f%94%81");
    return commands;
}

QStringList TrackedRoll::GetEmojiSet()
{
    static const QStringList emoji = {"%f0%9f%94%81"};
    return emoji;
}

std::string TrackedRoll::GetOtherUserErrorMessage(Client* client)
{
    return fmt::format("You need to spawn your own help page with {0}help", client->GetServerInstanceForChannel(token.channelID,token.serverID)->GetCommandPrefix());
}

CommandChain TrackedRoll::CloneForOtherUser()
{
    return {}; // intentionally empty for now, this is for the future
}
}
