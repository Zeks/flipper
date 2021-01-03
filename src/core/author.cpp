#include "core/author.h"
#include <QDebug>
#include <QDataStream>
#include <iostream>

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

void core::Author::LogWebIds() const
{
    qDebug() << "Author WebIds:" ;
    auto itTemp = webIds.cbegin();
    auto itEnd = webIds.cend();
    while(itTemp != itEnd){
        const auto key = itTemp.key();
        if(!key.trimmed().isEmpty())
            qDebug() << key  << " " << webIds[key];
        itTemp++;
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

void core::AuthorStats::Serialize(QDataStream &out)
{
    out << pageCreated;
    out << bioLastUpdated;
    out << favouritesLastUpdated;
    out << favouritesLastChecked;
    out << bioWordCount;

    favouriteStats.Serialize(out);
    ownFicStats.Serialize(out);
}

void core::AuthorStats::Deserialize(QDataStream &in)
{
    in >> pageCreated;
    in >> bioLastUpdated;
    in >> favouritesLastUpdated;
    in >> favouritesLastChecked;
    in >> bioWordCount;

    favouriteStats.Deserialize(in);
    ownFicStats.Deserialize(in);
}
void AuthorFandomStatsForWeightCalc::Serialize(QDataStream &out)
{
      out << listId;
      out << fandomCount;
      out << ficCount;
      out << fandomDiversity;

      out << fandomPresence;
      out << fandomCounts;
}

void AuthorFandomStatsForWeightCalc::Deserialize(QDataStream &in)
{
    in >> listId;
    in >> fandomCount;
    in >> ficCount;
    in >> fandomDiversity;

    in >> fandomPresence;
    in >> fandomCounts;

}
}
