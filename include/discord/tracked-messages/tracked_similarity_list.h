#pragma once
#include "discord/tracked-messages/tracked_message_base.h"
#include "discord/tracked-messages/tracked_recommendation_list.h"
#include "discord/rec_message_recreation_token.h"
#include "discord/timed_token.h"
#include "core/recommendation_list.h"
//#include "GlobalHeaders/scope_guard.hpp"
#include <QSharedPointer>



namespace discord{

class TrackedSimilarityList: public TrackedRecommendationList{

public:
    TrackedSimilarityList();
    TrackedSimilarityList(QString fic){ficId = fic; canBeUsedAsLastPage = false;}
    virtual ~TrackedSimilarityList(){};
    virtual CommandChain ProcessReactionImpl(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji) override;
    QStringList GetEmojiSet() override;
    std::string GetOtherUserErrorMessage(Client *client) override;
    CommandChain CloneForOtherUser() override;
    int GetDataExpirationIntervalS() override;
    std::chrono::system_clock::time_point GetDataExpirationPoint() override;
    void RetireData() override;
    std::shared_ptr<TrackedMessageBase> NewInstance() override{return std::make_shared<TrackedSimilarityList>();};
    QString ficId;

};


}

