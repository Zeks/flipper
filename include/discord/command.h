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
#include <QList>
#include <QList>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#include "sleepy_discord/embed.h"
#include "sleepy_discord/message.h"
#pragma GCC diagnostic pop

#include "discord/discord_user.h"
#include "discord/discord_message_token.h"
#include "discord/command_types.h"


namespace discord {
    class Server;

struct ResultingMessage{
    QString text;
    SleepyDiscord::Embed embed;
    QString discordUserId;
};
struct Command{

    enum EOperandType{
        ot_unspecified = 0,
        ot_fanfics = 1,
        ot_fandoms = 2,
        ot_user = 3,
    };
    bool isValid = false;
    ECommandType type = ECommandType::ct_none;
    EOperandType operand = EOperandType::ot_unspecified;
    bool requiresThread = false;

    QList<uint64_t> ids;
    QHash<QString, QVariant> variantHash;
    MessageToken originalMessageToken;
    QSharedPointer<User> user;
    QSharedPointer<Server> server;

    ResultingMessage result;
    std::function<ResultingMessage()> executor;
    QString textForPreExecution;
    SleepyDiscord::Snowflake<SleepyDiscord::Message> targetMessage;
    SleepyDiscord::Snowflake<SleepyDiscord::Channel> targetChannelID;
};

struct CommandChain{
    CommandChain() = default;
    CommandChain(CommandChain&&) = default;
    CommandChain& operator=(CommandChain&& other) = default;
    CommandChain(const CommandChain&) = delete;

    int Size(){return commands.size();}
    void Push(Command&&);
    void PushFront(Command&&);
    void AddUserToCommands(QSharedPointer<User>);
    Command Pop();

    CommandChain& operator+=(CommandChain& other){
        for(auto&& item : other.commands)
        this->commands.emplace_back(std::move(item));
        return *this;
    };
    CommandChain& operator+=(CommandChain&& other){
        for(auto&& item : other.commands)
        this->commands.emplace_back(std::move(item));
        return *this;
    };
    void Reset(){
        commands.clear();
        hasParseCommand = false;
        stopExecution = false;
    };
    std::list<Command> commands;
    QSharedPointer<User> user;
    bool hasParseCommand = false;
    bool hasFullParseCommand = false;
    bool stopExecution = false;
    int delayMs = 0;
};

}
