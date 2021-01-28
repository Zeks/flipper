#pragma once
#include "discord/tracked-messages/tracked_message_base.h"


namespace discord{

class TrackedHelpPage : public TrackedMessageBase{

public:
    TrackedHelpPage();
    virtual ~TrackedHelpPage(){};
    virtual CommandChain ProcessReactionImpl(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji) override;
    QStringList GetEmojiSet() override;
    std::string GetOtherUserErrorMessage(Client *client) override;
    int GetDataExpirationIntervalS() override;
    CommandChain CloneForOtherUser() override;
    std::chrono::system_clock::time_point GetDataExpirationPoint() override;
    void RetireData() override;
    std::shared_ptr<TrackedMessageBase> NewInstance() override{return std::make_shared<TrackedHelpPage>();};
    std::atomic<int> currenHelpPage = 0;


};


}
