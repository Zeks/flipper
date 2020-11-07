#pragma once
#include <string>
#include <QByteArray>
#include <QHash>
namespace std
{
    inline namespace __cxx11{
    inline uint qHash(const std::string& key, uint seed = 0)
     {
         return qHash(QByteArray::fromRawData(key.data(), key.length()), seed);
     }
    }
}


