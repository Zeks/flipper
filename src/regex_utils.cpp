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
RegularExpressionToken operator"" _s ( const char* data, size_t )
{
    return RegularExpressionToken(data, 0);
}
RegularExpressionToken operator"" _c ( const char* data, size_t )
{
    return RegularExpressionToken(data, 1);
}

QString BouncingSearch(QString str, FieldSearcher finder)
{
    auto tokens = finder.tokens;
    QString reversed = str;
    std::reverse(reversed.begin(), reversed.end());
    QStringRef directRef(&str);
    QStringRef reversedRef(&reversed);
    QStringRef currentString;
    int lastPosition = 0;
    int skip = 0;
    bool found = true;
    int originalSize;
    for(auto token: tokens)
    {

        QRegularExpression rx;
        if(token.forwardDirection)
        {
            currentString = directRef;
            rx.setPattern(token.regex);
            lastPosition = reversedRef.size() - skip;
        }
        else{
            currentString = reversedRef;
            rx.setPattern(token.reversedRegex);
            lastPosition = skip;
        }
        auto match = rx.match(currentString);
        if(match.isValid())
        {
            skip = match.capturedStart() + token.moveAmount;
            originalSize = reversedRef.size();
            if(token.forwardDirection)
            {
                if(token.snapLeftBound)
                {
                    directRef = directRef.mid(skip);
                    reversedRef = reversedRef.mid(0, (originalSize - skip));
                }
                else
                {
                    directRef = directRef.mid(0, skip);
                    reversedRef = reversedRef.mid(originalSize - skip);
                }

            }
            else
            {

                if(token.snapLeftBound)
                {
                    reversedRef = reversedRef.mid(skip);
                    directRef = directRef.mid(0, (originalSize - skip));
                }
                else
                {
                    reversedRef = reversedRef.mid(0, skip);
                    directRef = directRef.mid(originalSize - skip);

                }
            }
        }
        else
        {
            found = false;
            break;
        }
    }
    QString result;
    if(found)
    {
        result = directRef.toString();
    }
    return result;
}
