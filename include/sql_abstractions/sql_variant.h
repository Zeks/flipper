#pragma once
#include <QDateTime>
#include <QDataStream>
#include <QString>
#include <QDebug>
#include <variant>
#include <string>

namespace sql{

template<class> inline constexpr bool always_false_v = false;
class Variant{
public:
    Variant();
    Variant(const std::string&);
    Variant(const char*);
    Variant(const int32_t&);
    Variant(const int64_t&);
    Variant(const uint32_t&);
    Variant(const uint64_t&);
    Variant(const double&);
    Variant(const QDateTime&);
    Variant(const QDate&);
    Variant(const QString&);
    Variant(const bool&);
    Variant(const QByteArray&);
    Variant(std::string&&);
    Variant(int32_t&&);
    Variant(int64_t&&);
    Variant(uint32_t&&);
    Variant(uint64_t&&);
    Variant(double&&);
    Variant(QDateTime&&);
    Variant(QDate&&);
    Variant(bool&&);
    Variant(QByteArray&&);
    using InternalVariant = std::variant<std::string, int64_t, uint64_t, double, QDateTime, bool, QByteArray>;

    int toInt(bool* success = nullptr) const;
    uint toUInt(bool* success = nullptr)  const;
    double toDouble(bool* success = nullptr)  const;
    std::string toString()  const;
    float toFloat()  const;
    QDateTime toDateTime() const;
    QDate toDate() const;
    bool toBool() const;
    QByteArray toByteArray() const;

    template <typename T>
    T value();
    InternalVariant data;
};
template <> inline
uint64_t Variant::value<uint64_t>(){return toInt();}
template <> inline
bool Variant::value<bool>(){return toBool();}
template <> inline
int64_t Variant::value<int64_t>(){return toInt();}
template <> inline
double Variant::value<double>(){return toDouble();}
template <> inline
std::string Variant::value<std::string>(){return toString();}
template <> inline
QDateTime Variant::value<QDateTime>(){return toDateTime();}

}


//template<typename T, typename... Ts> inline
//QDataStream& operator<<(QDataStream& os, const std::variant<T, Ts...>& v)
//{
//    std::visit([&os](const auto & arg) {
//        using X = std::decay_t<decltype(arg)>;
//        if constexpr(std::is_same_v<X, long int>)
//            os << qint64(arg);
//        if constexpr(std::is_same_v<X, uint64_t>)
//            os << quint64(arg);

//    }, v);
//    return os;
//}

//inline QDataStream &operator<<(QDataStream & out, const sql::Variant& variant){
//   out << variant.data;
//   return out;
//}

template<typename T, typename... Ts> inline
QDebug operator<<(QDebug os, const std::variant<T, Ts...>& v)
{
    std::visit([&os](const auto & arg) {
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



