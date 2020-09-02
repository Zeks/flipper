#pragma once
#include <QList>
#include <QString>
#include <QRegularExpression>
#include "sleepy_discord/embed.h"
#include "sleepy_discord/message.h"
#include "discord/limits.h"

namespace discord{

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
        ct_change_page = 3,
        ct_ignore = 4,
        ct_list = 5,
        ct_tag = 6,
        ct_set_identity = 7,
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
    SleepyDiscord::Message originalMessage;
    QSharedPointer<User> user;

    ResultingMessage result;
    std::function<ResultingMessage()> executor;
};
struct CommandChain{
    void Push(Command);
    Command Pop();
    QList<Command> commands;
};


class CommandCreator{
public:
    virtual ~CommandCreator(){};
    virtual CommandChain ProcessInput(SleepyDiscord::Message, bool verifyUser = false);
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message) = 0;
    void EnsureUserExists(QString, QString userName);

    QRegularExpression rx;
    QRegularExpressionMatch match;
    Command nullCommand;
    CommandChain result;
};

class RecsCreationCommand : public CommandCreator{
public:
    RecsCreationCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

class PageChangeCommand : public CommandCreator{
public:
    PageChangeCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

class NextPageCommand : public CommandCreator{
public:
    NextPageCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};

class PreviousPageCommand : public CommandCreator{
public:
    PreviousPageCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
};





class CommandParser{
public:
    Command Execute(QString){

    };
};
}
