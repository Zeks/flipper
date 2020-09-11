#pragma once
#include <QString>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>
#include <QHash>
#include <QDateTime>
#include <chrono>
#include <regex>

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

    QString GetDedicatedChannelId() const;
    void SetDedicatedChannelId(const QString& value);

    QString GetCommandPrefix() const;
    void SetCommandPrefix(const QString& value);

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

    const std::regex& GetQuickCommandIdentifier() const;
    void SetQuickCommandIdentifier(const std::regex& value);

    private:
    std::string serverId;
    QString serverName;
    QString ownerId;
    QString dedicatedChannelId;
    QString commandPrefix = "!";
    std::regex commandIdentifier;

    bool isValid = false;
    bool banned = false;
    bool silenced = false;
    bool answerInPm = false;
    int parserRequestLimit = 0;
    int totalRequests = 0;

    QDateTime firstActive;
    QDateTime lastActive;


    mutable QReadWriteLock lock;
};


struct Servers{
    void AddServer(QSharedPointer<Server>);
    bool HasServer(const std::string&);
    QSharedPointer<Server> GetServer(const std::string&);
    bool LoadServer(const std::string&);
    QHash<std::string,QSharedPointer<Server>> servers;
    QReadWriteLock lock;
};
}

BIND_TO_SELF_SINGLE(discord::Servers);
