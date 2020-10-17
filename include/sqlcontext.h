#pragma once
#include "transaction.h"
#include "logger/QsLog.h"
#include <fmt/format.h>

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QSharedPointer>

#include <functional>
#include <memory>
#include <array>
#include <unordered_map>

namespace std
{
    inline namespace __cxx11{
    inline uint qHash(const std::string& key, uint seed = 0)
     {
         return qHash(QByteArray::fromRawData(key.data(), key.length()), seed);
     }
    }
}



namespace database {
namespace puresql{


struct QueryBinding{
    std::string key;
    QVariant value;
};


bool ExecAndCheck(QSqlQuery& q, bool reportErrors = true,  bool ignoreUniqueness = false);

template <typename T>
struct DiagnosticSQLResult
{
    bool success = true;
    QString oracleError;
    T data;
    bool ExecAndCheck(QSqlQuery& q, bool ignoreUniqueness = false) {
        bool success = database::puresql::ExecAndCheck(q, true, ignoreUniqueness);
        bool uniqueTriggered = ignoreUniqueness && q.lastError().text().contains("UNIQUE constraint failed");
        if(uniqueTriggered)
            return true;
        if(!success && !uniqueTriggered)
        {
            success = false;
            oracleError = q.lastError().text();
            qDebug().noquote() << oracleError;
            qDebug().noquote() << q.lastQuery();
        }
        return success;
    }
    bool CheckDataAvailability(QSqlQuery& q, bool allowEmptyRecords = false){
        if(!q.next())
        {
            if(!allowEmptyRecords)
            {
                success = false;
                oracleError = QStringLiteral("no data to read");
            }
            return false;
        }
        return true;
    }
};


template <typename ResultType>
struct SqlContext
{
    SqlContext(QSqlDatabase db) : q(db), transaction(db){
    }
    SqlContext(QSqlDatabase db, std::string&& qs) :qs(qs), q(db), transaction(db) {
        Prepare(qs);
    }

    SqlContext(QSqlDatabase db, std::list<std::string>&& queries) : q(db), transaction(db)
    {
        for(const auto& query : queries)
        {
            Prepare(query);
            BindValues();
            ExecAndCheck();
        }
    }

    SqlContext(QSqlDatabase db, std::string&& qs,  std::function<void(SqlContext<ResultType>*)> func) : qs(qs), q(db), transaction(db),  func(func){
        Prepare(qs);
        func(this);
    }

    SqlContext(QSqlDatabase db, std::string&& qs, std::unordered_map<std::string, QVariant> hash) :  qs(qs), q(db),  transaction(db){
        Prepare(qs);

        for(auto i = hash.begin(); i != hash.end(); i++)
            bindMoveValue(std::move(i->first), std::move(i->second));
    }

    SqlContext(QSqlDatabase db, std::unordered_map<std::string, QVariant> hash) :  q(db),  transaction(db){
        for(auto i = hash.begin(); i != hash.end(); i++)
            bindMoveValue(std::move(i->first), std::move(i->second));
    }

    ~SqlContext(){
        if(!result.success)
            transaction.cancel();
        else
            transaction.finalize();
    }

    DiagnosticSQLResult<ResultType> operator()(bool ignoreUniqueness = false){
        BindValues();
        ExecAndCheck(ignoreUniqueness);
        return result;
    }

    void ReplaceQuery(std::string&& query){
        qs = query;
        Prepare(qs);
        bindValues.clear();
    }

    void ExecuteWithArgsSubstitution(std::list<std::string>&& keys){
        for(const auto& key : keys)
        {
            auto newString = qs;
            newString = fmt::format(newString, key);
            Prepare(newString);
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
            //            for(QString nameKey: nameKeys)
            //            {
            //qDebug() << "cycling";
            q.bindValue(":" + nameKeys[0], key);
            q.bindValue(":" + nameKeys[1], args[key]);
            //}
            if(!ExecAndCheck(ignoreUniqueness))
            {
                qDebug() << "breaking out of cycle";
                break;
            }
        }
    }
    template <typename KeyType>
    void ExecuteWithKeyListAndBindFunctor(QList<KeyType> keyList, std::function<void(KeyType& key, QSqlQuery& q)> functor, bool ignoreUniqueness = false){
        BindValues();
        for(auto key : keyList)
        {
            functor(key, q);
            if(!ExecAndCheck(ignoreUniqueness))
                break;
        }
    }

    template <typename KeyType>
    void ExecuteWithValueList(QString keyName, QList<KeyType> valueList, bool ignoreUniqueness = false){
        BindValues();
        for(auto value : valueList)
        {
            q.bindValue(":" + keyName, value);
            if(!ExecAndCheck(ignoreUniqueness))
                break;
        }
    }
    template <typename KeyType>
    void ExecuteWithValueList(QString keyName, QVector<KeyType> valueList, bool ignoreUniqueness = false){
        BindValues();
        for(auto value : valueList)
        {
            q.bindValue(":" + keyName, value);
            if(!ExecAndCheck(ignoreUniqueness))
                break;
        }
    }


