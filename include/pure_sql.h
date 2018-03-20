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


template <typename ResultType>
struct SqlContext
{
    //SqlContext(QSqlDatabase db): q(db),transaction(db){}
    SqlContext(QSqlDatabase db, QString qs = "") : q(db), transaction(db), qs(qs){
        q.prepare(qs);
    }

    SqlContext(QSqlDatabase db, QStringList queries) : q(db), transaction(db), qs(qs){
        for(auto query : queries)
        {
            q.prepare(query);
            BindValues();
            ExecAndCheck();
        }
    }

    SqlContext(QSqlDatabase db, QString qs,  std::function<void(SqlContext<ResultType>*)> func) : q(db), transaction(db), qs(qs), func(func){
        q.prepare(qs);
        func(this);
    }

    SqlContext(QSqlDatabase db, QString qs, QVariantHash hash) : q(db), transaction(db), qs(qs), func(func){
        q.prepare(qs);
        for(auto valName: hash.keys())
            q.bindValue(valName, hash[valName]);
    }

    ~SqlContext(){
        if(!result.success)
            transaction.cancel();
    }

    void ExecuteWithArgsSubstitution(QStringList keys){
        for(auto key : keys)
        {
            QString newString = qs;
            newString = newString.arg(key);
            q.prepare(newString);
            BindValues();
            ExecAndCheck();
            if(!result.success)
                break;
        }
    }

    template <typename HashKey, typename HashValue>
    void ExecuteWithArgsHash(QStringList nameKeys, QHash<HashKey, HashValue> args, bool ignoreUniqueness = false){
        BindValues();
        for(auto key : args.keys())
        {
            for(QString nameKey: nameKeys)
            {
                q.bindValue(nameKey, key);
                q.bindValue(nameKey, args[key]);
            }
            if(!ExecAndCheck(ignoreUniqueness))
                break;
        }
    }

    DiagnosticSQLResult<ResultType> operator()(bool ignoreUniqueness = false){
        BindValues();
        if(ExecAndCheck(ignoreUniqueness))
            transaction.finalize();
        return result;
    }
    bool ExecAndCheck(bool ignoreUniqueness = false){
        BindValues();
        return result.ExecAndCheck(q, ignoreUniqueness);
    }
    bool CheckDataAvailability(){
        return result.CheckDataAvailability(q);
    }
    void FetchSelectFunctor(QString select, std::function<void(ResultType& data, QSqlQuery& q)> f)
    {
        q.prepare(select);
        BindValues();

        if(!ExecAndCheck())
            return;
        if(!CheckDataAvailability())
            return;

        do{
            f(result.data, q);
        } while(q.next());
    }

    void FetchLargeSelectIntoList(QString fieldName, QString actualQuery, QString countQuery = "")
    {
        if(countQuery.isEmpty())
            qs = "select count(*) from ( " + actualQuery + " ) ";
        else
            qs = countQuery;
        q.prepare(qs);
        BindValues();

        if(!ExecAndCheck())
            return;

        if(!CheckDataAvailability())
            return;
        int size = q.value(0).toInt();
        result.data.reserve(size);

        qs = actualQuery;
        q.prepare(qs);
        BindValues();

        if(!ExecAndCheck())
            return;
        if(!CheckDataAvailability())
            return;

        do{
            result.data += q.value(fieldName).template value<ResultType::value_type>();
        } while(q.next());
    }

    void FetchSelectIntoHash(QString actualQuery, QString idFieldName, QString valueFieldName)
    {
        qs = actualQuery;
        q.prepare(qs);
        BindValues();


        if(!ExecAndCheck())
            return;
        if(!CheckDataAvailability())
            return;
        do{
            result.data[q.value(idFieldName).template value<ResultType::key_type>()] =  q.value(valueFieldName).template value<ResultType::mapped_type>();
        } while(q.next());
    }

    void ExecuteList(QStringList queries){
        bool execResult = true;
        for(auto query : queries)
        {
            q.prepare(query);
            BindValues();
            if(!ExecAndCheck())
            {
                execResult = false;
                break;
            }
        }
        result.data = execResult;
    }

    template <typename KeyType>
    void ExecuteWithKeyListAndBindFunctor(QList<KeyType> keyList, std::function<void(KeyType key, QSqlQuery& q)> functor){
        BindValues();
        for(auto key : keyList)
        {
            functor(key, q);
            if(!ExecAndCheck())
                break;
        }
    }

