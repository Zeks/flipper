#pragma once
#include <functional>
#include <QString>
#include <memory>
#include <array>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSharedPointer>
#include "core/section.h"
#include "core/fic_genre_data.h"
#include "regex_utils.h"
#include "transaction.h"
#include "sqlcontext.h"

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

//bool ExecAndCheck(QSqlQuery& q);
bool CheckExecution(QSqlQuery& q);


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
DiagnosticSQLResult<bool> SetFandomTracked(int id, bool tracked, QSqlDatabase);
DiagnosticSQLResult<QStringList> GetTrackedFandomList(QSqlDatabase db);

DiagnosticSQLResult<bool> WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom, QSqlDatabase db);

DiagnosticSQLResult<QStringList> GetFandomListFromDB(QSqlDatabase db);
//void CalculateFandomsAverages(QSqlDatabase db);
DiagnosticSQLResult<bool> CalculateFandomsFicCounts(QSqlDatabase db);
DiagnosticSQLResult<bool> UpdateFandomStats(int fandomId, QSqlDatabase db);
DiagnosticSQLResult<bool> AssignTagToFandom(QString tag, int fandom_id, QSqlDatabase db, bool includeCrossovers = false);
DiagnosticSQLResult<bool> AssignTagToFanfic(QString tag, int fic_id, QSqlDatabase db);
DiagnosticSQLResult<bool> RemoveTagFromFanfic(QString tag, int fic_id, QSqlDatabase db);
DiagnosticSQLResult<bool> AssignChapterToFanfic(int chapter, int fic_id, QSqlDatabase db);
DiagnosticSQLResult<bool> AssignSlashToFanfic(int fic_id, int source, QSqlDatabase db);

DiagnosticSQLResult<bool> PerformGenreAssignment(QSqlDatabase db);

DiagnosticSQLResult<bool> ProcessSlashFicsBasedOnWords(std::function<SlashPresence(QString, QString, QString)>, QSqlDatabase db);
DiagnosticSQLResult<bool> WipeSlashMetainformation(QSqlDatabase db);

DiagnosticSQLResult<bool> CreateFandomInDatabase(QSharedPointer<core::Fandom> fandom,
                                                 QSqlDatabase db,
                                                 bool writeUrls = true,
                                                 bool useSuppliedIds = false);

DiagnosticSQLResult<QList<core::FandomPtr> > GetAllFandoms(QSqlDatabase db);
DiagnosticSQLResult<QList<core::FandomPtr> > GetAllFandomsAfter(int id, QSqlDatabase db);
QList<core::FandomPtr> GetAllFandomsFromSingleTable(QSqlDatabase db);
DiagnosticSQLResult<core::FandomPtr> GetFandom(QString name, QSqlDatabase db);

DiagnosticSQLResult<bool>  IgnoreFandom(int id, bool includeCrossovers, QSqlDatabase db);
DiagnosticSQLResult<bool>  RemoveFandomFromIgnoredList(int id, QSqlDatabase db);
DiagnosticSQLResult<QStringList>  GetIgnoredFandoms(QSqlDatabase db);
DiagnosticSQLResult<QHash<int, bool> > GetIgnoredFandomIDs(QSqlDatabase db);
DiagnosticSQLResult<QHash<int, QString> > GetFandomNamesForIDs(QList<int>, QSqlDatabase db);

DiagnosticSQLResult<bool>  IgnoreFandomSlashFilter(int id, QSqlDatabase db);
DiagnosticSQLResult<bool>  RemoveFandomFromIgnoredListSlashFilter(int id, QSqlDatabase db);
DiagnosticSQLResult<QStringList>  GetIgnoredFandomsSlashFilter(QSqlDatabase db);

DiagnosticSQLResult<bool> CleanupFandom(int fandom_id,  QSqlDatabase db);
DiagnosticSQLResult<bool> DeleteFandom(int fandom_id,  QSqlDatabase db);
DiagnosticSQLResult<int> GetFandomCountInDatabase(QSqlDatabase db);
DiagnosticSQLResult<bool> AddFandomForFic(int ficId, int fandomId, QSqlDatabase db);
DiagnosticSQLResult<bool> CreateFandomIndexRecord(int id, QString name, QSqlDatabase db);
//bool AddFandomLink(int oldId, int newId, QSqlDatabase db);
//bool RebindFicsToIndex(int oldId, int newId, QSqlDatabase db);
DiagnosticSQLResult<QHash<int, QList<int>>> GetWholeFicFandomsTable(QSqlDatabase db);
DiagnosticSQLResult<bool> EraseFicFandomsTable(QSqlDatabase db);
DiagnosticSQLResult<bool> SetLastUpdateDateForFandom(int id, QDate date, QSqlDatabase db);
DiagnosticSQLResult<bool> RemoveFandomFromRecentList(QString name, QSqlDatabase db);

