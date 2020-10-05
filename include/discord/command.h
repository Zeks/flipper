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
    SleepyDiscord::Message originalMessage;
    QSharedPointer<User> user;
    QSharedPointer<Server> server;

    ResultingMessage result;
    std::function<ResultingMessage()> executor;
    QString textForPreExecution;
    SleepyDiscord::Snowflake<SleepyDiscord::Message> targetMessage;
};

struct CommandChain{
    int Size(){return commands.size();}
    void Push(Command);
    void AddUserToCommands(QSharedPointer<User>);
    Command Pop();
    void RemoveEmptyCommands();
    CommandChain& operator+=(const CommandChain& other){
        this->commands += other.commands;
        return *this;
    };
    void Reset(){
        commands.clear();
        hasParseCommand = false;
        stopExecution = false;
    };
    QList<Command> commands;
    QSharedPointer<User> user;
    bool hasParseCommand = false;
    bool hasFullParseCommand = false;
    bool stopExecution = false;
};

}
