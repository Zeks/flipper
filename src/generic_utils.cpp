#include "include/generic_utils.h"

QString MicrosecondsToString(int value)
{
    QString decimal = QString::number(value/1000000);
    int offset = decimal == "0" ? 0 : decimal.length();
    QString partial = QString::number(value).mid(offset,1);
    return decimal + "." + partial;
}
