#pragma once
#include "discord/tracked-messages/tracked_message_base.h"
#include "discord/rec_message_recreation_token.h"
#include "discord/timed_token.h"
#include "core/recommendation_list.h"
//#include "GlobalHeaders/scope_guard.hpp"
#include <QSharedPointer>



namespace discord{

class TrackedReview : public TrackedMessageBase{

public:
    TrackedReview(std::vector<std::string> reviews, QSharedPointer<User> user);
    int GetDataExpirationIntervalS() override;
    void RetireData() override;
    //void FillMemo(QSharedPointer<User>) override;

    // own functions redefined in roll and similarity list
    std::string GetOtherUserErrorMessage(Client *client) override;
    CommandChain CloneForOtherUser() override;
    CommandChain ProcessReactionImpl(Client *client, QSharedPointer<User>, SleepyDiscord::Emoji emoji) override;
    QStringList GetEmojiSet() override;
    std::shared_ptr<TrackedMessageBase> NewInstance() override{return std::make_shared<TrackedReview>(std::vector<std::string>{}, this->originalUser);};


    std::atomic<int> currentPosition = 0;
    std::vector<std::string> reviews;
};


}
