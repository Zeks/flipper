#include "discord/tracked-messages/tracked_roll.h"
#include "discord/client_v2.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"
#include "fmt/format.h"
#include "GlobalHeaders/snippets_templates.h"


namespace discord{

CommandChain TrackedRoll::ProcessReactionImpl(Client* client, QSharedPointer<User> user, SleepyDiscord::Emoji emoji)
{
    if(!user)
        return {};
    if(!(emoji.name *in("👉", "👈")))
        return {};

    QLOG_INFO() << "bot is fetching message information";

    auto server = client->GetServerInstanceForChannel(token.channelID,token.serverID);
    bool scrollDirection = emoji.name == "👉" ? true : false;
    CommandChain commands;
    commands = CreateChangeHelpPageCommand(user,server, token, scrollDirection);
    commands += CreateRemoveReactionCommand(user,server, token, emoji.name == "👉" ? "%f0%9f%91%89" : "%f0%9f%91%88");
    return commands;
}

QStringList TrackedRoll::GetEmojiSet()
{
    static const QStringList emoji = {QStringLiteral("%f0%9f%91%88"), QStringLiteral("%f0%9f%91%89")};
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
