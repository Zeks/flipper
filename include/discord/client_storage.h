#pragma once
#include "GlobalHeaders/SingletonHolder.h"
#include "discord/identity_hash.h"
#include "discord/discord_server.h"
#include "discord/cached_message_source.h"
#include "discord/tracked-messages/tracked_message_base.h"
#include <QReadWriteLock>

namespace  discord{

struct ChannelSet{
    inline void push(int64_t channelId){
        QWriteLocker locker(&lock);
        set.insert(channelId);
    }
    inline bool contains(int64_t channelId){
        QReadLocker locker(&lock);
        return set.contains(channelId);
    }
    QSet<int64_t> set;
    QReadWriteLock lock;
};


struct ClientStorage{
    QReadWriteLock lock;
    QSharedPointer<discord::Server> fictionalDMServer;
    BotIdentityMatchingHash<CachedMessageSource> messageSourceAndTypeHash;
    BotIdentityMatchingHash<std::shared_ptr<TrackedMessageBase>> messageData;
    BotIdentityMatchingHash<int64_t> channelToServerHash;
    ChannelSet nonPmChannels;
};
}

BIND_TO_SELF_SINGLE(discord::ClientStorage);
