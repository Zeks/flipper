#pragma once
#include "discord/tracked-messages/tracked_message_base.h"
#include "discord/rec_message_recreation_token.h"
#include "discord/timed_token.h"
#include "core/recommendation_list.h"
//#include "GlobalHeaders/scope_guard.hpp"
#include <QSharedPointer>



namespace discord{

class TrackedRecommendationList: public TrackedMessageBase{

public:
    TrackedRecommendationList(QSharedPointer<User> user);
    int GetDataExpirationIntervalS() override;
    std::chrono::system_clock::time_point GetDataExpirationPoint() override;
    void RetireData() override;
    //void FillMemo(QSharedPointer<User>) override;

    // own functions redefined in roll and similarity list
    std::string GetOtherUserErrorMessage(Client *client) override;
    CommandChain CloneForOtherUser() override;
    CommandChain ProcessReactionImpl(Client *client, QSharedPointer<User>, SleepyDiscord::Emoji emoji) override;
    QStringList GetEmojiSet() override;
    std::shared_ptr<TrackedMessageBase> NewInstance() override{return std::make_shared<TrackedRecommendationList>(this->originalUser);};


    RecsMessageCreationMemo memo;
    TimedEntity<QSharedPointer<core::RecommendationListFicData>> ficData;

};


}
