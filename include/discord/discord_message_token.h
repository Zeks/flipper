/*Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
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

struct MessageIdToken{
    MessageIdToken() = default;
    MessageIdToken(const SleepyDiscord::Message& message){
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
    int ficId;
};

}
