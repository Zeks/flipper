/*
Flipper is a recommendation and search engine for fanfiction.net
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
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#pragma once
#include <QDateTime>
#include <QDataStream>
#include <QString>
#include <QDebug>
#include <variant>
#include <string>

namespace sql{


class Variant{
public:
    using InternalVariant = std::variant<std::monostate, std::string, int, uint, int64_t, uint64_t, double, QDateTime, bool, QByteArray>;
    Variant() = default;
    Variant(const std::string&);
    Variant(std::string&&);
    Variant(const char*);
    //Variant(const int32_t&);
    Variant(int32_t);
    //Variant(const int64_t&);
    Variant(int64_t);
    //Variant(const uint32_t&);
    Variant(uint32_t);
    //Variant(const uint64_t&);
    Variant(uint64_t);
    //Variant(const double&);
    Variant(double);
    Variant(const QDateTime&);
    Variant(QDateTime&&);
    Variant(const QDate&);
    Variant(QDate&&);
    Variant(const QString&);
    Variant(QString&&);
    //Variant(const bool&);
    Variant(bool);
    Variant(const QByteArray&);
    Variant(QByteArray&&);
    Variant(const QVariant&);
    Variant(QVariant&&v);



    int toInt(bool* success = nullptr) const;
    uint toUInt(bool* success = nullptr)  const;
    int64_t toInt64(bool* success = nullptr) const;
    uint64_t toUInt64(bool* success = nullptr) const;
    double toDouble(bool* success = nullptr)  const;
    std::string toString()  const;
    float toFloat()  const;
    QDateTime toDateTime() const;
    QDate toDate() const;
    bool toBool() const;
    QByteArray toByteArray() const;
    QVariant toQVariant() const;

    template <typename T>
    T value();
    InternalVariant data;
};
template <> inline
uint64_t Variant::value<uint64_t>(){return toUInt64();}
template <> inline
int Variant::value<int>(){return toInt();}
template <> inline
uint Variant::value<uint>(){return toUInt();}
template <> inline
bool Variant::value<bool>(){return toBool();}
template <> inline
int64_t Variant::value<int64_t>(){return toInt64();}
template <> inline
double Variant::value<double>(){return toDouble();}
template <> inline
std::string Variant::value<std::string>(){return toString();}
template <> inline
QString Variant::value<QString>(){return QString::fromStdString(toString());}
template <> inline
QDateTime Variant::value<QDateTime>(){return toDateTime();}



struct QueryBinding{
    std::string key;
    Variant value;
};

}

template<typename T, typename... Ts> inline
QDebug operator<<(QDebug os, const std::variant<T, Ts...>& v)
{
    std::visit([&os](const auto & arg) {
        using InnerType = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<InnerType, std::string>){
            os << arg.c_str();
        }
        else if constexpr (std::is_same_v<InnerType, std::monostate>){
            os << "empty Variant";
        }
        else
            os << arg;
    }, v);
    return os;
}

inline QDebug operator<<(QDebug debug, const sql::Variant& variant)
{
    QDebugStateSaver saver(debug);
    debug << variant.data;
    return debug;
}



