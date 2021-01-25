#pragma once
#include "discord/tracked-messages/tracked_message_base.h"


namespace discord{

class TrackedHelpPage : public TrackedMessageBase{

public:
    virtual ~TrackedHelpPage(){};
    virtual CommandChain ProcessReactionImpl(Client* client, QSharedPointer<User>,
                                         SleepyDiscord::Emoji emoji) override;
    QStringList GetEmojiSet() override;
    std::string GetOtherUserErrorMessage(Client *client) override;
    int GetDataExpirationIntervalS() override;
    CommandChain CloneForOtherUser() override;
    int currenHelpPage = 0;
};


}
