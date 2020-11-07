#pragma once
#include <QDebug>
#include <string>
inline QDebug operator<<(QDebug debug, const std::string& string)
{
    QDebugStateSaver saver(debug);
    debug << QString::fromStdString(string);
    return debug;
}
