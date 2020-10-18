#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#include "sleepy_discord/websocketpp_websocket.h"
#include "sleepy_discord/sleepy_discord.h"
#include "sleepy_discord/message.h"
#pragma GCC diagnostic pop

namespace discord{

struct MessageToken{
    MessageToken() = default;
    MessageToken(const MessageToken&) = default;
    MessageToken(MessageToken&&) = default;
    MessageToken& operator=(MessageToken&& other) = default;
    MessageToken& operator=(const MessageToken& other) = default;
    MessageToken(const SleepyDiscord::Message& message){
        messageID = message.ID;
        authorID = message.author.ID;
        serverID = message.serverID;
        channelID = message.channelID;
    };
    bool IsPM() const{
        if(serverID.string().empty())
            return true;
        return false;
    };

    SleepyDiscord::Snowflake<SleepyDiscord::Message> messageID;
    SleepyDiscord::Snowflake<SleepyDiscord::User> authorID;
    SleepyDiscord::Snowflake<SleepyDiscord::Server> serverID;
    SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID;
};

}
