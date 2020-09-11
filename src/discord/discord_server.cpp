#include "discord/discord_server.h"
#include "discord/discord_init.h"
#include "discord/db_vendor.h"
#include "sql/discord/discord_queries.h"

namespace discord {

    std::string Server::GetServerId() const
    {
        QReadLocker locker(&lock);
        return serverId;
    }

    void Server::SetServerId(const std::string& value)
    {
        QWriteLocker locker(&lock);
        serverId = value;
    }

    QString Server::GetServerName() const
    {
        QReadLocker locker(&lock);
        return serverName;
    }

    void Server::SetServerName(const QString& value)
    {
        QWriteLocker locker(&lock);
        serverName = value;
    }

    QString Server::GetOwnerId() const
    {
        QReadLocker locker(&lock);
        return ownerId;
    }

    void Server::SetOwnerId(const QString& value)
    {
        QWriteLocker locker(&lock);
        ownerId = value;
    }

    QString Server::GetDedicatedChannelId() const
    {
        QReadLocker locker(&lock);
        return dedicatedChannelId;
    }

    void Server::SetDedicatedChannelId(const QString& value)
    {
        QWriteLocker locker(&lock);
        dedicatedChannelId = value;
    }

    QString Server::GetCommandPrefix() const
    {
        QReadLocker locker(&lock);
        return commandPrefix;
    }

    void Server::SetCommandPrefix(const QString& value)
    {
        QWriteLocker locker(&lock);
        commandPrefix = value;
    }

    bool Server::GetIsValid() const
    {
        QReadLocker locker(&lock);
        return isValid;
    }

    void Server::SetIsValid(bool value)
    {
        QWriteLocker locker(&lock);
        isValid = value;
    }

    bool Server::GetBanned() const
    {
        QReadLocker locker(&lock);
        return banned;
    }

    void Server::SetBanned(bool value)
    {
        QWriteLocker locker(&lock);
        banned = value;
    }

    bool Server::GetSilenced() const
    {
        QReadLocker locker(&lock);
        return silenced;
    }

    void Server::SetSilenced(bool value)
    {
        QWriteLocker locker(&lock);
        silenced = value;
    }

    bool Server::GetAnswerInPm() const
    {
        QReadLocker locker(&lock);
        return answerInPm;
    }

    void Server::SetAnswerInPm(bool value)
    {
        QWriteLocker locker(&lock);
        answerInPm = value;
    }

    int Server::GetParserRequestLimit() const
    {
        QReadLocker locker(&lock);
        return parserRequestLimit;
    }

    void Server::SetParserRequestLimit(int value)
    {
        QWriteLocker locker(&lock);
        parserRequestLimit = value;
    }

    int Server::GetTotalRequests() const
    {
        QReadLocker locker(&lock);
        return totalRequests;
    }

    void Server::SetTotalRequests(int value)
    {
        QWriteLocker locker(&lock);
        totalRequests = value;
    }

    QDateTime Server::GetFirstActive() const
    {
        QReadLocker locker(&lock);
        return firstActive;
    }

    void Server::SetFirstActive(const QDateTime& value)
    {
        QWriteLocker locker(&lock);
        firstActive = value;
    }

    QDateTime Server::GetLastActive() const
    {
        QReadLocker locker(&lock);
        return lastActive;
    }

    void Server::SetLastActive(const QDateTime& value)
    {
        QWriteLocker locker(&lock);
        lastActive = value;
    }

    const std::regex& Server::GetQuickCommandIdentifier() const
    {
        QReadLocker locker(&lock);
        return commandIdentifier;
    }

    void Server::SetQuickCommandIdentifier(const std::regex& value)
    {
        QWriteLocker locker(&lock);
        commandIdentifier = value;
    }

    void Servers::AddServer(QSharedPointer<Server> server)
    {
        QWriteLocker locker(&lock);
        servers[server->GetServerId()] = server;
    }

    bool Servers::HasServer(const std::string& server)
    {
        QReadLocker locker(&lock);

        if(servers.contains(server))
            return true;
        return false;
    }

    QSharedPointer<Server> Servers::GetServer(const std::string& server)
    {
        if(!servers.contains(server))
            return {};
        return servers[server];
    }

    bool Servers::LoadServer(const std::string& server_id)
    {
        auto dbToken = An<discord::DatabaseVendor>()->GetDatabase("users");
        auto server = database::discord_queries::GetServer(dbToken->db, server_id).data;
        if(!server)
            return false;
        auto regex = GetSimpleCommandIdentifierPrefixless();
        server->SetQuickCommandIdentifier(std::regex((server->GetCommandPrefix() + regex).toStdString()));

        servers[server_id] = server;
        return true;
    }

}
