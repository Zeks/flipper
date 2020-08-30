#pragma once
#include "core/db_entity.h"
#include "core/identity.h"
#include "core/fav_list_details.h"

#include <QSharedPointer>
#include <QDateTime>
namespace core {
class Author;
typedef QSharedPointer<Author> AuthorPtr;
enum class AuthorIdStatus
{
    unassigned = -1,
    not_found = -2,
    valid = 0
};
class AuthorStats
{
public:
    QDate pageCreated;
    QDate bioLastUpdated;
    QDate favouritesLastUpdated;
    QDate favouritesLastChecked;
    int bioWordCount = -1;

    FavListDetails favouriteStats;
    FavListDetails ownFicStats;
    void Serialize(QDataStream &out);
    void Deserialize(QDataStream &in);
};

class Author : public DBEntity{
public:
    static AuthorPtr NewAuthor() { return AuthorPtr(new Author);}
    //FicSectionStats MergeStats(QList<AuthorPtr>);
    ~Author(){}
    void Log();
    void LogWebIds();
    void AssignId(int id){
        if(id == -1)
        {
            this->id = -1;
            this->idStatus = AuthorIdStatus::not_found;
        }
        if(id > -1)
        {
            this->id = id;
            this->idStatus = AuthorIdStatus::valid;
        }
    }
    AuthorIdStatus GetIdStatus() const {return idStatus;}
    void SetWebID(QString website, int id){webIds[website] = id;}
    int GetWebID(QString website) {
        if(webIds.contains(website))
            return webIds[website];
        return -1;
    }
    QString CreateAuthorUrl(QString urlType, int webId) const;
    QString url(QString type) const
    {
        if(webIds.contains(type))
            return CreateAuthorUrl(type, webIds[type]);
        return "";
    }
    QStringList GetWebsites() const;

    int id= -1;
    AuthorIdStatus idStatus = AuthorIdStatus::unassigned;
    QString name;
    QDateTime firstPublishedFic;
    QDateTime lastUpdated;
    int ficCount = -1;
    int recCount = -1;
    int favCount = -1;
    bool isValid = false;
    QHash<QString, int> webIds;
    UpdateMode updateMode = UpdateMode::none;

    AuthorStats stats;

    void Serialize(QDataStream &out);
    void Deserialize(QDataStream &in);

};

class AuthorRecommendationStats;
typedef QSharedPointer<AuthorRecommendationStats> AuhtorStatsPtr;

class AuthorRecommendationStats : public DBEntity
{
public:
    static AuhtorStatsPtr NewAuthorStats() { return QSharedPointer<AuthorRecommendationStats>(new AuthorRecommendationStats);}
    int authorId= -1;
    int totalRecommendations = -1;
    int matchesWithReference = -1;
    double matchRatio = -1;
    bool isValid = false;
    //QString listName;
    int listId = -1;
    QString usedTag;
    QString authorName;
};

struct AuthorFandomStatsForWeightCalc{
  int listId = -1;
  int fandomCount = -1;
  int ficCount = -1;

  double fandomDiversity = 0.0;

  QHash<int, double> fandomPresence;
  QHash<int, int> fandomCounts;

  void Serialize(QDataStream &out);
  void Deserialize(QDataStream &in);
};

typedef QSharedPointer<AuthorFandomStatsForWeightCalc> AuthorFavFandomStatsPtr;
}

Q_DECLARE_METATYPE(core::AuthorPtr);
