#include "discord/tracked-messages/tracked_message_base.h"
#include "discord/discord_message_token.h"
#include "discord/client_v2.h"
#include "GlobalHeaders/snippets_templates.h"


namespace discord{

discord::CommandChain discord::TrackedMessageBase::ProcessReaction(discord::Client *client, QSharedPointer<discord::User> user, SleepyDiscord::Emoji emoji)
{
    if(!user)
        return {};
    //if(std::find_if(actionableEmoji.begin(),actionableEmoji.end(), [&](const auto& i){ return i == emoji.name;}) == std::end(actionableEmoji))
    if(std::find(actionableEmoji.begin(),actionableEmoji.end(),emoji.name) == std::end(actionableEmoji))
        return {};

    if(!IsOriginaluser(user->UserID())){
        if(otherUserBehaviour == noub_error){
            client->sendMessageWrapper(token.channelID, token.serverID, CreateMention(token.authorID.string()) + GetOtherUserErrorMessage(client));
            return {};
        }else if(otherUserBehaviour == noub_error){
            return CloneForOtherUser();
        }
    }
    return ProcessReactionImpl(client, user, emoji);
}

bool TrackedMessageBase::GetRetireIsFinal() const
{
    return retireIsFinal;
}



}
