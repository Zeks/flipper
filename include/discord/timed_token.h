#pragma once 
#include <chrono>
namespace discord{

template class TimedEntity{
    std::chrono::system_clock::time_point expirationPoint;
    T data;
}
}