    bool ExecAndCheck(bool ignoreUniqueness = false){
        BindValues();
        return result.ExecAndCheck(q, ignoreUniqueness);
    }
    bool CheckDataAvailability(bool allowEmptyRecords = false){
        return result.CheckDataAvailability(q, allowEmptyRecords);
    }
    bool ExecAndCheckForData(){
        BindValues();
        result.ExecAndCheck(q);
        if(!result.success)
            return false;
        result.CheckDataAvailability(q);
        if(!result.success)
            return false;
        return true;
    }
    void FetchSelectFunctor(std::string&& select, std::function<void(ResultType& data, QSqlQuery& q)> f, bool allowEmptyRecords = false)
    {
        Prepare(select);
        BindValues();

        if(!ExecAndCheck())
            return;

        if(!CheckDataAvailability(allowEmptyRecords))
            return;

        do{
            f(result.data, q);
        } while(q.next());
    }
    template <typename ValueType>
    void FetchLargeSelectIntoList(std::string&& fieldName, std::string&& actualQuery, std::string&& countQuery = "",
                                  std::function<ValueType(QSqlQuery&)> func = std::function<ValueType(QSqlQuery&)>())
    {
        if(countQuery.length() == 0)
            qs = "select count(*) from ( " + actualQuery + " ) ";
        else
            qs = countQuery;
        Prepare(qs);
        if(!ExecAndCheck())
            return;

        if(!CheckDataAvailability())
            return;
        int size = q.value(0).toInt();
        //qDebug () << "query size: " << size;
        if(size == 0)
            return;
        result.data.reserve(size);

        qs = actualQuery;
        Prepare(qs);
        //BindValues();

        if(!ExecAndCheck())
            return;
        if(!CheckDataAvailability())
            return;

        do{
            if(!func)
                result.data += q.value(fieldName.c_str()).template value<typename ResultType::value_type>();
            else
                result.data += func(q);
        } while(q.next());
    }

    template <typename ValueType>
    void FetchLargeSelectIntoListWithoutSize(std::string&& fieldName, std::string&& actualQuery,
                                  std::function<ValueType(QSqlQuery&)> func = std::function<ValueType(QSqlQuery&)>())
    {
        qs = actualQuery;
        Prepare(qs);

        if(!ExecAndCheck())
            return;
        if(!CheckDataAvailability())
            return;

        do{
            if(!func)
                result.data += q.value(fieldName.c_str()).template value<typename ResultType::value_type>();
            else
                result.data += func(q);
        } while(q.next());
    }

    void FetchSelectIntoHash(std::string&& actualQuery, std::string&& idFieldName, std::string&& valueFieldName)
    {
        qs = actualQuery;
        Prepare(qs);
        BindValues();


        if(!ExecAndCheck())
            return;
        if(!CheckDataAvailability())
            return;
        do{
            result.data[q.value(idFieldName.c_str()).template value<typename ResultType::key_type>()] =  q.value(valueFieldName.c_str()).template value<typename ResultType::mapped_type>();
        } while(q.next());
    }

    template <typename T>
    void FetchSingleValue(std::string&& valueName,
                          ResultType defaultValue,
                          bool requireExisting = true,
                          std::string&& select = ""
            ){
        result.data = defaultValue;
        if(select.length() != 0)
        {
            qs = select;
            Prepare(qs);
            BindValues();
        }
        if(!ExecAndCheck())
            return;
        if(!CheckDataAvailability())
        {
            if(!requireExisting)
                result.success = true;
            return;
        }
        result.data = q.value(valueName.c_str()).template value<T>();
    }

    void ExecuteList(std::list<std::string>&& queries){
        bool execResult = true;
        for(const auto& query : queries)
        {
            Prepare(query);
            BindValues();
            if(!ExecAndCheck())
            {
                execResult = false;
                break;
            }
        }
        result.data = execResult;
    }

    void BindValues(){
        for(const auto& bind : std::as_const(bindValues))
        {
            q.bindValue(":" + QString::fromStdString(bind.key), bind.value);
        }
    }

    template<typename KeyType>
    void ProcessKeys(QList<KeyType> keys, const std::function<void(QString key, QSqlQuery&)>& func){
        for(auto key : keys)
            func(key, q);
    }

    void for_each(std::function<void(QSqlQuery&)> func){
        while(q.next())
            func(q);
    }

    DiagnosticSQLResult<ResultType> ForEachInSelect(const std::function<void(QSqlQuery&)>& func){
        BindValues();
        if(!ExecAndCheck())
            return result;
        for_each(func);
        return result;
    }
    bool Prepare(std::string_view qs){
        if(qs.length() == 0)
        {
            qDebug() << "passed empty query";
            return true;
        }
        bool success = q.prepare(QString::fromStdString(std::string(qs)));
        return success;
    }

