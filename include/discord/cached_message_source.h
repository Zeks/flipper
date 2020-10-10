#pragma once
#include "discord/identity_hash.h"
#include "discord/command_types.h"
namespace discord {
struct CachedMessageSource{
    int64_t authorId;
    ECommandType sourceCommandType;
};

template <>
struct IdentityMatchingDataComparator<CachedMessageSource>{
    static inline bool Compare(const QHash<int64_t,CachedMessageSource>& hash, int64_t key, int64_t value){
        return hash[key].authorId == value;
    }
};
}
