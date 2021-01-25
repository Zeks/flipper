#pragma once 
#include <chrono>
namespace discord{

template<typename T>
struct TimedEntity{
    std::chrono::system_clock::time_point expirationPoint;
    T data;
};
}
