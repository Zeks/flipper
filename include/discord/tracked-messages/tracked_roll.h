#pragma once
#include "discord/tracked-messages/tracked_message_base.h"
#include "discord/rec_message_recreation_token.h"
#include "discord/timed_token.h"
#include "core/recommendation_list.h"
#include "GlobalHeaders/scope_guard.hpp"
#include <QSharedPointer>



namespace discord{

class TrackedRoll: public TrackedMessageBase{

public:
    virtual ~TrackedRoll(){};
    virtual CommandChain ProcessReactionImpl(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji) override;
    QStringList GetEmojiSet() override;
    std::string GetOtherUserErrorMessage(Client *client) override;
    CommandChain CloneForOtherUser() override;
    int GetDataExpirationIntervalS() override;
    std::chrono::system_clock::time_point GetDataExpirationPoint() override;
    void RetireData() override;
    RecsMessageCreationMemo memo;
    TimedEntity<QSharedPointer<core::RecommendationListFicData>> ficData;
};


}
