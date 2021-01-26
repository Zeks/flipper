#pragma once
#include "discord/tracked-messages/tracked_message_base.h"


namespace discord{

class TrackedFicDetails : public TrackedMessageBase{

public:
    TrackedFicDetails();
    virtual ~TrackedFicDetails(){};
    virtual CommandChain ProcessReactionImpl(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji) override;
    QStringList GetEmojiSet() override;
    std::string GetOtherUserErrorMessage(Client *client) override;
    int GetDataExpirationIntervalS() override;
    CommandChain CloneForOtherUser() override;
    void FillMemo(QSharedPointer<User>) override;
    std::chrono::system_clock::time_point GetDataExpirationPoint() override;
    void RetireData() override;

    int ficId = 0;


};


}

