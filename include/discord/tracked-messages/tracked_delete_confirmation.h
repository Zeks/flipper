#pragma once
#include "discord/tracked-messages/tracked_message_base.h"
#include "discord/rec_message_recreation_token.h"
#include "discord/timed_token.h"
#include "core/recommendation_list.h"
//#include "GlobalHeaders/scope_guard.hpp"
#include <QSharedPointer>



namespace discord{

class TrackedDeleteConfirmation: public TrackedMessageBase{

public:
    TrackedDeleteConfirmation(QString type, QString identifier, QSharedPointer<User> user);
    virtual ~TrackedDeleteConfirmation(){};
    virtual CommandChain ProcessReactionImpl(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji) override;
    int GetDataExpirationIntervalS() override;
    std::chrono::system_clock::time_point GetDataExpirationPoint() override;
    QStringList GetEmojiSet() override;
    std::string GetOtherUserErrorMessage(Client *client) override;
    CommandChain CloneForOtherUser() override;
    std::shared_ptr<TrackedMessageBase> NewInstance() override{return std::make_shared<TrackedDeleteConfirmation>("", "",this->originalUser);};
    QString entityType;
    QString entityId;
    MessageIdToken spawnerToken;
};


}
