#pragma once
#include <functional>
#include <QString>
#include <QSqlDatabase>
#include <QSharedPointer>
#include "section.h"
namespace database {
namespace puresql{
bool ExecAndCheck(QSqlQuery& q);
bool CheckExecution(QSqlQuery& q);
bool ExecuteQuery(QSqlQuery& q, QString query);
bool ExecuteQueryChain(QSqlQuery& q, QStringList queries);

bool SetFandomTracked(int id, bool tracked, QSqlDatabase);
QStringList GetTrackedFandomList(QSqlDatabase db);

bool WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom, QSqlDatabase db);

QStringList GetFandomListFromDB(QSqlDatabase db);
void CalculateFandomsAverages(QSqlDatabase db);
void CalculateFandomsFicCounts(QSqlDatabase db);
bool UpdateFandomStats(int fandomId, QSqlDatabase db);
void AssignTagToFandom(QString tag, int fandom_id, QSqlDatabase db);
void AssignTagToFanfic(QString tag, int fic_id, QSqlDatabase db);
bool RemoveTagFromFanfic(QString tag, int fic_id, QSqlDatabase db);
bool AssignChapterToFanfic(int chapter, int fic_id, QSqlDatabase db);

bool CreateFandomInDatabase(QSharedPointer<core::Fandom> fandom, QSqlDatabase db);

QList<core::FandomPtr> GetAllFandoms(QSqlDatabase db);
core::FandomPtr GetFandom(QString name, QSqlDatabase db);
bool CleanuFandom(int fandom_id,  QSqlDatabase db);
int GetFandomCountInDatabase(QSqlDatabase db);
bool AddFandomForFic(int ficId, int fandomId, QSqlDatabase db);

int GetFicIdByAuthorAndName(QString author, QString title, QSqlDatabase db);
int GetFicIdByWebId(QString website, int webId, QSqlDatabase db);
core::FicPtr GetFicByWebId(QString website, int webId, QSqlDatabase db);

bool SetUpdateOrInsert(QSharedPointer<core::Fic> fic, QSqlDatabase db, bool alwaysUpdateIfNotInsert);
bool InsertIntoDB(QSharedPointer<core::Fic> section, QSqlDatabase db);
bool UpdateInDB(QSharedPointer<core::Fic> section, QSqlDatabase db);
bool WriteRecommendation(core::AuthorPtr author, int fic_id, QSqlDatabase db);
int GetAuthorIdFromUrl(QString url, QSqlDatabase db);
bool AssignNewNameForAuthor(core::AuthorPtr author, QString name, QSqlDatabase db);

QList<int> GetAllAuthorIds(QSqlDatabase db);


QList<core::AuthorPtr > GetAllAuthors(QString website,  QSqlDatabase db);
QList<core::AuthorPtr> GetAuthorsForRecommendationList(int listId,  QSqlDatabase db);

core::AuthorPtr  GetAuthorByNameAndWebsite(QString name, QString website,  QSqlDatabase db);
core::AuthorPtr  GetAuthorByUrl(QString url,  QSqlDatabase db);
core::AuthorPtr  GetAuthorById(int id,  QSqlDatabase db);
QList<core::AuhtorStatsPtr> GetRecommenderStatsForList(int listId, QString sortOn, QString order, QSqlDatabase db);
QList<QSharedPointer<core::RecommendationList>> GetAvailableRecommendationLists(QSqlDatabase db);
QSharedPointer<core::RecommendationList> GetRecommendationList(int listid, QSqlDatabase db);
QSharedPointer<core::RecommendationList> GetRecommendationList(QString name, QSqlDatabase db);

int GetMatchCountForRecommenderOnList(int authorId, int list, QSqlDatabase db);

QVector<int> GetAllFicIDsFromRecommendationList(int listId, QSqlDatabase db);
QStringList GetAllAuthorNamesForRecommendationList(int listId, QSqlDatabase db);

int GetCountOfTagInAuthorRecommendations(int authorId, QString tag, QSqlDatabase db);
int GetMatchesWithListIdInAuthorRecommendations(int authorId, int listId, QSqlDatabase db);


bool DeleteRecommendationList(int listId, QSqlDatabase db );
bool CopyAllAuthorRecommendationsToList(int authorId, int listId, QSqlDatabase db);
bool WriteAuthorRecommendationStatsForList(int listId, core::AuhtorStatsPtr stats, QSqlDatabase db);
bool CreateOrUpdateRecommendationList(QSharedPointer<core::RecommendationList> list, QDateTime creationTimestamp, QSqlDatabase db);
bool UpdateFicCountForRecommendationList(int listId, QSqlDatabase db);
QList<int> GetRecommendersForFicIdAndListId(int ficId, QSqlDatabase db);

bool DeleteTagFromDatabase(QString tag, QSqlDatabase db);
bool CreateTagInDatabase(QString tag, QSqlDatabase db);

bool AddAuthorFavouritesToList(int authorId, int listId, QSqlDatabase db);
void ShortenFFNurlsForAllFics(QSqlDatabase db);

//! todo  currently unused
int GetRecommendationListIdForName(QString name, QSqlDatabase db);
//will need to add genre tracker on ffn in case it'sever expanded
bool IsGenreList(QStringList list, QString website, QSqlDatabase db);

QVector<int> GetIdList(QString where, QSqlDatabase db);
QVector<int> GetWebIdList(QString where, QString website, QSqlDatabase db);
bool DeactivateStory(int id, QString website, QSqlDatabase db);

bool WriteAuthor(core::AuthorPtr author, QDateTime timestamp, QSqlDatabase db);
QStringList ReadUserTags(QSqlDatabase db);
bool PushTaglistIntoDatabase(QStringList, QSqlDatabase);
bool IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, QSqlDatabase db);
QSet<QString> GetAllGenres(QSqlDatabase db);
// those are required for managing recommendation lists and somewhat outdated
// moved them to dump temporarily
//void RemoveAuthor(const core::Author &recommender);
//void RemoveAuthor(int id);

namespace Internal{
bool WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom,
                              QString condition,
                              QSqlDatabase db,
                              std::function<void(QSharedPointer<core::Fandom>, QDateTime)> writer
                              );
}

}
}
