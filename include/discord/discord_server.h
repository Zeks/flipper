/*Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#pragma once
#include <QString>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>
#include <QHash>
#include <QDateTime>
#include <chrono>
#include <regex>
#include <set>

#include "GlobalHeaders/SingletonHolder.h"

namespace discord{

struct Server{
    Server(){}


    public:
    std::string GetServerId() const;
    void SetServerId(const std::string& value);

    QString GetServerName() const;
    void SetServerName(const QString& value);

    QString GetOwnerId() const;
    void SetOwnerId(const QString& value);

    std::string GetDedicatedChannelId() const;
    void SetDedicatedChannelId(const std::string& value);

    std::string_view GetCommandPrefix() const;
    void SetCommandPrefix(const std::string& value);

    bool GetIsValid() const;
    void SetIsValid(bool value);

    bool GetBanned() const;
    void SetBanned(bool value);

    bool GetSilenced() const;
    void SetSilenced(bool value);

    bool GetAnswerInPm() const;
    void SetAnswerInPm(bool value);

    int GetParserRequestLimit() const;
    void SetParserRequestLimit(int value);

    int GetTotalRequests() const;
    void SetTotalRequests(int value);

    QDateTime GetFirstActive() const;
    void SetFirstActive(const QDateTime& value);

    QDateTime GetLastActive() const;
    void SetLastActive(const QDateTime& value);

    bool GetAllowedToRemoveReactions() const;
    void SetAllowedToRemoveReactions(bool value);
    void AddForbiddenChannelForSendMessage(std::string);
    void RemoveForbiddenChannelForSendMessage(std::string);
    bool IsAllowedChannelForSendMessage(std::string);
    void ClearForbiddenChannels();

    bool GetAllowedToAddReactions() const;
    void SetAllowedToAddReactions(bool value);

    bool GetAllowedToEditMessages() const;
    void SetAllowedToEditMessages(bool value);

    bool GetShownBannedMessage() const;
    void SetShownBannedMessage(bool value);

private:
    std::string serverId;
    QString serverName;
    QString ownerId;
    std::string dedicatedChannelId;
    std::string commandPrefix = "!";

    bool isValid = false;
    bool banned = false;
    bool shownBannedMessage = false;
    bool silenced = false;
    bool answerInPm = false;
    int parserRequestLimit = 0;
    int totalRequests = 0;
    bool allowedToRemoveReactions = true;
    bool allowedToAddReactions = true;
    bool allowedToEditMessages = true;



    QDateTime firstActive;
    QDateTime lastActive;
    std::set<std::string> forbiddenChannels;

    mutable QReadWriteLock lock = QReadWriteLock(QReadWriteLock::Recursive);
};


struct Servers{
    void AddServer(QSharedPointer<Server>);
    bool HasServer(const std::string&);
    QSharedPointer<Server> GetServer(const std::string&) const;
    bool LoadServer(const std::string&);
    QHash<std::string,QSharedPointer<Server>> servers;
    QReadWriteLock lock;
};
}

BIND_TO_SELF_SINGLE(discord::Servers);
