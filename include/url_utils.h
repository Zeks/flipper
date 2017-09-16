#pragma once
#include <QString>

namespace url_utils{
QString GetWebId(QString, QString );
QString GetUrlFromWebId(int, QString );
namespace ffn{
    QString GetWebId(QString);
    QString GetUrlFromWebId(int);
}
}

