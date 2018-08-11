#pragma once

#include <QHash>
#include <QSet>
#include <QReadWriteLock>
#include <QSharedPointer>
#include "GlobalHeaders/SingletonHolder.h"
#include "logger/QsLog.h"
struct User{};
template <typename T>
struct UserToken;

template <typename T>
class TokenKeeper{
public:
    QSharedPointer<UserToken<T>> GetToken(QString token){
        {
            QLOG_INFO() << "Acquiring safety token";
            QReadLocker locker(&lock);
            if(tokensInUse.contains(token))
            {
                QLOG_INFO() << "Token already in use, returning nullptr";
                return QSharedPointer<UserToken<T>>();
            }
        }
        {
            QLOG_INFO() << "Token not in use, creating a new one";
            QWriteLocker locker(&lock);
            tokensInUse.insert(token);
            return QSharedPointer<UserToken<T>>(new UserToken<T>(token));
        }
    }
    void ReleaseToken(QString token) {
        QLOG_INFO() << "Releasing safety token";
        QWriteLocker locker(&lock);
        tokensInUse.remove(token);
    }
    void LockToken(QString token) {
        QLOG_INFO() << "Locking safety token";
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
    virtual ~UserToken(){ReleaseToken();}
    virtual void ReleaseToken(){
        An<TokenKeeper<T>> accessor;
        accessor->ReleaseToken(token);
    }
};
using UserTokenizer = TokenKeeper<User>;
BIND_TO_SELF_SINGLE(UserTokenizer);

