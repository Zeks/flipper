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
};

}
