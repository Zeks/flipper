#include "core/author.h"
#include <QDebug>
#include <QDataStream>

namespace core {


void core::Author::Log()
{

    qDebug() << "id: " << id;
    LogWebIds();
    qDebug() << "name: " << name;
    qDebug() << "valid: " << isValid;
    qDebug() << "idStatus: " << static_cast<int>(idStatus);
    qDebug() << "ficCount: " << ficCount;
    qDebug() << "favCount: " << favCount;
    qDebug() << "lastUpdated: " << lastUpdated;
    qDebug() << "firstPublishedFic: " << firstPublishedFic;
}

void core::Author::LogWebIds()
{
    qDebug() << "Author WebIds:" ;
    for(auto key : webIds.keys())
    {
        if(!key.trimmed().isEmpty())
            qDebug() << key << " " << webIds[key];
    }
}

QString core::Author::CreateAuthorUrl(QString urlType, int webId) const
{
    if(urlType == "ffn")
        return "https://www.fanfiction.net/u/" + QString::number(webId);
    return QString();
}

QStringList core::Author::GetWebsites() const
{
    return webIds.keys();
}

void core::Author::Serialize(QDataStream &out)
{
    out << hasChanges;
    out << id;
    out << static_cast<int>(idStatus);
    out << name;
    out << firstPublishedFic;
    out << lastUpdated;
    out << ficCount;
    out << recCount;
    out << favCount;
    out << isValid;
    out << webIds;
    out << static_cast<int>(updateMode);

    stats.Serialize(out);
}

void core::Author::Deserialize(QDataStream &in)
{
    in >> hasChanges;
    in >> id;
    int temp;
    in >> temp;
    idStatus = static_cast<AuthorIdStatus>(temp);

    in >> name;
    in >> firstPublishedFic;
    in >> lastUpdated;
    in >> ficCount;
    in >> recCount;
    in >> favCount;
    in >> isValid;
    in >> webIds;
    in >> temp;
    updateMode = static_cast<UpdateMode>(temp);

    stats.Deserialize(in);
}

}
