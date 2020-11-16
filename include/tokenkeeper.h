/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
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
            //QLOG_INFO() << "Acquiring safety token";
            QReadLocker locker(&lock);
            if(tokensInUse.contains(token))
            {
                QLOG_INFO() << "Token already in use, returning nullptr";
                return QSharedPointer<UserToken<T>>();
            }
        }
        {
            //QLOG_INFO() << "Token not in use, creating a new one";
            QWriteLocker locker(&lock);
            tokensInUse.insert(token);
            return QSharedPointer<UserToken<T>>(new UserToken<T>(token));
        }
    }
    void ReleaseToken(QString token) {
        //QLOG_INFO() << "Releasing safety token";
        QWriteLocker locker(&lock);
        tokensInUse.remove(token);
    }
    void LockToken(QString token) {
        //QLOG_INFO() << "Locking safety token";
        QWriteLocker locker(&lock);
        tokensInUse.insert(token);
    }
    QSet<QString> tokensInUse;
    mutable QReadWriteLock lock;
};
template <typename T>
struct UserToken{
    QString token;
    QString subToken;
    UserToken(QString token, QString subToken = ""):token(token), subToken(subToken){}
    virtual ~UserToken(){ReleaseToken();}
    virtual void ReleaseToken(){
        An<TokenKeeper<T>> accessor;
        if(subToken.isEmpty())
            accessor->ReleaseToken(token);
        else
            accessor->ReleaseToken(subToken);
    }
};
using UserTokenizer = TokenKeeper<User>;
BIND_TO_SELF_SINGLE(UserTokenizer);

