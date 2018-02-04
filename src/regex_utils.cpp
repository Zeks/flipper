/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
#include "include/regex_utils.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
NarrowResult GetNextInstanceOf(QString text, QString regex1, QString regex2, bool forward)
{
    QRegExp rx1(regex1);
    rx1.setMinimal(true);
    QRegExp rx2(regex2);
    rx2.setMinimal(true);
    int first = -1;
    int second = -1;
    int secondCandidate = -1;
    auto invalid = NarrowResult(-1,-1);
    if(regex1.isEmpty())
        first = text.length();
    else
        first = rx1.indexIn(text);
    if(first == -1)
        return invalid;

    if(forward)
    {
        second = rx2.indexIn(text, first);
        return {first,second};
    }
    else
    {
        if(regex1.isEmpty())
            first = text.length();
        do
        {
            second = secondCandidate;
            secondCandidate = rx2.indexIn(text, second + 1);
            if(secondCandidate == -1)
                break;
        }while(secondCandidate < first);
        if(second == -1)
            return invalid;
        return {first,second};
    }
}
QString GetSingleNarrow(QString text,QString regex1, QString regex2, bool forward1)
{
    QString result;
    auto firstNarrow = GetNextInstanceOf(text,regex1, regex2, forward1);
    result = text.mid(firstNarrow.first,firstNarrow.Length());
    return result;
}


QString GetDoubleNarrow(QString text,
                        QString regex1, QString regex2, bool forward1,
                        QString regex3, QString regex4, bool forward2,
                        int lengthOfLastTag)
{
    QString result;
    auto firstNarrow = GetNextInstanceOf(text,regex1, regex2, forward1);

    QString temp = text.mid(firstNarrow.first,firstNarrow.Length());
    auto secondNarrow = GetNextInstanceOf(temp,regex3,regex4, false);
    if(!firstNarrow.IsValid() || !secondNarrow.IsValid() )
        return result;
    result = temp.mid(secondNarrow.second + lengthOfLastTag, firstNarrow.second);
    return result;
}