    void BindValues(){
        for(auto bind : bindValues.keys())
            q.bindValue(bind, bindValues[bind]);
    }

    template<typename KeyType>
    void ProcessKeys(QList<KeyType> keys, std::function<void(QString key, QSqlQuery&)> func){
        for(auto key : keys)
            func(key, q);
    }

    DiagnosticSQLResult<ResultType> result;
    QString qs;
    QSqlQuery q;
    Transaction transaction;
    QVariantHash bindValues;
    std::function<void(SqlContext<ResultType>*)> func;
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
DiagnosticSQLResult<bool> SetFandomTracked(int id, bool tracked, QSqlDatabase);
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
bool AssignSlashToFanfic(int fic_id, int source, QSqlDatabase db);

DiagnosticSQLResult<bool> PerformGenreAssignment(QSqlDatabase db);

DiagnosticSQLResult<bool> ProcessSlashFicsBasedOnWords(std::function<SlashPresence(QString, QString, QString)>, QSqlDatabase db);
DiagnosticSQLResult<bool> WipeSlashMetainformation(QSqlDatabase db);

bool CreateFandomInDatabase(QSharedPointer<core::Fandom> fandom, QSqlDatabase db);

QList<core::FandomPtr> GetAllFandoms(QSqlDatabase db);
QList<core::FandomPtr> GetAllFandomsFromSingleTable(QSqlDatabase db);
core::FandomPtr GetFandom(QString name, QSqlDatabase db);

DiagnosticSQLResult<bool>  IgnoreFandom(int id, bool includeCrossovers, QSqlDatabase db);
DiagnosticSQLResult<bool>  RemoveFandomFromIgnoredList(int id, QSqlDatabase db);
DiagnosticSQLResult<QStringList>  GetIgnoredFandoms(QSqlDatabase db);

DiagnosticSQLResult<bool>  IgnoreFandomSlashFilter(int id, QSqlDatabase db);
DiagnosticSQLResult<bool>  RemoveFandomFromIgnoredListSlashFilter(int id, QSqlDatabase db);
DiagnosticSQLResult<QStringList>  GetIgnoredFandomsSlashFilter(QSqlDatabase db);

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
DiagnosticSQLResult<QSet<int>>  GetAllKnownSlashFics(QSqlDatabase db);
DiagnosticSQLResult<QSet<int>>  GetAllKnownNotSlashFics(QSqlDatabase db);
DiagnosticSQLResult<QSet<int>>  GetAllKnownFicIds(QString, QSqlDatabase db);
DiagnosticSQLResult<QSet<int>>  GetSingularFicsInLargeButSlashyLists(QSqlDatabase db);
DiagnosticSQLResult<QHash<int, std::array<double, 21>>> GetListGenreData(QSqlDatabase db);
DiagnosticSQLResult<QHash<int, double>>  GetFicGenreData(QString genre, QString cutoff, QSqlDatabase db);
DiagnosticSQLResult<QHash<int, std::array<double, 21>>> GetFullFicGenreData(QSqlDatabase db);
DiagnosticSQLResult<QHash<int, double> > GetDoubleValueHashForFics(QString fieldName, QSqlDatabase db);




DiagnosticSQLResult<bool> AssignIterationOfSlash(QString iteration, QSqlDatabase db);


DiagnosticSQLResult<bool> WipeAuthorStatisticsRecords(QSqlDatabase db);
DiagnosticSQLResult<bool> CreateStatisticsRecordsForAuthors(QSqlDatabase db);
DiagnosticSQLResult<bool> CalculateSlashStatisticsPercentages(QString usedField, QSqlDatabase db);



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
bool DeleteRecommendationListData(int listId, QSqlDatabase db );

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


DiagnosticSQLResult<bool>  ExportSlashToDatabase (QSqlDatabase originDB, QSqlDatabase targetDB);
DiagnosticSQLResult<bool>  ImportSlashFromDatabase(QSqlDatabase originDB, QSqlDatabase targetDB);

bool AddAuthorFavouritesToList(int authorId, int listId, QSqlDatabase db);
void ShortenFFNurlsForAllFics(QSqlDatabase db);

//! todo  currently unused
int GetRecommendationListIdForName(QString name, QSqlDatabase db);
//will need to add genre tracker on ffn in case it'sever expanded
bool IsGenreList(QStringList list, QString website, QSqlDatabase db);

DiagnosticSQLResult<QVector<int>> GetIdList(QString where, QSqlDatabase db);
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
