#pragma once
#include "discord/discord_message_token.h"
#include "discord/command_types.h"
#include "discord/command_chain.h"
#include <QSharedPointer>

namespace discord{
class Client;
class User;
class TrackedMessageBase;
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
    MessageIdToken originalMessageToken;
    std::shared_ptr<TrackedMessageBase> messageData;

    SleepyDiscord::Snowflake<SleepyDiscord::Message> targetMessage;
    SleepyDiscord::Snowflake<SleepyDiscord::Channel> targetChannel;
    ECommandType originalCommandType = ct_none;
    std::list<CommandChain> commandsToReemit;
    QStringList errors;
    bool emptyAction = false;
    bool stopChain = false;
    bool deleteOriginalMessage = false;
    static QStringList tips;
};


}
