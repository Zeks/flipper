#pragma once
#include "discord/tracked-messages/tracked_message_base.h"


namespace discord{

class TrackedFicDetails : public TrackedMessageBase{

public:
    TrackedFicDetails(QSharedPointer<User> user);
    virtual ~TrackedFicDetails(){};
    virtual CommandChain ProcessReactionImpl(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji) override;
    QStringList GetEmojiSet() override;
    std::string GetOtherUserErrorMessage(Client *client) override;
    int GetDataExpirationIntervalS() override;
    CommandChain CloneForOtherUser() override;
    std::chrono::system_clock::time_point GetDataExpirationPoint() override;
    void RetireData() override;
    std::shared_ptr<TrackedMessageBase> NewInstance() override{return std::make_shared<TrackedFicDetails>(this->originalUser);}

    int ficId = 0;
protected:
    TrackedFicDetails(){};


};


}

