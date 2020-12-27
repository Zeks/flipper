#pragma once
#include <string>

namespace sql{

struct ConnectionToken{
    ConnectionToken() = default;
    // constructor for postgres
    ConnectionToken(std::string serviceName, std::string host, int port, std::string user, std::string password){
        tokenType="PQXX";
        this->serviceName = serviceName;
        this->ip = host;
        this->port = port;
        this->user = user;
        this->password = password;
    }
    // constructor for sqlite
    ConnectionToken(std::string serviceName, std::string initFileName, std::string folder){
        tokenType="QSQLITE";
        this->serviceName = serviceName;
        this->initFileName = initFileName;
        this->folder = folder;
    }
    std::string serviceName;

    // pg needs these
    std::string ip;
    uint32_t port = 0;
    std::string user;
    std::string password;

    // sqlite needs these
    std::string initFileName;
    std::string folder;

    // type of the token
    std::string tokenType;
};

}
