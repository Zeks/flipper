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

    std::string Server::GetDedicatedChannelId() const
    {
        QReadLocker locker(&lock);
        return dedicatedChannelId;
    }

    void Server::SetDedicatedChannelId(const std::string& value)
    {
        QWriteLocker locker(&lock);
        dedicatedChannelId = value;
    }

    std::string_view Server::GetCommandPrefix() const
    {
        QReadLocker locker(&lock);
        return commandPrefix;
    }

    void Server::SetCommandPrefix(const std::string& value)
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
    
    bool Server::GetAllowedToRemoveReactions() const
    {
        QReadLocker locker(&lock);
        return allowedToRemoveReactions;
    }
    
    void Server::SetAllowedToRemoveReactions(bool value)
    {
        QWriteLocker locker(&lock);
        allowedToRemoveReactions = value;
    }

    void Server::AddForbiddenChannelForSendMessage(std::string channel)
    {
        QWriteLocker locker(&lock);
        forbiddenChannels.insert(channel);
    }

    void Server::RemoveForbiddenChannelForSendMessage(std::string channel)
    {
        QWriteLocker locker(&lock);
        forbiddenChannels.erase(channel);
    }

    bool Server::IsAllowedChannelForSendMessage(std::string channel)
    {
        QReadLocker locker(&lock);
        return forbiddenChannels.find(channel) == forbiddenChannels.end();
    }

    void Server::ClearForbiddenChannels()
    {
        QWriteLocker locker(&lock);
        forbiddenChannels.clear();
    }

    bool Server::GetAllowedToAddReactions() const
    {
        QReadLocker locker(&lock);
        return allowedToAddReactions;
    }

    void Server::SetAllowedToAddReactions(bool value)
    {
        QWriteLocker locker(&lock);
        allowedToAddReactions = value;
    }

    bool Server::GetAllowedToEditMessages() const
    {
        QReadLocker locker(&lock);
        return allowedToEditMessages;
    }

    void Server::SetAllowedToEditMessages(bool value)
    {
        QWriteLocker locker(&lock);
        allowedToEditMessages = value;
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

    QSharedPointer<Server> Servers::GetServer(const std::string& server) const
    {
        if(!servers.contains(server))
            return {};
        return servers[server];
    }

    bool Servers::LoadServer(const std::string& server_id)
    {
        auto dbToken = An<discord::DatabaseVendor>()->GetDatabase(QStringLiteral("users"));
        auto server = database::discord_queries::GetServer(dbToken->db, server_id).data;
        if(!server)
            return false;

        servers[server_id] = server;
        return true;
    }

}
