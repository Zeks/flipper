#include "discord/send_message_command.h"
#include "discord/client_storage.h"
#include "discord/discord_server.h"
#include "discord/discord_user.h"
#include "discord/client_v2.h"
#include "GlobalHeaders/snippets_templates.h"
namespace discord{

QStringList SendMessageCommand::tips = {};
void SendMessageCommand::Invoke(Client * client)
{
    try{
        auto addReaction = [&](const SleepyDiscord::Message& newMessage, std::string targetChannel = ""){
            An<discord::Servers> servers;
            auto server = servers->GetServer(originalMessageToken.serverID);
            try{
                if(!server || server->GetAllowedToAddReactions()){
                    SleepyDiscord::Snowflake<SleepyDiscord::Channel> channel = targetChannel.length() > 0 ? targetChannel : originalMessageToken.channelID.string();
                    for(const auto& reaction: std::as_const(reactionsToAdd))
                        client->addReaction(channel, newMessage.ID, reaction.toStdString());
                }
            }
            catch (const SleepyDiscord::ErrorCode& error){
                if(error != 403)
                    QLOG_INFO() << "Discord error:" << error;
                else{
                    // we don't have permissions to add reaction on this server, preventing this from happening again
                    if(server){
                        server->SetAllowedToAddReactions(false);
                    }
                }
            }
        };
        auto removeReaction = [&](SleepyDiscord::Snowflake<SleepyDiscord::Message> targetMessage, std::string targetChannel = ""){
            for(const auto& reaction: std::as_const(reactionsToRemove))
                try{
                An<discord::Servers> servers;
                auto server = servers->GetServer(originalMessageToken.serverID);
                if(!server || server->GetAllowedToRemoveReactions()){
                    SleepyDiscord::Snowflake<SleepyDiscord::Channel> channel = targetChannel.length() > 0 ? targetChannel : originalMessageToken.channelID.string();
                    client->removeReaction(channel, targetMessage, reaction.toStdString(), user->UserID().toStdString());
                }
            }
            catch (const SleepyDiscord::ErrorCode& error){
                if(error != 403)
                    QLOG_INFO() << "Discord error:" << error;
                else{
                    // we don't have permissions to edit reaction on this server, preventing this from happening again
                    An<discord::Servers> servers;
                    auto server = servers->GetServer(originalMessageToken.serverID);
                    if(server){
                        server->SetAllowedToRemoveReactions(false);
                    }
                }
            }
        };
        An<discord::Servers> servers;
        auto server = servers->GetServer(originalMessageToken.serverID);
        //auto channelToSendTo = originalMessageToken.channelID;
        SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelToSendTo = targetChannel.string().length() > 0 ? targetChannel : originalMessageToken.channelID;
        if(targetMessage.string().length() == 0){
            if(targetChannel.string().length() > 0 && targetChannel.string() != "0")
                channelToSendTo = targetChannel;
            if(embed.empty())
            {
                SleepyDiscord::Embed embed;
                if(text.length() > 0)
                {
                    try{
                        if(!server || server->IsAllowedChannelForSendMessage(channelToSendTo.string()))
                            client->sendMessageWrapper(channelToSendTo, originalMessageToken.serverID,text.toStdString(), embed);
                    }
                    catch (const SleepyDiscord::ErrorCode& error){
                        if(error != 403)
                            QLOG_INFO() << "Discord error:" << error;
                        else{
                            // we don't have permissions to send messages on this server and channel, preventing this from happening again
                            if(server){
                                server->AddForbiddenChannelForSendMessage(channelToSendTo.string());
                            }
                        }
                    }
                }
            }
            else{
                try{

                    MessageResponseWrapper resultingMessage;
                    if(!server || server->IsAllowedChannelForSendMessage(originalMessageToken.channelID.string()))
                        resultingMessage = client->sendMessageWrapper(channelToSendTo, originalMessageToken.serverID, text.toStdString(), embed);
                    if(resultingMessage.response.has_value()){
                        // I only need to hash messages that the user can later react to
                        // meaning page, rng and help commands
                        if(originalCommandType *in(ct_display_page, ct_display_rng, ct_display_help, ct_show_fic)){
                            MessageIdToken newToken = originalMessageToken;
                            newToken.messageID = resultingMessage.response->cast().ID.number();
                            if(!targetChannel.string().empty())
                                newToken.channelID = targetChannel;
                            if(messageData){
                                An<ClientStorage> storage;
                                messageData->token = newToken;
                                storage->messageData.push(newToken.messageID.number(),messageData);
                                storage->timedMessageData.push(newToken.messageID.number(),messageData);
                            }
                            if(originalCommandType *in(ct_display_page, ct_display_rng)){
                                if(messageData->CanBeUsedAsLastPage() )
                                    this->user->SetLastRecsPageMessage({resultingMessage.response->cast(), channelToSendTo});
                                this->user->SetLastPostedListCommandMemo({resultingMessage.response->cast(), channelToSendTo});
                            }

                            this->user->SetLastAnyTypeMessageID(resultingMessage.response->cast());

                            if(targetChannel.string().length() > 0)
                                originalMessageToken.channelID = targetChannel;

                            addReaction(resultingMessage.response.value().cast(), targetChannel.string());
                        }
                    }

                }
                catch (const SleepyDiscord::ErrorCode& error){
                    if(error != 403)
                        QLOG_INFO() << "Discord error:" << error;
                    else{
                        // we don't have permissions to send messages on this server and channel, preventing this from happening again
                        if(server){
                            server->AddForbiddenChannelForSendMessage(channelToSendTo.string());
                        }
                    }
                }
            }
        }
        else{
            if(this->deleteOriginalMessage){
                An<ClientStorage> storage;
                client->deleteMessage(originalMessageToken.channelID,targetMessage);
                storage->messageData.remove(targetMessage.number());
            }
            else if(reactionsToRemove.size() > 0)
                removeReaction(targetMessage);
            else
                // editing message doesn't change its Id so rehashing isn't required
                client->editMessage(channelToSendTo, targetMessage, text.toStdString(), embed);
        }
        if(!diagnosticText.isEmpty())
            client->sendMessageWrapper(channelToSendTo,originalMessageToken.serverID, diagnosticText.toStdString());
    }
    catch (const SleepyDiscord::ErrorCode& error){
        QLOG_INFO() << "Discord error:" << error;
        QLOG_INFO() << "Discord error:" << QString::fromStdString(this->embed.description);
    }

}


}
