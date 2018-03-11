#pragma once
#include <functional>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSharedPointer>
#include "core/section.h"

class BasePageTask;
class PageTask;
class PageSubTask;
class PageFailure;
class PageTaskAction;
typedef QSharedPointer<BasePageTask> BaseTaskPtr;
typedef QSharedPointer<PageTask> PageTaskPtr;
typedef QSharedPointer<PageSubTask> SubTaskPtr;
typedef QSharedPointer<PageFailure> PageFailurePtr;
typedef QSharedPointer<PageTaskAction> PageTaskActionPtr;
typedef QList<PageTaskPtr> TaskList;
typedef QList<SubTaskPtr> SubTaskList;
typedef QList<PageFailurePtr> SubTaskErrors;
typedef QList<PageTaskActionPtr> PageTaskActions;
namespace database {
namespace puresql{

bool ExecAndCheck(QSqlQuery& q);
bool CheckExecution(QSqlQuery& q);
bool ExecuteQuery(QSqlQuery& q, QString query);
bool ExecuteQueryChain(QSqlQuery& q, QStringList queries);

template <typename T>
struct DiagnosticSQLResult
{
    bool success = true;
    QString oracleError;
    T data;
    bool ExecAndCheck(QSqlQuery& q, bool ignoreUniqueness = false) {
        bool success = database::puresql::ExecAndCheck(q);
        bool uniqueTriggered = ignoreUniqueness && q.lastError().text().contains("UNIQUE constraint failed");
        if(uniqueTriggered)
            return true;
        if(!success && !uniqueTriggered)
        {
            success = false;
            oracleError = q.lastError().text();
        }
        return success;
    }
    bool CheckDataAvailability(QSqlQuery& q){
        if(!q.next())
        {
            success = false;
            oracleError = "no data to read";
            return false;
        }
        return true;
    }
};


struct FanficIdRecord
{
    FanficIdRecord();
    DiagnosticSQLResult<int> CreateRecord(QSqlDatabase db) const;
    int GetID(QString value) const {
        if(ids.contains(value))
            return ids[value];
        return -1;
    }
    QHash<QString, int> ids;
};
struct FicIdHash
{
    int GetID(QString website, int value) const
    {
        if(ids[website].contains(value))
            return ids[website][value];
        return -1;
    }
    FanficIdRecord GetRecord(int value) const {
        if(records.contains(value))
            return records[value];
        return emptyRecord;
    }
    QHash<QString, QHash<int, int>> ids;
    QHash<int, FanficIdRecord> records;
    FanficIdRecord emptyRecord;
};
bool SetFandomTracked(int id, bool tracked, QSqlDatabase);
QStringList GetTrackedFandomList(QSqlDatabase db);

bool WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom, QSqlDatabase db);

QStringList GetFandomListFromDB(QSqlDatabase db);
//void CalculateFandomsAverages(QSqlDatabase db);
void CalculateFandomsFicCounts(QSqlDatabase db);
bool UpdateFandomStats(int fandomId, QSqlDatabase db);
void AssignTagToFandom(QString tag, int fandom_id, QSqlDatabase db, bool includeCrossovers = false);
void AssignTagToFanfic(QString tag, int fic_id, QSqlDatabase db);
bool RemoveTagFromFanfic(QString tag, int fic_id, QSqlDatabase db);
bool AssignChapterToFanfic(int chapter, int fic_id, QSqlDatabase db);

bool CreateFandomInDatabase(QSharedPointer<core::Fandom> fandom, QSqlDatabase db);

QList<core::FandomPtr> GetAllFandoms(QSqlDatabase db);
QList<core::FandomPtr> GetAllFandomsFromSingleTable(QSqlDatabase db);
core::FandomPtr GetFandom(QString name, QSqlDatabase db);

DiagnosticSQLResult<bool>  IgnoreFandom(int id, bool includeCrossovers, QSqlDatabase db);
DiagnosticSQLResult<bool>  RemoveFandomFromIgnoredList(int id, QSqlDatabase db);
DiagnosticSQLResult<QStringList>  GetIgnoredFandoms(QSqlDatabase db);

bool CleanupFandom(int fandom_id,  QSqlDatabase db);
DiagnosticSQLResult<bool> DeleteFandom(int fandom_id,  QSqlDatabase db);
int GetFandomCountInDatabase(QSqlDatabase db);
bool AddFandomForFic(int ficId, int fandomId, QSqlDatabase db);
bool CreateFandomIndexRecord(int id, QString name, QSqlDatabase db);
//bool AddFandomLink(int oldId, int newId, QSqlDatabase db);
//bool RebindFicsToIndex(int oldId, int newId, QSqlDatabase db);
QHash<int, QList<int> > GetWholeFicFandomsTable(QSqlDatabase db);
bool EraseFicFandomsTable(QSqlDatabase db);
bool SetLastUpdateDateForFandom(int id, QDate date, QSqlDatabase db);
DiagnosticSQLResult<bool> RemoveFandomFromRecentList(QString name, QSqlDatabase db);

QStringList GetFandomNamesForFicId(int ficId, QSqlDatabase db);
DiagnosticSQLResult<bool> AddUrlToFandom(int fandomID, core::Url url, QSqlDatabase);

int GetFicIdByAuthorAndName(QString author, QString title, QSqlDatabase db);
int GetFicIdByWebId(QString website, int webId, QSqlDatabase db);
core::FicPtr GetFicByWebId(QString website, int webId, QSqlDatabase db);
core::FicPtr GetFicById(int ficId, QSqlDatabase db);

bool SetUpdateOrInsert(QSharedPointer<core::Fic> fic, QSqlDatabase db, bool alwaysUpdateIfNotInsert);
bool InsertIntoDB(QSharedPointer<core::Fic> section, QSqlDatabase db);
bool UpdateInDB(QSharedPointer<core::Fic> section, QSqlDatabase db);
bool WriteRecommendation(core::AuthorPtr author, int fic_id, QSqlDatabase db);
int GetAuthorIdFromUrl(QString url, QSqlDatabase db);
int GetAuthorIdFromWebID(int id, QString website, QSqlDatabase db);
bool AssignNewNameForAuthor(core::AuthorPtr author, QString name, QSqlDatabase db);

QList<int> GetAllAuthorIds(QSqlDatabase db);
DiagnosticSQLResult<QStringList> GetAllAuthorFavourites(int id, QSqlDatabase db);

QList<core::AuthorPtr > GetAllAuthors(QString website,  QSqlDatabase db);
QList<core::AuthorPtr> GetAuthorsForRecommendationList(int listId,  QSqlDatabase db);

core::AuthorPtr  GetAuthorByNameAndWebsite(QString name, QString website,  QSqlDatabase db);
core::AuthorPtr  GetAuthorByIDAndWebsite(int id, QString website,  QSqlDatabase db);
DiagnosticSQLResult<bool> LoadAuthorStatistics(core::AuthorPtr, QSqlDatabase db);

core::AuthorPtr  GetAuthorByUrl(QString url,  QSqlDatabase db);
core::AuthorPtr  GetAuthorById(int id,  QSqlDatabase db);


DiagnosticSQLResult<bool> WriteAuthorFavouriteStatistics(core::AuthorPtr author, QSqlDatabase db);
DiagnosticSQLResult<bool> WriteAuthorFavouriteGenreStatistics(core::AuthorPtr author, QSqlDatabase db);
DiagnosticSQLResult<bool> WriteAuthorFavouriteFandomStatistics(core::AuthorPtr author, QSqlDatabase db);
DiagnosticSQLResult<bool> WipeAuthorStatistics(core::AuthorPtr author, QSqlDatabase db);
DiagnosticSQLResult<QList<int>>  GetAllAuthorRecommendations(int id, QSqlDatabase db);


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
bool RemoveAuthorRecommendationStatsFromDatabase(int listId, int authorId, QSqlDatabase db);
//bool UploadLinkedAuthorsForAuthor(int authorId, QStringList list, QSqlDatabase db);
bool UploadLinkedAuthorsForAuthor(int authorId, QString website, QList<int> ids, QSqlDatabase db);
bool DeleteLinkedAuthorsForAuthor(int authorId,  QSqlDatabase db);
bool CreateOrUpdateRecommendationList(QSharedPointer<core::RecommendationList> list, QDateTime creationTimestamp, QSqlDatabase db);
bool UpdateFicCountForRecommendationList(int listId, QSqlDatabase db);
QList<int> GetRecommendersForFicIdAndListId(int ficId, QSqlDatabase db);
QStringList GetLinkedPagesForList(int listId, QString website, QSqlDatabase db);
bool SetFicsAsListOrigin(QList<int> ficIds, int listId,QSqlDatabase db);
DiagnosticSQLResult<bool>  FillRecommendationListWithData(int listId, QHash<int, int>, QSqlDatabase db);



bool DeleteTagFromDatabase(QString tag, QSqlDatabase db);
bool CreateTagInDatabase(QString tag, QSqlDatabase db);
DiagnosticSQLResult<bool>  ExportTagsToDatabase (QSqlDatabase originDB, QSqlDatabase targetDB);
DiagnosticSQLResult<bool>  ImportTagsFromDatabase(QSqlDatabase originDB, QSqlDatabase targetDB);

bool AddAuthorFavouritesToList(int authorId, int listId, QSqlDatabase db);
void ShortenFFNurlsForAllFics(QSqlDatabase db);

//! todo  currently unused
int GetRecommendationListIdForName(QString name, QSqlDatabase db);
//will need to add genre tracker on ffn in case it'sever expanded
bool IsGenreList(QStringList list, QString website, QSqlDatabase db);

QVector<int> GetIdList(QString where, QSqlDatabase db);
QVector<int> GetWebIdList(QString where, QString website, QSqlDatabase db);
bool DeactivateStory(int id, QString website, QSqlDatabase db);

bool UpdateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, QSqlDatabase db);
bool CreateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, QSqlDatabase db);
QStringList ReadUserTags(QSqlDatabase db);
bool PushTaglistIntoDatabase(QStringList, QSqlDatabase);
bool IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, QSqlDatabase db);
bool DecrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, QSqlDatabase db);
QSet<QString> GetAllGenres(QSqlDatabase db);
// those are required for managing recommendation lists and somewhat outdated
// moved them to dump temporarily
//void RemoveAuthor(const core::Author &recommender);
//void RemoveAuthor(int id);

