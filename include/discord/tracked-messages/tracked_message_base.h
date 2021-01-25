#pragma once
#include "discord/discord_message_token.h"
#include "discord/command.h"


namespace discord{

class Server;
class Client;

class TrackedMessageBase{
    public:
    enum ENonOriginalUserBehaviour{
        noub_error = 0,
        noub_clone = 1,
        noub_legal = 2,
    };
    virtual ~TrackedMessageBase(){};
    // base functions
    bool IsOriginaluser(QString user){return user.toStdString() == token.authorID.string();};
    CommandChain ProcessReaction(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji);
    static std::string CreateMention(const std::string& string){
        return "<@" + string + ">";
    }

    // virtual functions
    virtual std::string GetOtherUserErrorMessage(Client* client) = 0;
    virtual CommandChain CloneForOtherUser() = 0;
        virtual CommandChain ProcessReactionImpl(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji) = 0;
    virtual QStringList GetEmojiSet() = 0;

    ENonOriginalUserBehaviour otherUserBehaviour = noub_error;
    MessageIdToken token;
};


}
