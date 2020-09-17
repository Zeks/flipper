#pragma once
#include <QList>
#include <QString>
#include <QRegularExpression>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include "sleepy_discord/embed.h"
#include "sleepy_discord/message.h"
#pragma GCC diagnostic pop

#include "discord/command.h"
#include "discord/limits.h"
#include "discord/discord_user.h"


#include <mutex>

namespace discord{
class Client;
class Server;
struct Command;

template<typename T>
struct CommandState{
    static bool active;
    static QString help;
    static QString regexCommandIdentifier;
    static QString regexWithoutPrefix;
};

template<class T>
bool CommandState<T>::active = false;
template<class T>
QString CommandState<T>::help;
template<class T>
QString CommandState<T>::regexCommandIdentifier;
template<class T>
QString CommandState<T>::regexWithoutPrefix;

class CommandCreator{
public:
    virtual ~CommandCreator();
    virtual CommandChain ProcessInput(Client*, QSharedPointer<discord::Server>, SleepyDiscord::Message, bool verifyUser = false);
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message) = 0;
    virtual bool IsThisCommand(const std::string&) = 0;
    static void EnsureUserExists(QString, QString userName);
    //QRegularExpression rx;
    Command nullCommand;
    CommandChain result;
    QString userId;
    Client* client = nullptr;
    QSharedPointer<discord::Server> server;
    static QSharedPointer<User> user; // shitcode
};



class DisplayHelpCommand: public CommandCreator{
public:
    DisplayHelpCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

// requires active recommendation set to work
class RecommendationsCommand: public CommandCreator{
public:
    RecommendationsCommand();
    virtual CommandChain ProcessInput(Client*, QSharedPointer<discord::Server>, SleepyDiscord::Message, bool verifyUser = false);
    virtual bool IsThisCommand(const std::string& cmd);
};

class RecsCreationCommand : public CommandCreator{
public:
    RecsCreationCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

class PageChangeCommand : public RecommendationsCommand{
public:
    PageChangeCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

class NextPageCommand : public RecommendationsCommand{
public:
    NextPageCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

class PreviousPageCommand : public RecommendationsCommand{
public:
    PreviousPageCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

class SetFandomCommand : public RecommendationsCommand{
public:
    SetFandomCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

class IgnoreFandomCommand : public RecommendationsCommand{
public:
    IgnoreFandomCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

class IgnoreFicCommand: public RecommendationsCommand{
public:
    IgnoreFicCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

class RngCommand: public RecommendationsCommand{
public:
    RngCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

class SetIdentityCommand: public CommandCreator{
public:
    SetIdentityCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

class ChangeServerPrefixCommand: public CommandCreator{
public:
    ChangeServerPrefixCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

class ForceListParamsCommand: public CommandCreator{
public:
    ForceListParamsCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};

class FilterLikedAuthorsCommand: public RecommendationsCommand{
public:
    FilterLikedAuthorsCommand();
    virtual CommandChain ProcessInputImpl(SleepyDiscord::Message);
    virtual bool IsThisCommand(const std::string& cmd);
};


class CommandParser{
public:
    CommandChain Execute(std::string command, QSharedPointer<Server> server, SleepyDiscord::Message);
    QList<QSharedPointer<CommandCreator>> commandProcessors;
    Client* client = nullptr;
    std::mutex lock;
};


class SendMessageCommand{
public:
    static QSharedPointer<SendMessageCommand> Create() {return QSharedPointer<SendMessageCommand>(new SendMessageCommand);}
    void Invoke(discord::Client*);
    SleepyDiscord::Embed embed;
    QString text;
    QSharedPointer<User> user;
    SleepyDiscord::Message originalMessage;
    QStringList errors;
    bool emptyAction = false;
    bool stopChain = false;
};

}
