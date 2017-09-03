#include "url_utils.h"
#include <QRegExp>
namespace core{

QString FFNUrlUtils::GetWebId(QString url)
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

}
