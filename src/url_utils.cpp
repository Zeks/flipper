#include "url_utils.h"
#include <QRegExp>
namespace url_utils{

QString GetWebId(QString url, QString source)
{
   if(source == "ffn")
       return ffn::GetWebId(url);
   return "";
}

QString GetUrlFromWebId(int id, QString source)
{
    if(source == "ffn")
        return ffn::GetUrlFromWebId(id);
    return "";
}

namespace ffn{
QString GetWebId(QString url)
{

    QString result;
    QRegExp rxWebId("/s/(\\d+)");
    auto indexWeb = rxWebId.indexIn(url);
    if(indexWeb != -1)
    {
        result = rxWebId.cap(1);
    }
    return result;
}

QString GetUrlFromWebId(int id)
{
    return "https://www.fanfiction.net/s/" + QString::number(id);
}

}



}
