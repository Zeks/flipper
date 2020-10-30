#pragma once
#include "discord/identity_hash.h"
#include "discord/command_types.h"
#include "discord/discord_message_token.h"
namespace discord {
struct CachedMessageSource{
    MessageToken token;
    ECommandType sourceCommandType;
};

template <>
struct IdentityMatchingDataComparator<CachedMessageSource>{
    static inline bool Compare(const QHash<int64_t,CachedMessageSource>& hash, int64_t key, int64_t value){
        return hash.value(key).token.authorID == value;
    }
};
}