    QVariant value(QString name){return q.value(name);}
    QString trimmedValue(QString name){return q.value(name).toString().trimmed();}

    void bindValue(std::string&& key, QVariant value){
        auto it = std::find_if(bindValues.cbegin(), bindValues.cend(), [key](const QueryBinding& b){
            return b.key == key;
        });
        if(it!=bindValues.cend())
        {
            auto it = std::remove_if(bindValues.begin(), bindValues.end(), [key](const QueryBinding& b){
                return b.key == key;
            });
            bindValues.erase(it, bindValues.end());
        }
        bindValues.push_back({std::move(key), value});
    }
    void bindValue(const std::string& key, QVariant value){
        auto it = std::find_if(bindValues.cbegin(), bindValues.cend(), [key](const QueryBinding& b){
            return b.key == key;
        });
        if(it!=bindValues.cend())
        {
            auto it = std::remove_if(bindValues.begin(), bindValues.end(), [key](const QueryBinding& b){
                return b.key == key;
            });
            bindValues.erase(it, bindValues.end());
        }
        bindValues.push_back({key, value});
    }
    void bindMoveValue(std::string&& key, QVariant&& value){
        auto it = std::find_if(bindValues.cbegin(), bindValues.cend(), [key](const QueryBinding& b){
            return b.key == key;
        });
        if(it!=bindValues.cend())
        {
            auto it = std::remove_if(bindValues.begin(), bindValues.end(), [key](const QueryBinding& b){
                return b.key == key;
            });
            bindValues.erase(it, bindValues.end());
        }
        bindValues.push_back({std::move(key), std::move(value)});
    }
    void bindMoveValue(const std::string& key, QVariant&& value){
        auto it = std::find_if(bindValues.cbegin(), bindValues.cend(), [key](const QueryBinding& b){
            return b.key == key;
        });
        if(it!=bindValues.cend())
        {
            auto it = std::remove_if(bindValues.begin(), bindValues.end(), [key](const QueryBinding& b){
                return b.key == key;
            });
            bindValues.erase(it, bindValues.end());
        }
        bindValues.push_back({key, std::move(value)});
    }
    void SetDefaultValue(ResultType value) {result.data = value;}
    bool Success() const {return result.success;}
    bool Next() { return q.next();}
    DiagnosticSQLResult<ResultType> result;
    std::string qs;
    QSqlQuery q;
    Transaction transaction;
    QList<QueryBinding> bindValues;
    std::function<void(SqlContext<ResultType>*)> func;
};




template <typename ResultType>
struct ParallelSqlContext
{
    ParallelSqlContext(QSqlDatabase source, std::string&& sourceQuery, QList<std::string>&& sourceFields,
                       QSqlDatabase target, std::string&& targetQuery, QList<std::string>&& targetFields):
        sourceQ(source), targetQ(target),
        sourceDB(source), targetDB(target), transaction(target) {
        sourceQ.prepare(QString::fromStdString(sourceQuery));
        targetQ.prepare(QString::fromStdString(targetQuery));
        this->sourceFields = sourceFields;
        this->targetFields = targetFields;
    }

    ~ParallelSqlContext(){
        if(!result.success)
            transaction.cancel();
    }

    DiagnosticSQLResult<ResultType> operator()(bool ignoreUniqueness = false){
        if(!result.ExecAndCheck(sourceQ))
            return result;
        while(sourceQ.next())
        {
            for(int i = 0; i < sourceFields.size(); ++i )
            {
                //qDebug() << "binding field: " << sourceFields[i];
                QVariant value;
                if(valueConverters.contains(sourceFields[i]))
                {
                    value = valueConverters.value(sourceFields.at(i))(sourceFields.at(i), sourceQ, targetDB, result);
                    if(!result.success)
                        return result;
                }
                else
                {
                    value = sourceQ.value(sourceFields.at(i).c_str());
                    //qDebug() << "binding value: " << value;
                }
                //qDebug() << "to target field: " << targetFields[i];
                targetQ.bindValue((":" + targetFields[i]).c_str(), value);
            }

            if(!result.ExecAndCheck(targetQ, ignoreUniqueness))
                return result;
        }
        return result;
    }
    bool Success() const {return result.success;}
    DiagnosticSQLResult<ResultType> result;
    QSqlQuery sourceQ;
    QSqlQuery targetQ;
    QSqlDatabase sourceDB;
    QSqlDatabase targetDB;
    QList<std::string> sourceFields;
    QList<std::string> targetFields;
    Transaction transaction;
    QHash<std::string,std::function<QVariant(std::string, QSqlQuery, QSqlDatabase, DiagnosticSQLResult<ResultType>&)>> valueConverters;
};

}
}