DiagnosticSQLResult<QStringList> GetFandomNamesForFicId(int ficId, QSqlDatabase db);
DiagnosticSQLResult<bool> AddUrlToFandom(int fandomID, core::Url url, QSqlDatabase);

DiagnosticSQLResult<int>  GetFicIdByAuthorAndName(QString author, QString title, QSqlDatabase db);
DiagnosticSQLResult<int> GetFicIdByWebId(QString website, int webId, QSqlDatabase db);
DiagnosticSQLResult<core::FicPtr> GetFicByWebId(QString website, int webId, QSqlDatabase db);
DiagnosticSQLResult<core::FicPtr> GetFicById(int ficId, QSqlDatabase db);



DiagnosticSQLResult<bool> SetUpdateOrInsert(QSharedPointer<core::Fic> fic, QSqlDatabase db, bool alwaysUpdateIfNotInsert);
DiagnosticSQLResult<bool> InsertIntoDB(QSharedPointer<core::Fic> section, QSqlDatabase db);
DiagnosticSQLResult<bool>  UpdateInDB(QSharedPointer<core::Fic> section, QSqlDatabase db);
DiagnosticSQLResult<bool> WriteRecommendation(core::AuthorPtr author, int fic_id, QSqlDatabase db);
DiagnosticSQLResult<int> GetAuthorIdFromUrl(QString url, QSqlDatabase db);
DiagnosticSQLResult<int> GetAuthorIdFromWebID(int id, QString website, QSqlDatabase db);
DiagnosticSQLResult<QSet<int>> GetAuthorsForFics(QSet<int>, QSqlDatabase db);
DiagnosticSQLResult<bool>  AssignNewNameForAuthor(core::AuthorPtr author, QString name, QSqlDatabase db);

DiagnosticSQLResult<QList<int>> GetAllAuthorIds(QSqlDatabase db);
DiagnosticSQLResult<QSet<int> > GetAllMatchesWithRecsUID(QSharedPointer<core::RecommendationList> params, QString, QSqlDatabase db);
DiagnosticSQLResult<QSet<int> > ConvertFFNSourceFicsToDB(QString, QSqlDatabase db);
DiagnosticSQLResult<bool> ConvertFFNTaggedFicsToDB(QHash<int, int> &, QSqlDatabase db);
DiagnosticSQLResult<bool> ConvertDBFicsToFFN(QHash<int, int> &, QSqlDatabase db);

DiagnosticSQLResult<bool> ResetActionQueue(QSqlDatabase db);
DiagnosticSQLResult<bool> WriteDetectedGenres(QVector<genre_stats::FicGenreData>, QSqlDatabase db);



DiagnosticSQLResult<QHash<int, int> > GetMatchesForUID(QString uid, QSqlDatabase db);

DiagnosticSQLResult<QStringList> GetAllAuthorFavourites(int id, QSqlDatabase db);

DiagnosticSQLResult<QList<core::AuthorPtr> > GetAllAuthors(QString website, QSqlDatabase db,  int limit = 0);
DiagnosticSQLResult<QList<core::AuthorPtr>> GetAllAuthorsWithFavUpdateSince(QString website, QDateTime date, QSqlDatabase db,  int limit = 0);

DiagnosticSQLResult<QList<core::AuthorPtr>> GetAuthorsForRecommendationList(int listId,  QSqlDatabase db);


DiagnosticSQLResult<core::AuthorPtr> GetAuthorByNameAndWebsite(QString name, QString website,  QSqlDatabase db);
DiagnosticSQLResult<core::AuthorPtr> GetAuthorByIDAndWebsite(int id, QString website,  QSqlDatabase db);
DiagnosticSQLResult<bool> LoadAuthorStatistics(core::AuthorPtr, QSqlDatabase db);
DiagnosticSQLResult<QHash<int, QSet<int>>> LoadFullFavouritesHashset(QSqlDatabase db);

DiagnosticSQLResult<core::AuthorPtr> GetAuthorByUrl(QString url,  QSqlDatabase db);
DiagnosticSQLResult<core::AuthorPtr> GetAuthorById(int id,  QSqlDatabase db);


