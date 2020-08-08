#include "core/fanfic.h"
#include "core/section.h"


namespace core {
Fic::Fic(){
    author = QSharedPointer<Author>(new Author);
}

void FicDataForRecommendationCreation::Serialize(QDataStream &out)
{
    out << id;
    out << adult;
    out << authorId;
    out << complete;
    out << dead;
    out << favCount;
    out << genreString;
    out << published;
    out << updated;
    out << reviewCount;
    out << sameLanguage;
    out << slash;
    out << wordCount;

    out << fandoms.size();
    for(auto fandom: fandoms)
        out << fandom;
}
void FicDataForRecommendationCreation::Log()
{
    qDebug() << id;
    qDebug() << adult;
    qDebug() << authorId;
    qDebug() << complete;
    qDebug() << dead;
    qDebug() << favCount;
    qDebug() << genreString;
    qDebug() << published;
    qDebug() << updated;
    qDebug() << reviewCount;
    qDebug() << sameLanguage;
    qDebug() << slash;
    qDebug() << wordCount;

    qDebug() << fandoms.size();
    for(auto fandom: fandoms)
        qDebug() << fandom;
}

void FicDataForRecommendationCreation::Deserialize(QDataStream &in)
{
    in >> id;
    //qDebug() << "id: " << id;
    in >> adult;
    //qDebug() << "adult: " << adult;
    in >> authorId;
    //qDebug() << "authorId " << authorId;
    in >> complete;
    //qDebug() << "complete: " << complete;
    in >> dead;
    //qDebug() << "dead: " << dead;
    in >> favCount;
    //qDebug() << "favCount: " << favCount;
    in >> genreString;
    //qDebug() << "genreString: " << genreString;
    in >> published;
    //qDebug() << "published: " << published;
    in >> updated;
    //qDebug() << "updated: " << updated;
    in >> reviewCount;
    //qDebug() << "reviewCount: " << reviewCount;
    in >> sameLanguage;
    //qDebug() << "sameLanguage: " << sameLanguage;
    in >> slash;
    //qDebug() << "slash: " << slash;
    in >> wordCount;
    //qDebug() << "wordCount: " << wordCount;
    int size = -1;
    in >> size;
    if(size > 2 || size < 0)
        qDebug() << "crap fandom size for fic: " << id << " size:" << size;
    int fandom = -1;
    for(int i = 0; i < size; i++)
    {
        in >> fandom;
        fandoms.push_back(fandom);
    }
}

inline QTextStream &operator>>(QTextStream &in, FicDataForRecommendationCreation &p)
{
    QString temp;

    in >> temp;
    p.id = temp.toInt();

    in >> temp;
    p.adult = temp.toInt();

    in >> temp;
    p.authorId = temp.toInt();

    in >> temp;
    p.complete = temp.toInt();

    in >> temp;
    p.dead = temp.toInt();

    in >> temp;
    p.favCount = temp.toInt();

    in >> p.genreString;
    if(p.genreString == "#")
        p.genreString.clear();
    p.genreString.replace("_", " ");

    in >> temp;
    p.published = QDate::fromString("yyyyMMdd");

    in >> temp;
    p.updated = QDate::fromString("yyyyMMdd");

    in >> temp;
    p.reviewCount = temp.toInt();

    in >> temp;
    p.sameLanguage = temp.toInt();

    in >> temp;
    p.slash = temp.toInt();

    in >> temp;
    p.wordCount = temp.toInt();

    in >> temp;
    int fandomSize = temp.toInt();

    for(int i = 0; i < fandomSize; i++)
    {
        in >> temp;
        p.fandoms.push_back(temp.toInt());
    }

    return in;
}

inline QTextStream &operator<<(QTextStream &out, const FicDataForRecommendationCreation &p)
{
    out << QString::number(p.id) << " ";
    out << QString::number(static_cast<int>(p.adult)) << " ";
    out << QString::number(p.authorId) << " ";
    out << QString::number(p.complete) << " ";
    out << QString::number(p.dead) << " ";
    out << QString::number(p.favCount) << " ";
    if(p.genreString.trimmed().isEmpty())
        out << "#" << " ";
    else
        out << p.genreString << " ";
    if(p.published.isValid())
        out << p.published.toString("yyyyMMdd") << " ";
    else
        out << "0" << " ";
    if(p.updated.isValid())
        out << p.updated.toString("yyyyMMdd") << " ";
    else
        out << "0" << " ";

    out << QString::number(p.reviewCount) << " ";
    out << QString::number(p.sameLanguage) << " ";
    out << QString::number(p.slash) << " ";
    out << QString::number(p.wordCount) << " ";

    out << QString::number(p.fandoms.size()) << " ";

    for(auto fandom: p.fandoms)
        out << QString::number(fandom) << " ";
    out << "\n";

    return out;
}

}
