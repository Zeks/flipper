#pragma once
#include <string>

namespace sql{

struct ConnectionToken{
    std::string ip;
    uint32_t port = 0;
    std::string serviceName;
    std::string user;
    std::string password;
};

}