DiagnosticSQLResult<bool> WriteAuthorFavouriteStatistics(core::AuthorPtr author, QSqlDatabase db);
DiagnosticSQLResult<bool> WriteAuthorFavouriteGenreStatistics(core::AuthorPtr author, QSqlDatabase db);
DiagnosticSQLResult<bool> WriteAuthorFavouriteFandomStatistics(core::AuthorPtr author, QSqlDatabase db);
DiagnosticSQLResult<bool> WipeAuthorStatistics(core::AuthorPtr author, QSqlDatabase db);
DiagnosticSQLResult<QList<int>>  GetAllAuthorRecommendations(int id, QSqlDatabase db);
DiagnosticSQLResult<QSet<int>>  GetAllKnownSlashFics(QSqlDatabase db);
DiagnosticSQLResult<QSet<int>>  GetAllKnownNotSlashFics(QSqlDatabase db);
DiagnosticSQLResult<QSet<int>>  GetAllKnownFicIds(QString, QSqlDatabase db);
DiagnosticSQLResult<QSet<int>>  GetSingularFicsInLargeButSlashyLists(QSqlDatabase db);
DiagnosticSQLResult<QHash<int, std::array<double, 21>>> GetListGenreData(QSqlDatabase db);
DiagnosticSQLResult<QHash<int, double>>  GetFicGenreData(QString genre, QString cutoff, QSqlDatabase db);
DiagnosticSQLResult<QHash<int, std::array<double, 21>>> GetFullFicGenreData(QSqlDatabase db);
DiagnosticSQLResult<QHash<int, double> > GetDoubleValueHashForFics(QString fieldName, QSqlDatabase db);

DiagnosticSQLResult<genre_stats::FicGenreData> GetRealGenresForFic(int ficId, QSqlDatabase db);
DiagnosticSQLResult<QVector<genre_stats::FicGenreData>> GetGenreDataForQueuedFics(QSqlDatabase db);
DiagnosticSQLResult<bool> QueueFicsForGenreDetection(int minAuthorRecs, int minFoundLists, int minFaves, QSqlDatabase db);



DiagnosticSQLResult<bool> AssignIterationOfSlash(QString iteration, QSqlDatabase db);


DiagnosticSQLResult<bool> WipeAuthorStatisticsRecords(QSqlDatabase db);
DiagnosticSQLResult<bool> CreateStatisticsRecordsForAuthors(QSqlDatabase db);
DiagnosticSQLResult<bool> CalculateSlashStatisticsPercentages(QString usedField, QSqlDatabase db);



DiagnosticSQLResult<QList<core::AuhtorStatsPtr>> GetRecommenderStatsForList(int listId, QString sortOn, QString order, QSqlDatabase db);
DiagnosticSQLResult<QList<QSharedPointer<core::RecommendationList>>> GetAvailableRecommendationLists(QSqlDatabase db);
DiagnosticSQLResult<QSharedPointer<core::RecommendationList> > GetRecommendationList(int listid, QSqlDatabase db);
DiagnosticSQLResult<QSharedPointer<core::RecommendationList>> GetRecommendationList(QString name, QSqlDatabase db);

DiagnosticSQLResult<int> GetMatchCountForRecommenderOnList(int authorId, int list, QSqlDatabase db);

DiagnosticSQLResult<QVector<int>> GetAllFicIDsFromRecommendationList(int listId, QSqlDatabase db);
DiagnosticSQLResult<QVector<int>> GetAllSourceFicIDsFromRecommendationList(int listId, QSqlDatabase db);

DiagnosticSQLResult<QHash<int,int>> GetAllFicsHashFromRecommendationList(int listId, QSqlDatabase db, int minMatchCount = 0);
DiagnosticSQLResult<QStringList> GetAllAuthorNamesForRecommendationList(int listId, QSqlDatabase db);

DiagnosticSQLResult<int> GetCountOfTagInAuthorRecommendations(int authorId, QString tag, QSqlDatabase db);
DiagnosticSQLResult<int> GetMatchesWithListIdInAuthorRecommendations(int authorId, int listId, QSqlDatabase db);


DiagnosticSQLResult<bool> DeleteRecommendationList(int listId, QSqlDatabase db );
DiagnosticSQLResult<bool> DeleteRecommendationListData(int listId, QSqlDatabase db );

DiagnosticSQLResult<bool> CopyAllAuthorRecommendationsToList(int authorId, int listId, QSqlDatabase db);
DiagnosticSQLResult<bool> WriteAuthorRecommendationStatsForList(int listId, core::AuhtorStatsPtr stats, QSqlDatabase db);
DiagnosticSQLResult<bool> RemoveAuthorRecommendationStatsFromDatabase(int listId, int authorId, QSqlDatabase db);
//bool UploadLinkedAuthorsForAuthor(int authorId, QStringList list, QSqlDatabase db);
DiagnosticSQLResult<bool> UploadLinkedAuthorsForAuthor(int authorId, QString website, QList<int> ids, QSqlDatabase db);
DiagnosticSQLResult<bool> DeleteLinkedAuthorsForAuthor(int authorId,  QSqlDatabase db);
DiagnosticSQLResult<bool> CreateOrUpdateRecommendationList(QSharedPointer<core::RecommendationList> list, QDateTime creationTimestamp, QSqlDatabase db);
DiagnosticSQLResult<bool> UpdateFicCountForRecommendationList(int listId, QSqlDatabase db);
DiagnosticSQLResult<QList<int> > GetRecommendersForFicIdAndListId(int ficId, QSqlDatabase db);
DiagnosticSQLResult<QSet<int> > GetAllTaggedFics(QStringList tags, QSqlDatabase db);
DiagnosticSQLResult<QVector<int> > GetAllFicsThatDontHaveDBID( QSqlDatabase db);
DiagnosticSQLResult<bool> FillDBIDsForFics(QVector<core::IdPack>, QSqlDatabase db);
DiagnosticSQLResult<bool> FetchTagsForFics(QVector<core::Fic> * fics, QSqlDatabase db);



