#include "discord/tracked-messages/tracked_message_base.h"
#include "discord/discord_message_token.h"
#include "discord/client_v2.h"


namespace discord{

discord::CommandChain discord::TrackedMessageBase::ProcessReaction(discord::Client *client, QSharedPointer<discord::User> user, SleepyDiscord::Emoji emoji)
{
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



}
