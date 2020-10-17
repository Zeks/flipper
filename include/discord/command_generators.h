#pragma once
#include <QList>
#include <QString>
#include <QRegularExpression>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
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
};

template<class T>
bool CommandState<T>::active = false;


class CommandCreator{
public:
    virtual ~CommandCreator() = default;
    virtual CommandChain ProcessInput(Client*, QSharedPointer<discord::Server>, const SleepyDiscord::Message&, bool verifyUser = false);
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&) = 0;
    virtual bool IsThisCommand(const std::string&) = 0;
    void AddFilterCommand(Command&&);
    static void EnsureUserExists(QString, QString userName);
    //QRegularExpression rx;
    Command nullCommand;
    CommandChain result;
    QString userId;
    Client* client = nullptr;
    QSharedPointer<discord::Server> server;
    bool currentOperationRestoresActiveSet = false;
    static QSharedPointer<User> user; // shitcode
};



class DisplayHelpCommand: public CommandCreator{
public:
    DisplayHelpCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

// requires active recommendation set to work
class RecommendationsCommand: public CommandCreator{
public:
    RecommendationsCommand() = default;
    virtual CommandChain ProcessInput(Client*, QSharedPointer<discord::Server>, const SleepyDiscord::Message &, bool verifyUser = false);
    virtual bool IsThisCommand(const std::string& cmd);
};

class RecsCreationCommand : public CommandCreator{
public:
    RecsCreationCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class PageChangeCommand : public RecommendationsCommand{
public:
    PageChangeCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class NextPageCommand : public RecommendationsCommand{
public:
    NextPageCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class PreviousPageCommand : public RecommendationsCommand{
public:
    PreviousPageCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class SetFandomCommand : public RecommendationsCommand{
public:
    SetFandomCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class IgnoreFandomCommand : public RecommendationsCommand{
public:
    IgnoreFandomCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class IgnoreFicCommand: public RecommendationsCommand{
public:
    IgnoreFicCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class RngCommand: public RecommendationsCommand{
public:
    RngCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class SetIdentityCommand: public CommandCreator{
public:
    SetIdentityCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class ChangeServerPrefixCommand: public CommandCreator{
public:
    ChangeServerPrefixCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

//class ForceListParamsCommand: public RecommendationsCommand{
//public:
//    ForceListParamsCommand();
//    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
//    virtual bool IsThisCommand(const std::string& cmd);
//};

class FilterLikedAuthorsCommand: public RecommendationsCommand{
public:
    FilterLikedAuthorsCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

//class ShowFullFavouritesCommand: public CommandCreator{
//public:
//    ShowFullFavouritesCommand();
//    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
//    virtual bool IsThisCommand(const std::string& cmd);
//};

class ShowFreshRecsCommand: public RecommendationsCommand{
public:
    ShowFreshRecsCommand() = default;
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class ShowCompletedCommand: public RecommendationsCommand{
public:
    ShowCompletedCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};


class HideDeadCommand: public RecommendationsCommand{
public:
    HideDeadCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class PurgeCommand: public CommandCreator{
public:
    PurgeCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class ResetFiltersCommand: public RecommendationsCommand{
public:
    ResetFiltersCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class SimilarFicsCommand: public RecommendationsCommand{
public:
    SimilarFicsCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class WordcountCommand: public RecommendationsCommand{
public:
    WordcountCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};


class CommandParser{
public:
    CommandChain Execute(const std::string &command, QSharedPointer<Server> server, const SleepyDiscord::Message &);
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
    QString diagnosticText;
    QStringList reactionsToAdd;
    QSharedPointer<User> user;
    SleepyDiscord::Message originalMessage;
    SleepyDiscord::Snowflake<SleepyDiscord::Message> targetMessage;
    ECommandType originalCommandType = ct_none;
    std::list<CommandChain> commandsToReemit;
    QStringList errors;
    bool emptyAction = false;
    bool stopChain = false;
    static QStringList tips;
};

CommandChain CreateRollCommand(QSharedPointer<User> , QSharedPointer<Server> , const SleepyDiscord::Message& );
CommandChain CreateChangeRecommendationsPageCommand(QSharedPointer<User> , QSharedPointer<Server> , const SleepyDiscord::Message& , bool shiftRight = true);
CommandChain CreateChangeHelpPageCommand(QSharedPointer<User> , QSharedPointer<Server> , const SleepyDiscord::Message&, bool shiftRight = true);
Command NewCommand(QSharedPointer<discord::Server> server, const SleepyDiscord::Message& message, ECommandType type);
}

