#pragma once
#include "discord/tracked-messages/tracked_message_base.h"


namespace discord{

class TrackedRoll: public TrackedMessageBase{

public:
    virtual ~TrackedRoll(){};
    virtual CommandChain ProcessReactionImpl(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji) override;
    QStringList GetEmojiSet() override;
    std::string GetOtherUserErrorMessage(Client *client) override;
    CommandChain CloneForOtherUser() override;
    int currenHelpPage = 0;
};


}
