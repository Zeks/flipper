#pragma once
#include "discord/discord_message_token.h"
#include "discord/command.h"
#include <mutex>
namespace discord{

class Server;
class Client;
class ResultGuard;
class TrackedMessageBase : public std::enable_shared_from_this<TrackedMessageBase>{
    public:
    enum ENonOriginalUserBehaviour{
        noub_error = 0,
        noub_clone = 1,
        noub_legal = 2,
    };
    TrackedMessageBase(QSharedPointer<User> user){originalUser = user;};
    virtual ~TrackedMessageBase(){};
    // base functions
    bool IsOriginaluser(QString user){return user.toStdString() == token.authorID.string();};
    bool CanBeUsedAsLastPage(){return canBeUsedAsLastPage;};
    CommandChain ProcessReaction(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji);
    static std::string CreateMention(const std::string& string){
        return "<@" + string + ">";
    }

    // virtual functions
    virtual std::string GetOtherUserErrorMessage(Client* client) = 0;
    virtual int GetDataExpirationIntervalS() = 0;
    //virtual void FillMemo(QSharedPointer<User>) = 0;
    virtual std::chrono::system_clock::time_point GetDataExpirationPoint(){return std::chrono::high_resolution_clock::now();};
    virtual void RetireData(){};
    virtual CommandChain CloneForOtherUser() = 0;
        virtual CommandChain ProcessReactionImpl(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji) = 0;
    virtual QStringList GetEmojiSet() = 0;
    virtual std::shared_ptr<TrackedMessageBase> NewInstance() = 0;

    ENonOriginalUserBehaviour otherUserBehaviour = noub_error;
    MessageIdToken token;

    std::shared_ptr<ResultGuard> LockResult(){return std::make_shared<ResultGuard>(this);};
    std::atomic<bool> resultInUse = false;
    bool canBeUsedAsLastPage = true;
    std::vector<std::string> actionableEmoji;
    QSharedPointer<User> originalUser;
    std::mutex mutex;
protected:
    TrackedMessageBase(){};

};

class ResultGuard{
public:
    ResultGuard(TrackedMessageBase* target): lock(target->mutex){
    };
    TrackedMessageBase* target = nullptr;
    const std::lock_guard<std::mutex> lock;
};
}
