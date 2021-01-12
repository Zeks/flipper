#pragma once
#include <functional>
#include <QString>
#include <memory>
#include <array>
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_context.h"
#include "sql_abstractions/sql_query.h"
#include <QSqlError>
#include <QSharedPointer>
#include "core/section.h"
#include "core/fandom.h"
#include "core/fandom_list.h"
#include "core/fic_genre_data.h"
#include "core/recommendation_list.h"
#include "core/experimental/fic_relations.h"
#include "include/storyfilter.h"
#include "include/reclist_author_result.h"
#include "filters/date_filter.h"
#include "regex_utils.h"
#include "transaction.h"


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
typedef QVector<PageTaskPtr> TaskList;
typedef QList<SubTaskPtr> SubTaskList;
typedef QList<PageFailurePtr> SubTaskErrors;
typedef QList<PageTaskActionPtr> PageTaskActions;

namespace sql{
static constexpr int genreArraySize = 22;
//bool ExecAndCheck(sql::Query& q);
bool CheckExecution(sql::Query& q);

struct DBVerificationResult{
  bool success = true;
  QStringList data;
};

enum IDType{
    idt_db = 0,
    idt_ffn = 1
};


struct FanficIdRecord
{
    FanficIdRecord();
    DiagnosticSQLResult<int> CreateRecord(sql::Database db) const;
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
DiagnosticSQLResult<bool> SetFandomTracked(int id, bool tracked, sql::Database);
DiagnosticSQLResult<QStringList> GetTrackedFandomList(sql::Database db);

DiagnosticSQLResult<bool> WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom, sql::Database db);

DiagnosticSQLResult<QStringList> GetFandomListFromDB(sql::Database db);
//void CalculateFandomsAverages(sql::Database db);
DiagnosticSQLResult<bool> CalculateFandomsFicCounts(sql::Database db);
DiagnosticSQLResult<bool> UpdateFandomStats(int fandomId, sql::Database db);
DiagnosticSQLResult<bool> AssignTagToFandom(QString tag, int fandom_id, sql::Database db, bool includeCrossovers = false);
DiagnosticSQLResult<bool> AssignTagToFanfic(QString tag, int fic_id, sql::Database db);
DiagnosticSQLResult<bool> RemoveTagFromFanfic(QString tag, int fic_id, sql::Database db);
DiagnosticSQLResult<bool> AssignChapterToFanfic(int fic_id, int chapter, sql::Database db);
DiagnosticSQLResult<bool> AssignScoreToFanfic(int chapter, int fic_id, sql::Database db);

DiagnosticSQLResult<bool> AssignSlashToFanfic(int fic_id, int source, sql::Database db);
DiagnosticSQLResult<bool> AssignQueuedToFanfic(int fic_id, sql::Database db);

DiagnosticSQLResult<bool> PerformGenreAssignment(sql::Database db);

DiagnosticSQLResult<bool> ProcessSlashFicsBasedOnWords(std::function<SlashPresence(QString, QString, QString)>, sql::Database db);
DiagnosticSQLResult<bool> WipeSlashMetainformation(sql::Database db);

DiagnosticSQLResult<bool> CreateFandomInDatabase(QSharedPointer<core::Fandom> fandom,
                                                 sql::Database db,
                                                 bool writeUrls = true,
                                                 bool useSuppliedIds = false);

DiagnosticSQLResult<QList<core::FandomPtr>> GetAllFandoms(sql::Database db);
DiagnosticSQLResult<QList<core::FandomPtr>> GetAllFandomsAfter(int id, sql::Database db);
QList<core::FandomPtr> GetAllFandomsFromSingleTable(sql::Database db);
DiagnosticSQLResult<core::FandomPtr> GetFandom(QString name, bool loadFandomStats, sql::Database db);
DiagnosticSQLResult<core::FandomPtr> GetFandom(int id, bool loadFandomStats,  sql::Database db);

DiagnosticSQLResult<bool>  IgnoreFandom(int id, bool includeCrossovers, sql::Database db);
DiagnosticSQLResult<bool>  RemoveFandomFromIgnoredList(int id, sql::Database db);
DiagnosticSQLResult<bool>  ProcessIgnoresIntoFandomLists(sql::Database db);

DiagnosticSQLResult<QStringList>  GetIgnoredFandoms(sql::Database db);
DiagnosticSQLResult<QHash<int, bool>> GetIgnoredFandomIDs(sql::Database db);
DiagnosticSQLResult<QHash<int, QString>> GetFandomNamesForIDs(QList<int>, sql::Database db);

DiagnosticSQLResult<bool>  IgnoreFandomSlashFilter(int id, sql::Database db);
DiagnosticSQLResult<bool>  RemoveFandomFromIgnoredListSlashFilter(int id, sql::Database db);
DiagnosticSQLResult<QStringList>  GetIgnoredFandomsSlashFilter(sql::Database db);

DiagnosticSQLResult<bool> CleanupFandom(int fandom_id,  sql::Database db);
DiagnosticSQLResult<bool> DeleteFandom(int fandom_id,  sql::Database db);
DiagnosticSQLResult<int> GetFandomCountInDatabase(sql::Database db);
DiagnosticSQLResult<bool> AddFandomForFic(int ficId, int fandomId, sql::Database db);
DiagnosticSQLResult<bool> CreateFandomIndexRecord(int id, QString name, sql::Database db);
//bool AddFandomLink(int oldId, int newId, sql::Database db);
//bool RebindFicsToIndex(int oldId, int newId, sql::Database db);
DiagnosticSQLResult<QHash<int, QList<int>>> GetWholeFicFandomsTable(sql::Database db);
DiagnosticSQLResult<bool> EraseFicFandomsTable(sql::Database db);
DiagnosticSQLResult<bool> SetLastUpdateDateForFandom(int id, QDate date, sql::Database db);
DiagnosticSQLResult<bool> RemoveFandomFromRecentList(QString name, sql::Database db);

DiagnosticSQLResult<QStringList> GetFandomNamesForFicId(int ficId, sql::Database db);
DiagnosticSQLResult<bool> AddUrlToFandom(int fandomID, core::Url url, sql::Database);


DiagnosticSQLResult<std::vector<core::fandom_lists::List::ListPtr>> FetchFandomLists(sql::Database);
DiagnosticSQLResult<std::vector<core::fandom_lists::FandomStateInList>> FetchFandomStatesInUserList(int list_id, sql::Database);
DiagnosticSQLResult<bool> AddFandomToUserList(uint32_t list_id, uint32_t fandom_id,  QString fandom_name, sql::Database);
DiagnosticSQLResult<bool> RemoveFandomFromUserList(uint32_t list_id, uint32_t fandom_id, sql::Database);
DiagnosticSQLResult<int> AddNewFandomList(QString name, sql::Database);
DiagnosticSQLResult<bool> RemoveFandomList(uint32_t list_id, sql::Database);

DiagnosticSQLResult<bool> EditFandomStateForList(const core::fandom_lists::FandomStateInList&, sql::Database);
DiagnosticSQLResult<bool> EditListState(const core::fandom_lists::List &, sql::Database);
DiagnosticSQLResult<bool> FlipListValues(uint32_t list_id, sql::Database);






DiagnosticSQLResult<int>  GetFicIdByAuthorAndName(QString author, QString title, sql::Database db);
DiagnosticSQLResult<int> GetFicIdByWebId(QString website, int webId, sql::Database db);
DiagnosticSQLResult<core::FicPtr> GetFicByWebId(QString website, int webId, sql::Database db);
DiagnosticSQLResult<core::FicPtr> GetFicById(int ficId, sql::Database db);



DiagnosticSQLResult<bool> SetUpdateOrInsert(QSharedPointer<core::Fanfic> fic, sql::Database db, bool alwaysUpdateIfNotInsert);
DiagnosticSQLResult<bool> InsertIntoDB(QSharedPointer<core::Fanfic> section, sql::Database db);
DiagnosticSQLResult<bool>  UpdateInDB(QSharedPointer<core::Fanfic> section, sql::Database db);
DiagnosticSQLResult<bool> WriteRecommendation(core::AuthorPtr author, int fic_id, sql::Database db);
DiagnosticSQLResult<bool> WriteFicRelations(QList<core::FicWeightResult> result,  sql::Database db);
DiagnosticSQLResult<bool> WriteAuthorsForFics(QHash<uint32_t, uint32_t> data,  sql::Database db);


DiagnosticSQLResult<int> GetAuthorIdFromUrl(QString url, sql::Database db);
DiagnosticSQLResult<int> GetAuthorIdFromWebID(int id, QString website, sql::Database db);
DiagnosticSQLResult<QSet<int>> GetAuthorsForFics(QSet<int>, sql::Database db);
DiagnosticSQLResult<QSet<int>> GetRecommendersForFics(QSet<int>, sql::Database db);

DiagnosticSQLResult<QHash<uint32_t, int>> GetHashAuthorsForFics(QSet<int>, sql::Database db);

DiagnosticSQLResult<bool>  AssignNewNameForAuthor(core::AuthorPtr author, QString name, sql::Database db);

DiagnosticSQLResult<QList<int>> GetAllAuthorIds(sql::Database db);
DiagnosticSQLResult<QSet<int>> GetAllMatchesWithRecsUID(QSharedPointer<core::RecommendationList> params, QString, sql::Database db);
DiagnosticSQLResult<QSet<int>> ConvertFFNSourceFicsToDB(QString, sql::Database db);
DiagnosticSQLResult<QHash<uint32_t, core::FicWeightPtr>> GetFicsForRecCreation(sql::Database db);
DiagnosticSQLResult<bool> ConvertFFNTaggedFicsToDB(QHash<int, int> &, sql::Database db);
DiagnosticSQLResult<bool> ConvertDBFicsToFFN(QHash<int, int> &, sql::Database db);

DiagnosticSQLResult<bool> ResetActionQueue(sql::Database db);
DiagnosticSQLResult<bool> WriteDetectedGenres(QVector<genre_stats::FicGenreData>, sql::Database db);

DiagnosticSQLResult<bool> WriteDetectedGenresIteration2(QVector<genre_stats::FicGenreData>, sql::Database db);

DiagnosticSQLResult<QHash<int, QList<genre_stats::GenreBit>>> GetFullGenreList(sql::Database db, bool useOriginalOnly = false);
DiagnosticSQLResult<bool> SetUserProfile(int id,  sql::Database db);
DiagnosticSQLResult<int> GetUserProfile(sql::Database db);
DiagnosticSQLResult<int> GetRecommenderIDByFFNId(int id, sql::Database db);



DiagnosticSQLResult<QHash<int, int>> GetMatchesForUID(QString uid, sql::Database db);

DiagnosticSQLResult<QStringList> GetAllAuthorFavourites(int id, sql::Database db);
DiagnosticSQLResult<QList<int>> GetAllAuthorRecommendationIDs(int id, sql::Database db);


DiagnosticSQLResult<QList<core::AuthorPtr>> GetAllAuthors(QString website, sql::Database db,  int limit = 0);
DiagnosticSQLResult<QList<core::AuthorPtr>> GetAllAuthorsWithFavUpdateSince(QString website, QDateTime date, sql::Database db,  int limit = 0);
DiagnosticSQLResult<QList<core::AuthorPtr>> GetAllAuthorsWithFavUpdateBetween(QString website,
                                                                 QDateTime dateStart,
                                                                 QDateTime dateEnd, sql::Database db, int limit = 0);

DiagnosticSQLResult<QList<core::AuthorPtr>> GetAuthorsForRecommendationList(int listId,  sql::Database db);
DiagnosticSQLResult<QString> GetAuthorsForRecommendationListClient(int list_id,  sql::Database db);



DiagnosticSQLResult<core::AuthorPtr> GetAuthorByNameAndWebsite(QString name, QString website,  sql::Database db);
DiagnosticSQLResult<core::AuthorPtr> GetAuthorByIDAndWebsite(int id, QString website,  sql::Database db);
DiagnosticSQLResult<bool> LoadAuthorStatistics(core::AuthorPtr, sql::Database db);
DiagnosticSQLResult<QHash<int, QSet<int>>> LoadFullFavouritesHashset(sql::Database db);

DiagnosticSQLResult<core::AuthorPtr> GetAuthorByUrl(QString url,  sql::Database db);
DiagnosticSQLResult<core::AuthorPtr> GetAuthorById(int id,  sql::Database db);
DiagnosticSQLResult<bool> AssignAuthorNamesForWebIDsInFanficTable(sql::Database db);


DiagnosticSQLResult<bool> WriteAuthorFavouriteStatistics(core::AuthorPtr author, sql::Database db);
DiagnosticSQLResult<bool> WriteAuthorFavouriteGenreStatistics(core::AuthorPtr author, sql::Database db);
DiagnosticSQLResult<bool> WriteAuthorFavouriteFandomStatistics(core::AuthorPtr author, sql::Database db);
DiagnosticSQLResult<bool> WipeAuthorStatistics(core::AuthorPtr author, sql::Database db);
DiagnosticSQLResult<QList<int>>  GetAllAuthorRecommendations(int id, sql::Database db);
DiagnosticSQLResult<QSet<int>>  GetAllKnownSlashFics(sql::Database db);
DiagnosticSQLResult<QSet<int>>  GetAllKnownNotSlashFics(sql::Database db);
DiagnosticSQLResult<QSet<int>>  GetAllKnownFicIds(QString, sql::Database db);
DiagnosticSQLResult<QSet<int>>  GetFicIDsWithUnsetAuthors(sql::Database db);

DiagnosticSQLResult<QVector<core::FicWeightPtr>>  GetAllFicsWithEnoughFavesForWeights(int faves, sql::Database db);
DiagnosticSQLResult<QHash<int, core::AuthorFavFandomStatsPtr>> GetAuthorListFandomStatistics(QList<int> authors, sql::Database db);

DiagnosticSQLResult<QSet<int>>  GetSingularFicsInLargeButSlashyLists(sql::Database db);
DiagnosticSQLResult<QHash<int, std::array<double, genreArraySize>>> GetListGenreData(sql::Database db);
DiagnosticSQLResult<QHash<uint32_t, genre_stats::ListMoodData>> GetMoodDataForLists(sql::Database db);
DiagnosticSQLResult<QHash<int, double>>  GetFicGenreData(QString genre, QString cutoff, sql::Database db);
DiagnosticSQLResult<QHash<int, std::array<double, genreArraySize>>> GetFullFicGenreData(sql::Database db);
DiagnosticSQLResult<QHash<int, double>> GetDoubleValueHashForFics(QString fieldName, sql::Database db);
DiagnosticSQLResult<QHash<int, QString>>GetGenreForFics(sql::Database db);
DiagnosticSQLResult<QHash<int, int>> GetScoresForFics(sql::Database db);


DiagnosticSQLResult<genre_stats::FicGenreData> GetRealGenresForFic(int ficId, sql::Database db);
DiagnosticSQLResult<QVector<genre_stats::FicGenreData>> GetGenreDataForQueuedFics(sql::Database db);
DiagnosticSQLResult<bool> QueueFicsForGenreDetection(int minAuthorRecs, int minFoundLists, int minFaves, sql::Database db);



DiagnosticSQLResult<bool> AssignIterationOfSlash(QString iteration, sql::Database db);


DiagnosticSQLResult<bool> WipeAuthorStatisticsRecords(sql::Database db);
DiagnosticSQLResult<bool> CreateStatisticsRecordsForAuthors(sql::Database db);
DiagnosticSQLResult<bool> CalculateSlashStatisticsPercentages(QString usedField, sql::Database db);

DiagnosticSQLResult<bool> WriteFicRecommenderRelationsForRecList(int listId,
                                                            QHash<uint32_t,QVector<uint32_t>>,
                                                            sql::Database db);
DiagnosticSQLResult<bool> WriteAuthorStatsForRecList(int listId,
                                                            const QVector<core::AuthorResult> &,
                                                            sql::Database db);





DiagnosticSQLResult<QList<core::AuhtorStatsPtr>> GetRecommenderStatsForList(int listId, QString sortOn, QString order, sql::Database db);
DiagnosticSQLResult<QList<QSharedPointer<core::RecommendationList>>> GetAvailableRecommendationLists(sql::Database db);
DiagnosticSQLResult<QSharedPointer<core::RecommendationList>> GetRecommendationList(int listid, sql::Database db);
DiagnosticSQLResult<QSharedPointer<core::RecommendationList>> GetRecommendationList(QString name, sql::Database db);

DiagnosticSQLResult<int> GetMatchCountForRecommenderOnList(int authorId, int list, sql::Database db);

DiagnosticSQLResult<QVector<int>> GetAllFicIDsFromRecommendationList(int listId, core::StoryFilter::ESourceListLimiter limiter, sql::Database db);
DiagnosticSQLResult<QVector<int>> GetAllSourceFicIDsFromRecommendationList(int listId, sql::Database db);



DiagnosticSQLResult<core::RecommendationListFicSearchToken>  GetRelevanceScoresInFilteredReclist(const core::ReclistFilter &filter, sql::Database db);



DiagnosticSQLResult<QStringList> GetAllAuthorNamesForRecommendationList(int listId, sql::Database db);

DiagnosticSQLResult<int> GetCountOfTagInAuthorRecommendations(int authorId, QString tag, sql::Database db);
DiagnosticSQLResult<int> GetMatchesWithListIdInAuthorRecommendations(int authorId, int listId, sql::Database db);


DiagnosticSQLResult<bool> DeleteRecommendationList(int listId, sql::Database db );
DiagnosticSQLResult<bool> DeleteRecommendationListData(int listId, sql::Database db );

DiagnosticSQLResult<bool> CopyAllAuthorRecommendationsToList(int authorId, int listId, sql::Database db);
DiagnosticSQLResult<bool> WriteAuthorRecommendationStatsForList(int listId, core::AuhtorStatsPtr stats, sql::Database db);
DiagnosticSQLResult<bool> RemoveAuthorRecommendationStatsFromDatabase(int listId, int authorId, sql::Database db);
//bool UploadLinkedAuthorsForAuthor(int authorId, QStringList list, sql::Database db);
DiagnosticSQLResult<bool> UploadLinkedAuthorsForAuthor(int authorId, QString website, QList<int> ids, sql::Database db);
DiagnosticSQLResult<bool> DeleteLinkedAuthorsForAuthor(int authorId,  sql::Database db);
DiagnosticSQLResult<QVector<int>> GetAllUnprocessedLinkedAuthors(sql::Database db);


DiagnosticSQLResult<bool> CreateOrUpdateRecommendationList(QSharedPointer<core::RecommendationList> list, QDateTime creationTimestamp, sql::Database db);
DiagnosticSQLResult<bool> WriteAuxParamsForReclist(QSharedPointer<core::RecommendationList> list, sql::Database db);

DiagnosticSQLResult<bool> UpdateFicCountForRecommendationList(int listId, sql::Database db);
DiagnosticSQLResult<QList<int>> GetRecommendersForFicIdAndListId(int ficId, sql::Database db);
DiagnosticSQLResult<QSet<int>> GetAllTaggedFics(sql::Database db);
DiagnosticSQLResult<QSet<int>> GetFicsTaggedWith(QStringList tags, bool useAND, sql::Database db);
DiagnosticSQLResult<QSet<int>> GetAuthorsForTags(QStringList tags, sql::Database db);
DiagnosticSQLResult<QHash<QString, int>> GetTagSizes(QStringList tags, sql::Database db);
DiagnosticSQLResult<bool> RemoveTagsFromEveryFic(QStringList tags, sql::Database db);


// assumes that ficsForSelection are filled
DiagnosticSQLResult<QHash<int, core::FanficCompletionStatus>> GetSnoozeInfo(sql::Database db);
DiagnosticSQLResult<QHash<int, core::FanficSnoozeStatus>> GetUserSnoozeInfo(bool fetchExpired = true, bool limitedSelection = false, sql::Database db = {});


DiagnosticSQLResult<bool> WriteExpiredSnoozes(QSet<int> ,sql::Database db);
DiagnosticSQLResult<bool> SnoozeFic(const core::FanficSnoozeStatus &, sql::Database db);
DiagnosticSQLResult<bool> RemoveSnooze(int,sql::Database db);

DiagnosticSQLResult<bool> AddNoteToFic(int, QString, sql::Database db);
DiagnosticSQLResult<bool> RemoveNoteFromFic(int, sql::Database db);


DiagnosticSQLResult<QHash<int, QString>> GetNotesForFics(bool limitedSelection = false, sql::Database db = {});
DiagnosticSQLResult<QHash<int, int>> GetReadingChaptersForFics(bool limitedSelection = false, sql::Database db = {});




DiagnosticSQLResult<QVector<int>> GetAllFicsThatDontHaveDBID( sql::Database db);
DiagnosticSQLResult<bool> FillDBIDsForFics(QVector<core::Identity>, sql::Database db);
DiagnosticSQLResult<bool> FetchTagsForFics(QVector<core::Fanfic> * fics, sql::Database db);

DiagnosticSQLResult<bool> FetchSnoozesForFics(QVector<core::Fanfic> * fics, sql::Database db);
DiagnosticSQLResult<bool> FetchReadingChaptersForFics(QVector<core::Fanfic> * fics, sql::Database db);
//DiagnosticSQLResult<bool> FetchNotesForFics(QVector<core::Fic> * fics, sql::Database db);



DiagnosticSQLResult<bool> FetchRecommendationsBreakdown(QVector<core::Fanfic> * fics, int listId, sql::Database db);
DiagnosticSQLResult<bool> FetchRecommendationScoreForFics(QHash<int, int> &scores, const core::ReclistFilter&, sql::Database db);

DiagnosticSQLResult<bool> LoadPlaceAndRecommendationsData(QVector<core::Fanfic> * fics, const core::ReclistFilter &, sql::Database db);

DiagnosticSQLResult<QSharedPointer<core::RecommendationList>> FetchParamsForRecList(int id, sql::Database db);




DiagnosticSQLResult<QStringList> GetLinkedPagesForList(int listId, QString website, sql::Database db);
DiagnosticSQLResult<bool> SetFicsAsListOrigin(QVector<int> ficIds, int listId, sql::Database db);
DiagnosticSQLResult<bool>  FillRecommendationListWithData(int listId, const QHash<int, int> &, sql::Database db);




DiagnosticSQLResult<bool> DeleteTagFromDatabase(QString tag, sql::Database db);
DiagnosticSQLResult<bool>  CreateTagInDatabase(QString tag, sql::Database db);
DiagnosticSQLResult<bool>  ExportTagsToDatabase (sql::Database originDB, sql::Database targetDB);
DiagnosticSQLResult<bool>  ImportTagsFromDatabase(sql::Database originDB, sql::Database targetDB);


DiagnosticSQLResult<bool>  ExportSlashToDatabase (sql::Database originDB, sql::Database targetDB);
DiagnosticSQLResult<bool>  ImportSlashFromDatabase(sql::Database originDB, sql::Database targetDB);

DiagnosticSQLResult<bool>  AddAuthorFavouritesToList(int authorId, int listId, sql::Database db);
DiagnosticSQLResult<bool>  ShortenFFNurlsForAllFics(sql::Database db);

//! todo  currently unused
DiagnosticSQLResult<int> GetRecommendationListIdForName(QString name, sql::Database db);
//will need to add genre tracker on ffn in case it'sever expanded
DiagnosticSQLResult<bool> IsGenreList(QStringList list, QString website, sql::Database db);

DiagnosticSQLResult<QVector<int>> GetIdList(QString where, sql::Database db);
DiagnosticSQLResult<QVector<int>> GetWebIdList(QString where, QString website, sql::Database db);
DiagnosticSQLResult<bool> DeactivateStory(int id, QString website, sql::Database db);

DiagnosticSQLResult<bool> UpdateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, sql::Database db);
DiagnosticSQLResult<bool> UpdateAuthorFavouritesUpdateDate(int authorId, QDateTime date, sql::Database db);