DiagnosticSQLResult<QStringList> GetLinkedPagesForList(int listId, QString website, QSqlDatabase db);
DiagnosticSQLResult<bool> SetFicsAsListOrigin(QVector<int> ficIds, int listId, QSqlDatabase db);
DiagnosticSQLResult<bool>  FillRecommendationListWithData(int listId, QHash<int, int>, QSqlDatabase db);




DiagnosticSQLResult<bool> DeleteTagFromDatabase(QString tag, QSqlDatabase db);
DiagnosticSQLResult<bool>  CreateTagInDatabase(QString tag, QSqlDatabase db);
DiagnosticSQLResult<bool>  ExportTagsToDatabase (QSqlDatabase originDB, QSqlDatabase targetDB);
DiagnosticSQLResult<bool>  ImportTagsFromDatabase(QSqlDatabase originDB, QSqlDatabase targetDB);


DiagnosticSQLResult<bool>  ExportSlashToDatabase (QSqlDatabase originDB, QSqlDatabase targetDB);
DiagnosticSQLResult<bool>  ImportSlashFromDatabase(QSqlDatabase originDB, QSqlDatabase targetDB);

DiagnosticSQLResult<bool>  AddAuthorFavouritesToList(int authorId, int listId, QSqlDatabase db);
DiagnosticSQLResult<bool>  ShortenFFNurlsForAllFics(QSqlDatabase db);

//! todo  currently unused
DiagnosticSQLResult<int> GetRecommendationListIdForName(QString name, QSqlDatabase db);
//will need to add genre tracker on ffn in case it'sever expanded
DiagnosticSQLResult<bool> IsGenreList(QStringList list, QString website, QSqlDatabase db);

DiagnosticSQLResult<QVector<int>> GetIdList(QString where, QSqlDatabase db);
DiagnosticSQLResult<QVector<int> > GetWebIdList(QString where, QString website, QSqlDatabase db);
DiagnosticSQLResult<bool> DeactivateStory(int id, QString website, QSqlDatabase db);

DiagnosticSQLResult<bool> UpdateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, QSqlDatabase db);
DiagnosticSQLResult<bool> UpdateAuthorFavouritesUpdateDate(int authorId, QDateTime date, QSqlDatabase db);

DiagnosticSQLResult<bool> CreateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, QSqlDatabase db);
DiagnosticSQLResult<QStringList> ReadUserTags(QSqlDatabase db);
DiagnosticSQLResult<bool> PushTaglistIntoDatabase(QStringList, QSqlDatabase);
DiagnosticSQLResult<bool> IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, QSqlDatabase db);
DiagnosticSQLResult<bool> DecrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, QSqlDatabase db);
DiagnosticSQLResult<QSet<QString> > GetAllGenres(QSqlDatabase db);
// those are required for managing recommendation lists and somewhat outdated
// moved them to dump temporarily
//void RemoveAuthor(const core::Author &recommender);
//void RemoveAuthor(int id);
DiagnosticSQLResult<bool> FillFicDataForList(int listId,
                                             const QVector<int>&,
                                             const QVector<int>&,
                                             const QSet<int> &origins,
                                             QSqlDatabase db);

// potentially ethically problematic. better not do this
//DiagnosticSQLResult<bool> FillAuthorDataForList(int listId, const QVector<int>&, QSqlDatabase db);

// page tasks
DiagnosticSQLResult<int> GetLastExecutedTaskID(QSqlDatabase db);
DiagnosticSQLResult<bool> GetTaskSuccessByID(int id, QSqlDatabase db);

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
DiagnosticSQLResult<bool>  EnsureUUIDForUserDatabase(QUuid id, QSqlDatabase db);
DiagnosticSQLResult<QString> GetUserToken(QSqlDatabase db);
DiagnosticSQLResult<int> GetLastFandomID(QSqlDatabase db);

namespace Internal{
DiagnosticSQLResult<bool> WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom,
                                 QString condition,
                                 QSqlDatabase db,
                                 std::function<void(QSharedPointer<core::Fandom>, QDateTime)> writer
                                 );
}

}
}
