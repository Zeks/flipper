/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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
#include "url_utils.h"
#include <QRegExp>
#include <QRegularExpression>
namespace url_utils{

QString GetWebId(QString url, QString source)
{
    if(source == QStringLiteral("ffn"))
        return ffn::GetWebId(url);
    return "";
}

QString GetStoryUrlFromWebId(int id, QString source)
{
    if(source == QStringLiteral("ffn"))
        return ffn::GetStoryUrlFromWebId(id);
    return "";
}
QString GetAuthorUrlFromWebId(int id, QString source)
{
    if(source == QStringLiteral("ffn"))
        return ffn::GetAuthorUrlFromWebId(id);
    return "";
}

namespace ffn{
QString GetWebId(QString url)
{

    QString result;
    thread_local QRegExp rxWebId(QStringLiteral("/(s|u)/(\\d+)"));
    auto indexWeb = rxWebId.indexIn(url);
    if(indexWeb != -1)
    {
        result = rxWebId.cap(2);
    }
    return result;
}

QString GetStoryUrlFromWebId(int id)
{
    return "https://www.fanfiction.net/s/" + QString::number(id);
}
QString GetAuthorUrlFromWebId(int id)
{
    return "https://www.fanfiction.net/u/" + QString::number(id);
}

int GetLastPageIndex(QString url)
{
    QRegularExpression rx(QStringLiteral("&p=(\\d+)"));
    auto match = rx.match(url);
    if(!match.hasMatch())
        return 1;
    int result = match.captured(1).toInt();
    return result;
}

}

QString AppendBase(QString website, QString postfix)
{
    if(website == QStringLiteral("ffn"))
        return QStringLiteral("https://www.fanfiction.net") + postfix;
    return postfix;
}

int GetLastPageIndex(QString url)
{
    if(url.contains(QStringLiteral("fanfiction.net")))
        return ffn::GetLastPageIndex(url);
    return 1;
}



}
