#pragma once
#include <QHash>
#include <QReadWriteLock>
namespace discord {
template <typename T>
struct IdentityMatchingDataComparator{};

template <>
struct IdentityMatchingDataComparator<int64_t>{
    static inline bool Compare(const QHash<int64_t,int64_t>& hash, int64_t key, int64_t value){
        return hash.value(key)== value;
    }
};


template <typename T>
struct BotIdentityMatchingHash{
    inline void push(int64_t keyId, T value){
        QWriteLocker locker(&lock);
        hash.insert(keyId, value);
    }
    inline bool contains(int64_t keyId){
        QReadLocker locker(&lock);
        return hash.contains(keyId);
    }
    inline bool same_user(int64_t keyId, int64_t userId){
        QReadLocker locker(&lock);
        bool result = IdentityMatchingDataComparator<T>::Compare(hash, keyId, userId);
        return result;
    }
    T value(int64_t keyId){
        QReadLocker locker(&lock);
        return hash.value(keyId);
    }
    QHash<int64_t,T> hash;
    QReadWriteLock lock;
};

}
