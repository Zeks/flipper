#include "sql_abstractions/sql_variant.h"

namespace sql{

Variant::Variant(){}
Variant::Variant(const std::string & data):data(data){}
Variant::Variant(const char *data):data(std::string(data)){}
Variant::Variant(const int32_t &data):data(data){}
Variant::Variant(const uint32_t &data):data(static_cast<uint64_t>(data)){}
Variant::Variant(const uint64_t &data):data(data){}
Variant::Variant(const double &data):data(data){}
Variant::Variant(const QDateTime &data):data(data){}
Variant::Variant(const QDate &data):data(QDateTime(data.startOfDay())){}
Variant::Variant(QDate &&data):data(QDateTime(data.startOfDay())){}
Variant::Variant(const QString &data):data(data.toStdString()){}
Variant::Variant(QString && data):data(data.toStdString()){}
Variant::Variant(const bool &data):data(data){}
Variant::Variant(const QByteArray &data):data(data){}
Variant::Variant(std::string &&data):data(data){}
Variant::Variant(int32_t &&data):data(data){}
Variant::Variant(int64_t &&data):data(data){}
Variant::Variant(uint32_t &&data):data(static_cast<uint64_t>(data)){}
Variant::Variant(uint64_t &&data):data(data){}
Variant::Variant(double &&data):data(data){}
Variant::Variant(QDateTime &&data):data(data){}
Variant::Variant(bool &&data):data(data){}
Variant::Variant(QByteArray &&data):data(data){}

Variant::Variant(const QVariant & v)
{
    switch(v.type()){
    case QVariant::Bool:
        data = InternalVariant(v.toBool());
        break;
    case QVariant::Int:
        data = InternalVariant(v.toInt());
        break;
    case QVariant::UInt:
        data = InternalVariant(v.toULongLong());
        break;
    case QVariant::LongLong:
        data = InternalVariant(v.toLongLong());
        break;
    case QVariant::ULongLong:
        data = InternalVariant(v.toULongLong());
        break;
    case QVariant::Date:
        data = InternalVariant(v.toDate().startOfDay());
        break;
    case QVariant::DateTime:
        data = InternalVariant(v.toDateTime());
        break;
    case QVariant::String:
        data = InternalVariant(v.toString().toStdString());
        break;
    case QVariant::ByteArray:
        data = InternalVariant(v.toByteArray());
        break;
    case QVariant::Double:
        data = InternalVariant(v.toDouble());
        break;
    case QVariant::Invalid:
        data = std::monostate();
        break;

    default:
        throw std::logic_error("unexpected variant type not handled in switch:" + QString::number(v.type()).toStdString());
        break;
    }
}

Variant::Variant(QVariant && v) : Variant(v){}

int Variant::toInt(bool* success) const{
    int result = 0;
    std::visit([&](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, int64_t>){
                    result = arg;
                    if(success)
                        *success = true;
                }
                else if constexpr (std::is_same_v<T, uint64_t>){
                    result = arg;
                    if(success)
                        *success = true;
                }
                else if constexpr (std::is_same_v<T, double>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, std::string>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, QDateTime>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, bool>){
                    result = arg > 0;
                }
                else if constexpr (std::is_same_v<T, QByteArray>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, std::monostate>){
                    if(success)
                        *success = false;
                }
                else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
    }, data);
    return result;
}

uint64_t Variant::toUInt64(bool *success) const
{
    int result = 0;
    std::visit([&](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, int64_t>){
                    result = arg; // todo, error, should't be used like that, too lazy to add asert now
                    if(success)
                        *success = true;
                }
                else if constexpr (std::is_same_v<T, uint64_t>){
                    result = arg;
                    if(success)
                        *success = true;
                }
                else if constexpr (std::is_same_v<T, double>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, std::string>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, QDateTime>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, bool>){
                    result = arg > 0;
                }
                else if constexpr (std::is_same_v<T, QByteArray>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, std::monostate>){
                    if(success)
                        *success = false;
                }
                else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
    }, data);
    return result;
}

int64_t Variant::toInt64(bool *success) const
{
    int result = 0;
    std::visit([&](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, int64_t>){
                    result = arg;
                    if(success)
                        *success = true;
                }
                else if constexpr (std::is_same_v<T, uint64_t>){
                    result = arg; // todo, error, should't be used like that, too lazy to add asert now
                    if(success)
                        *success = true;
                }
                else if constexpr (std::is_same_v<T, double>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, std::string>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, QDateTime>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, bool>){
                    result = arg > 0;
                }
                else if constexpr (std::is_same_v<T, QByteArray>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, std::monostate>){
                    if(success)
                        *success = false;
                }
                else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
    }, data);
    return result;
}

