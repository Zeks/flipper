#pragma once
#include <QList>
#include <QString>
#include <QRegularExpression>
#include "sleepy_discord/embed.h"
#include "sleepy_discord/message.h"
#include "discord/limits.h"
#include "discord/discord_user.h"

namespace discord{
class Client;

struct ResultingMessage{
    QString text;
    SleepyDiscord::Embed embed;
    QString discordUserId;
};


struct Command;


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
        ct_display_help = 10
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
};


class CommandCreator{
public:
    virtual ~CommandCreator(){};
    virtual CommandChain ProcessInput(SleepyDiscord::Message, bool verifyUser = false);
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message) = 0;
    void EnsureUserExists(QString, QString userName);

    QRegularExpression rx;
    QRegularExpressionMatchIterator matches;
    Command nullCommand;
    CommandChain result;
    QString user;
};



class DisplayHelpCommand: public CommandCreator{
public:
    DisplayHelpCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

// requires active recommendation set to work
class RecommendationsCommand: public CommandCreator{
public:
    RecommendationsCommand();
    virtual CommandChain ProcessInput(SleepyDiscord::Message, bool verifyUser = false);
};

class RecsCreationCommand : public CommandCreator{
public:
    RecsCreationCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

class PageChangeCommand : public RecommendationsCommand{
public:
    PageChangeCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

class NextPageCommand : public RecommendationsCommand{
public:
    NextPageCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

class PreviousPageCommand : public RecommendationsCommand{
public:
    PreviousPageCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

class SetFandomCommand : public RecommendationsCommand{
public:
    SetFandomCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

class IgnoreFandomCommand : public RecommendationsCommand{
public:
    IgnoreFandomCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

class IgnoreFicCommand: public RecommendationsCommand{
public:
    IgnoreFicCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

class SetIdentityCommand: public CommandCreator{
public:
    SetIdentityCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

class CommandParser{
public:
    CommandChain Execute(SleepyDiscord::Message);
    QList<QSharedPointer<CommandCreator>> commandProcessors;
};


class SendMessageCommand{
public:
    void Invoke(discord::Client*);
    SleepyDiscord::Embed embed;
    QString text;
    SleepyDiscord::Message originalMessage;
};

}
