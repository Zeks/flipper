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

void WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom, QSqlDatabase db);

QStringList GetFandomListFromDB(QString section);
void CalculateFandomAverages(QSqlDatabase db);
void CalculateFandomFicCounts(QSqlDatabase db);
void AssignTagToFandom(QString tag, int fandom_id, QSqlDatabase db);

bool CreateFandomInDatabase(QSharedPointer<core::Fandom> fandom, QSqlDatabase db);

int GetFicIdByAuthorAndName(QString author, QString title, QSqlDatabase db);

// needs not exist? better read from fics?
// probbaly needs to return whole fic
int GetFicIdByWebId(QString website, int webId, QSqlDatabase db);

void SetUpdateOrInsert(QSharedPointer<core::Fic> fic, QSqlDatabase db, bool alwaysUpdateIfNotInsert);
bool InsertIntoDB(QSharedPointer<core::Fic> section, QSqlDatabase db);
bool UpdateInDB(QSharedPointer<core::Fic> section, QSqlDatabase db);
bool WriteRecommendation(QSharedPointer<core::Author> author, int fic_id, QSqlDatabase db);
int GetAuthorIdFromUrl(QString url, QSqlDatabase db);
bool AssignNewNameForRecommenderId(core::Author recommender, QSqlDatabase db);



QList<QSharedPointer<core::Author> > GetAllAuthors(QString website,  QSqlDatabase db);
QList<core::AuthorRecommendationStats> GetRecommenderStatsForList(int listId, QString sortOn, QString order, QSqlDatabase db);
QList<QSharedPointer<core::RecommendationList>> GetAvailableRecommendationLists(QSqlDatabase db);

QList<core::AuthorRecommendationStats> GetRecommenderStatsForList(QString listName, QString sortOn, QString order, QSqlDatabase db);



// those are required for managing recommendation lists and somewhat outdated
// moved them to dump temporarily
//void RemoveAuthor(const core::Author &recommender);
//void RemoveAuthor(int id);

namespace Internal{
void WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom,
                              QString condition,
                              QSqlDatabase db,
                              std::function<void(QSharedPointer<core::Fandom>, QDateTime)> writer);
}

}
}