DiagnosticSQLResult<bool> CreateAuthorRecord(core::AuthorPtr author, QDateTime timestamp, sql::Database db);
DiagnosticSQLResult<QStringList> ReadUserTags(sql::Database db);
DiagnosticSQLResult<bool> PushTaglistIntoDatabase(QStringList, sql::Database);
DiagnosticSQLResult<bool> IncrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, sql::Database db);
DiagnosticSQLResult<bool> DecrementAllValuesInListMatchingAuthorFavourites(int authorId, int listId, sql::Database db);
DiagnosticSQLResult<QSet<QString>> GetAllGenres(sql::Database db);
// those are required for managing recommendation lists and somewhat outdated
// moved them to dump temporarily
//void RemoveAuthor(const core::Author &recommender);
//void RemoveAuthor(int id);
DiagnosticSQLResult<bool> FillFicDataForList(int listId,
                                             const QVector<int>&,
                                             const QVector<int>&,
                                             const QSet<int> &origins,
                                             sql::Database db);

DiagnosticSQLResult<bool> FillFicDataForList(QSharedPointer<core::RecommendationList>,
                                             sql::Database db);




// potentially ethically problematic. better not do this
//DiagnosticSQLResult<bool> FillAuthorDataForList(int listId, const QVector<int>&, sql::Database db);

