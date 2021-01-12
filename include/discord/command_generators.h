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
#include "discord/discord_message_token.h"


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
    bool CheckAdminRole(const SleepyDiscord::Message & message);
    //QRegularExpression rx;
    Command nullCommand;
    CommandChain result;
    QString userId;
    static std::atomic<uint64_t> ownerId;
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

class ChangePermittedChannelCommand: public CommandCreator{
public:
    ChangePermittedChannelCommand() = default;
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


class GemsCommand: public RecommendationsCommand{
public:
    GemsCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};


class CutoffCommand: public CommandCreator{
public:
    CutoffCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class YearCommand: public CommandCreator{
public:
    YearCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class ChangeTargetCommand: public CommandCreator{
public:
    ChangeTargetCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class SendMessageToChannelCommand: public CommandCreator{
public:
    SendMessageToChannelCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class ToggleBanCommand: public CommandCreator{
public:
    ToggleBanCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class StatsCommand: public CommandCreator{
public:
    StatsCommand(){}
    virtual CommandChain ProcessInputImpl(const SleepyDiscord::Message&);
    virtual bool IsThisCommand(const std::string& cmd);
};

class SusCommand: public CommandCreator{
public:
    SusCommand(){}
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
    QStringList reactionsToRemove;
    QSharedPointer<User> user;
    MessageToken originalMessageToken;
    SleepyDiscord::Snowflake<SleepyDiscord::Message> targetMessage;
    SleepyDiscord::Snowflake<SleepyDiscord::Channel> targetChannel;
    ECommandType originalCommandType = ct_none;
    std::list<CommandChain> commandsToReemit;
    QStringList errors;
    bool emptyAction = false;
    bool stopChain = false;
    static QStringList tips;
};

CommandChain CreateRollCommand(QSharedPointer<User> , QSharedPointer<Server> , const MessageToken & );
CommandChain CreateChangeRecommendationsPageCommand(QSharedPointer<User> , QSharedPointer<Server> , const MessageToken & , bool shiftRight = true);
CommandChain CreateChangeHelpPageCommand(QSharedPointer<User> , QSharedPointer<Server> , const MessageToken &, bool shiftRight = true);
CommandChain CreateRemoveReactionCommand(QSharedPointer<User> , QSharedPointer<Server> server, const MessageToken &message, const std::string &reaction);
Command NewCommand(QSharedPointer<discord::Server> server, const SleepyDiscord::Message& message, ECommandType type);
Command NewCommand(QSharedPointer<discord::Server> server, const MessageToken& message, ECommandType type);
}

