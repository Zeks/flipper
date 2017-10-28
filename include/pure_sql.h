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

bool WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom, QSqlDatabase db);

QStringList GetFandomListFromDB(QSqlDatabase db);
void CalculateFandomAverages(QSqlDatabase db);
void CalculateFandomFicCounts(QSqlDatabase db);
void AssignTagToFandom(QString tag, int fandom_id, QSqlDatabase db);

bool CreateFandomInDatabase(QSharedPointer<core::Fandom> fandom, QSqlDatabase db);

int GetFicIdByAuthorAndName(QString author, QString title, QSqlDatabase db);

// needs not exist? better read from fics?
// probbaly needs to return whole fic
int GetFicIdByWebId(QString website, int webId, QSqlDatabase db);

bool SetUpdateOrInsert(QSharedPointer<core::Fic> fic, QSqlDatabase db, bool alwaysUpdateIfNotInsert);
bool InsertIntoDB(QSharedPointer<core::Fic> section, QSqlDatabase db);
bool UpdateInDB(QSharedPointer<core::Fic> section, QSqlDatabase db);
bool WriteRecommendation(QSharedPointer<core::Author> author, int fic_id, QSqlDatabase db);
int GetAuthorIdFromUrl(QString url, QSqlDatabase db);
bool AssignNewNameForAuthor(QSharedPointer<core::Author> author, QString name, QSqlDatabase db);

QList<int> GetAllAuthorIds(QSqlDatabase db);


QList<QSharedPointer<core::Author> > GetAllAuthors(QString website,  QSqlDatabase db);
QList<QSharedPointer<core::AuthorRecommendationStats>> GetRecommenderStatsForList(int listId, QString sortOn, QString order, QSqlDatabase db);
QList<QSharedPointer<core::RecommendationList>> GetAvailableRecommendationLists(QSqlDatabase db);
QSharedPointer<core::RecommendationList> GetRecommendationList(int listid, QSqlDatabase db);
QSharedPointer<core::RecommendationList> GetRecommendationList(QString name, QSqlDatabase db);

int GetMatchCountForRecommenderOnList(int authorId, int list, QSqlDatabase db);

QVector<int> GetAllFicIDsFromRecommendationList(int listId, QSqlDatabase db);


int GetCountOfTagInAuthorRecommendations(int authorId, QString tag, QSqlDatabase db);
int GetMatchesWithListIdInAuthorRecommendations(int authorId, int listId, QSqlDatabase db);


bool DeleteRecommendationList(int listId, QSqlDatabase db );
bool CopyAllAuthorRecommendationsToList(int authorId, int listId, QSqlDatabase db);
bool WriteAuthorRecommendationStatsForList(int listId, QSharedPointer<core::AuthorRecommendationStats> stats, QSqlDatabase db);
bool CreateOrUpdateRecommendationList(QSharedPointer<core::RecommendationList> list, QDateTime creationTimestamp, QSqlDatabase db);
bool UpdateFicCountForRecommendationList(int listId, QSqlDatabase db);
bool DeleteTagfromDatabase(QString tag, QSqlDatabase db);
bool AddAuthorFavouritesToList(int authorId, int listId, QSqlDatabase db);
void ShortenFFNurlsForAllFics(QSqlDatabase db);

//! todo  currently unused
int GetRecommendationListIdForName(QString name, QSqlDatabase db);
//will need to add genre tracker on ffn in case it'sever expanded
bool IsGenreList(QStringList list, QString website, QSqlDatabase db);

QVector<int> GetIdList(QString where, QSqlDatabase db);
QVector<int> GetWebIdList(QString where, QString website, QSqlDatabase db);
bool DeactivateStory(int id, QString website, QSqlDatabase db);

bool WriteAuthor(QSharedPointer<core::Author> author, QDateTime timestamp, QSqlDatabase db);
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
