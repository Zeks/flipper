#pragma once

#include <QHash>
#include <QSet>
#include <QReadWriteLock>
#include <QSharedPointer>
#include "GlobalHeaders/SingletonHolder.h"
struct User{};
template <typename T>
struct UserToken;

template <typename T>
class TokenKeeper{
public:
    QSharedPointer<UserToken<T>> GetToken(QString token){
        {
            QReadLocker locker(&lock);
            if(tokensInUse.contains(token))
                return QSharedPointer<UserToken<T>>();
        }
        {
            QWriteLocker locker(&lock);
            tokensInUse.insert(token);
            return QSharedPointer<UserToken<T>>(new UserToken<T>(token));
        }
    }
    void ReleaseToken(QString token) {
        QWriteLocker locker(&lock);
        tokensInUse.remove(token);
    }
    void LockToken(QString token) {
        QWriteLocker locker(&lock);
        tokensInUse.insert(token);
    }
    QSet<QString> tokensInUse;
    mutable QReadWriteLock lock;
};
template <typename T>
struct UserToken{
    QString token;
    UserToken(QString token):token(token){}
    ~UserToken(){}
    virtual void ReleaseToken(){
        An<TokenKeeper<T>> accessor;
        accessor->ReleaseToken(token);
    }
};
using UserTokenizer = TokenKeeper<User>;
BIND_TO_SELF_SINGLE(UserTokenizer);