// page tasks
DiagnosticSQLResult<int> GetLastExecutedTaskID(QSqlDatabase db);
bool GetTaskSuccessByID(int id, QSqlDatabase db);

DiagnosticSQLResult<PageTaskPtr> GetTaskData(int id, QSqlDatabase db);
DiagnosticSQLResult<SubTaskList> GetSubTaskData(int id, QSqlDatabase db);
DiagnosticSQLResult<SubTaskErrors> GetErrorsForSubTask(int id, QSqlDatabase db, int subId = -1);
DiagnosticSQLResult<PageTaskActions> GetActionsForSubTask(int id, QSqlDatabase db, int subId = -1);

DiagnosticSQLResult<int> CreateTaskInDB(PageTaskPtr, QSqlDatabase);
DiagnosticSQLResult<bool> CreateSubTaskInDB(SubTaskPtr, QSqlDatabase);
DiagnosticSQLResult<bool> CreateActionInDB(PageTaskActionPtr, QSqlDatabase);
DiagnosticSQLResult<bool> CreateErrorsInDB(SubTaskErrors, QSqlDatabase);

DiagnosticSQLResult<bool> UpdateTaskInDB(PageTaskPtr, QSqlDatabase);
DiagnosticSQLResult<bool> UpdateSubTaskInDB(SubTaskPtr, QSqlDatabase);
DiagnosticSQLResult<bool> SetTaskFinished(int, QSqlDatabase);
DiagnosticSQLResult<TaskList> GetUnfinishedTasks(QSqlDatabase);

namespace Internal{
bool WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom,
                              QString condition,
                              QSqlDatabase db,
                              std::function<void(QSharedPointer<core::Fandom>, QDateTime)> writer
                              );
}

}
}
