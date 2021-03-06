#pragma once
#include "discord/tracked-messages/tracked_recommendation_list.h"
#include "discord/rec_message_recreation_token.h"
#include "discord/timed_token.h"
#include "core/recommendation_list.h"
//#include "GlobalHeaders/scope_guard.hpp"
#include <QSharedPointer>



namespace discord{

class TrackedRoll: public TrackedRecommendationList{

public:
    TrackedRoll(QSharedPointer<User> user);
    virtual ~TrackedRoll(){};
    virtual CommandChain ProcessReactionImpl(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji) override;
    QStringList GetEmojiSet() override;
    std::string GetOtherUserErrorMessage(Client *client) override;
    CommandChain CloneForOtherUser() override;
    std::shared_ptr<TrackedMessageBase> NewInstance() override{return std::make_shared<TrackedRoll>(this->originalUser);};
};


}