// page tasks
DiagnosticSQLResult<int> GetLastExecutedTaskID(sql::Database db);
DiagnosticSQLResult<bool> GetTaskSuccessByID(int id, sql::Database db);
DiagnosticSQLResult<bool>  IsForceStopActivated(int id, sql::Database db);

DiagnosticSQLResult<PageTaskPtr> GetTaskData(int id, sql::Database db);
DiagnosticSQLResult<SubTaskList> GetSubTaskData(int id, sql::Database db);
DiagnosticSQLResult<SubTaskErrors> GetErrorsForSubTask(int id, sql::Database db, int subId = -1);
DiagnosticSQLResult<PageTaskActions> GetActionsForSubTask(int id, sql::Database db, int subId = -1);

DiagnosticSQLResult<int> CreateTaskInDB(PageTaskPtr, sql::Database);
DiagnosticSQLResult<bool> CreateSubTaskInDB(SubTaskPtr, sql::Database);
DiagnosticSQLResult<bool> CreateActionInDB(PageTaskActionPtr, sql::Database);
DiagnosticSQLResult<bool> CreateErrorsInDB(const SubTaskErrors &, sql::Database);

DiagnosticSQLResult<bool> UpdateTaskInDB(PageTaskPtr, sql::Database);
DiagnosticSQLResult<bool> UpdateSubTaskInDB(SubTaskPtr, sql::Database);
DiagnosticSQLResult<bool> SetTaskFinished(int, sql::Database);
DiagnosticSQLResult<TaskList> GetUnfinishedTasks(sql::Database);
DiagnosticSQLResult<bool>  EnsureUUIDForUserDatabase(QUuid id, sql::Database db);
DiagnosticSQLResult<QString> GetUserToken(sql::Database db);
DiagnosticSQLResult<int> GetLastFandomID(sql::Database db);


DiagnosticSQLResult<bool> PassScoresToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
DiagnosticSQLResult<bool> PassReadingDataToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
DiagnosticSQLResult<bool> PassSnoozesToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
DiagnosticSQLResult<bool> PassFicTagsToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
DiagnosticSQLResult<bool> PassFicNotesToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
DiagnosticSQLResult<bool> PassTagSetToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
DiagnosticSQLResult<bool> PassRecentFandomsToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
DiagnosticSQLResult<bool> PassIgnoredFandomsToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
DiagnosticSQLResult<bool> PassClientDataToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
DiagnosticSQLResult<bool> PassFandomListSetToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);
DiagnosticSQLResult<bool> PassFandomListDataToAnotherDatabase(sql::Database dbSource, sql::Database dbTarget);


DiagnosticSQLResult<DBVerificationResult> VerifyDatabaseIntegrity(sql::Database db);

namespace Internal{
DiagnosticSQLResult<bool> WriteMaxUpdateDateForFandom(QSharedPointer<core::Fandom> fandom,
                                 QString condition,
                                 sql::Database db,
                                 std::function<void(QSharedPointer<core::Fandom>, QDateTime)> writer
                                 );
}

}