uint Variant::toUInt(bool *success) const
{
    uint result = 0;
    std::visit([&](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, int64_t>){
                    result = arg;
                    if(success)
                        *success = true;
                }
                else if constexpr (std::is_same_v<T, uint64_t>){
                    result = arg;
                    if(success)
                        *success = true;
                }
                else if constexpr (std::is_same_v<T, double>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, std::string>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, QDateTime>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, bool>){
                    result = arg > 0;
                }
                else if constexpr (std::is_same_v<T, QByteArray>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, std::monostate>){
                    if(success)
                        *success = false;
                }
                else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
    }, data);
    return result;
}
double Variant::toDouble(bool* success) const
{
    double result = 0.;
    std::visit([&](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, int64_t>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, uint64_t>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, double>){
                    if(success)
                        *success = true;
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, std::string>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, QDateTime>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, bool>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, QByteArray>){
                    if(success)
                        *success = false;
                }
                else if constexpr (std::is_same_v<T, std::monostate>){
                    if(success)
                        *success = false;
                }
                else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
    }, data);
    return result;
}



std::string Variant::toString() const
{
    std::string result;
    std::visit([&](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, int64_t>){
                    result = std::to_string(arg);
                }
                else if constexpr (std::is_same_v<T, uint64_t>){
                    result = std::to_string(arg);
                }
                else if constexpr (std::is_same_v<T, double>){
                    result = std::to_string(arg);
                }
                else if constexpr (std::is_same_v<T, std::string>){
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, QDateTime>){
                    result = arg.toString().toStdString();
                }
                else if constexpr (std::is_same_v<T, bool>){
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, QByteArray>){
                }
                else if constexpr (std::is_same_v<T, std::monostate>){
                }
                else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
    }, data);
    return result;
}

float Variant::toFloat() const
{
    return toDouble();
}

QDateTime Variant::toDateTime() const
{
    QDateTime result;
    std::visit([&](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, int64_t>){
                }
                else if constexpr (std::is_same_v<T, uint64_t>){
                }
                else if constexpr (std::is_same_v<T, double>){
                }
                else if constexpr (std::is_same_v<T, std::string>){
                }
                else if constexpr (std::is_same_v<T, QDateTime>){
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, bool>){
                }
                else if constexpr (std::is_same_v<T, QByteArray>){
                }
                else if constexpr (std::is_same_v<T, std::monostate>){
                }
                else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
    }, data);
    return result;
}

QDate Variant::toDate() const
{
    return toDateTime().date();
}

bool Variant::toBool() const
{
    bool result = false;
    std::visit([&](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, int64_t>){
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, uint64_t>){
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, double>){
                }
                else if constexpr (std::is_same_v<T, std::string>){
                }
                else if constexpr (std::is_same_v<T, QDateTime>){
                }
                else if constexpr (std::is_same_v<T, bool>){
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, QByteArray>){
                }
                else if constexpr (std::is_same_v<T, std::monostate>){
                }
                else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
    }, data);
    return result;
}

QByteArray Variant::toByteArray() const
{
    QByteArray result;
    std::visit([&](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, int64_t>){
                }
                else if constexpr (std::is_same_v<T, uint64_t>){
                }
                else if constexpr (std::is_same_v<T, double>){
                }
                else if constexpr (std::is_same_v<T, std::string>){
                }
                else if constexpr (std::is_same_v<T, QDateTime>){
                }
                else if constexpr (std::is_same_v<T, bool>){
                }
                else if constexpr (std::is_same_v<T, QByteArray>){
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, std::monostate>){
                }
                else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
    }, data);
    return result;
}

QVariant Variant::toQVariant() const
{
    QVariant result;
    std::visit([&](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, int64_t>){
                    result = qint64(arg);
                }
                else if constexpr (std::is_same_v<T, uint64_t>){
                    result = quint64(arg);
                }
                else if constexpr (std::is_same_v<T, double>){
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, std::string>){
                    result = QString::fromStdString(arg);
                }
                else if constexpr (std::is_same_v<T, QDateTime>){
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, bool>){
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, QByteArray>){
                    result = arg;
                }
                else if constexpr (std::is_same_v<T, std::monostate>){
                }
                else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
    }, data);
    return result;
}

}
