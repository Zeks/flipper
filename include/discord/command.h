#pragma once
#include <QList>
#include <QList>
#include "sleepy_discord/embed.h"
#include "sleepy_discord/message.h"
#include "discord/discord_user.h"


namespace discord {

struct ResultingMessage{
    QString text;
    SleepyDiscord::Embed embed;
    QString discordUserId;
};
struct Command{
    enum ECommandType{
        ct_none = 0,
        ct_fill_recommendations = 1,
        ct_display_page = 2,
        ct_ignore_fics = 4,
        ct_list = 5,
        ct_tag = 6,
        ct_set_identity = 7,
        ct_set_fandoms = 8,
        ct_ignore_fandoms = 9,
        ct_display_help = 10,
        ct_ignore_fandom_with_crosses = 11,
    };
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

    QList<int> ids;
    QList<QString> strings;
    SleepyDiscord::Message originalMessage;
    QSharedPointer<User> user;

    ResultingMessage result;
    std::function<ResultingMessage()> executor;
};

struct CommandChain{
    int Size(){return commands.size();}
    void Push(Command);
    Command Pop();
    CommandChain& operator+=(const CommandChain& other){
        this->commands += other.commands;
        return *this;
    };
    QList<Command> commands;
    bool hasParseCommand = false;
};

}
