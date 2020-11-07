#pragma once
#include "sql_abstractions/sql_transaction.h"
#include "sql_abstractions/sql_query.h"
#include "sql_abstractions/sql_error.h"
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/string_trimmer.h"
#include <fmt/format.h>
#include <functional>
#include <memory>
#include <array>
#include <unordered_map>
#include <iostream>


namespace sql{
bool ExecAndCheck(Query& q, bool reportErrors = true,  bool ignoreUniqueness = false);

template <typename T>
struct DiagnosticSQLResult
{
    bool success = true;
    std::string oracleError;
    T data;
    bool ExecAndCheck(Query& q, bool ignoreUniqueness = false) {
        bool success = sql::ExecAndCheck(q, true, ignoreUniqueness);
        bool uniqueTriggered = ignoreUniqueness && q.lastError().text().find("UNIQUE constraint failed") != std::string::npos;
        if(uniqueTriggered)
            return true;
        if(!success && !uniqueTriggered)
        {
            success = false;
            oracleError = q.lastError().text();
            std::cout << oracleError;
            std::cout << q.lastQuery();
        }
        return success;
    }
    bool CheckDataAvailability(Query& q, bool allowEmptyRecords = false){
        if(!q.next())
        {
            if(!allowEmptyRecords)
            {
                success = false;
                oracleError = "no data to read";
            }
            return false;
        }
        return true;
    }
};


template <typename ResultType>
struct SqlContext
{
    SqlContext(Database db) : q(db), transaction(db){
    }
    SqlContext(Database db, std::string&& qs) :qs(qs), q(db), transaction(db) {
        Prepare(qs);
    }

    SqlContext(Database db, std::list<std::string>&& queries) : q(db), transaction(db)
    {
        for(const auto& query : queries)
        {
            Prepare(query);
            BindValues();
            ExecAndCheck();
        }
    }

    SqlContext(Database db, std::string&& qs,  std::function<void(SqlContext<ResultType>*)> func) : qs(qs), q(db), transaction(db),  func(func){
        Prepare(qs);
        func(this);
    }

    SqlContext(Database db, std::string&& qs, std::unordered_map<std::string, Query::Variant>&& hash) :  qs(qs), q(db),  transaction(db){
        Prepare(qs);

        for(auto i = hash.begin(); i != hash.end(); i++)
            bindValue(i->first, std::move(i->second));
    }

    SqlContext(Database db, std::unordered_map<std::string, Query::Variant>&& hash) :  q(db),  transaction(db){
        for(auto i = hash.begin(); i != hash.end(); i++)
            bindValue(i->first, std::move(i->second));
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
    void ExecuteWithArgsHash(std::vector<std::string> nameKeys, std::unordered_map<HashKey, HashValue> args, bool ignoreUniqueness = false){
        BindValues();
        for(const auto& key : args.keys())
        {
            q.bindValue(":" + nameKeys[0], key);
            q.bindValue(":" + nameKeys[1], args[key]);
            if(!ExecAndCheck(ignoreUniqueness))
            {
                qDebug() << "breaking out of cycle";
                break;
            }
        }
    }
    template <typename KeyType>
    void ExecuteWithKeyListAndBindFunctor(std::vector<KeyType> keyList, std::function<void(KeyType&& key, Query& q)>&& functor, bool ignoreUniqueness = false){
        BindValues();
        for(auto&& key : keyList)
        {
            functor(std::move(key), q);
            if(!ExecAndCheck(ignoreUniqueness))
                break;
        }
    }

    template <typename ValueType>
    void ExecuteWithValueList(std::string keyName, std::vector<ValueType>&& valueList, bool ignoreUniqueness = false){
        BindValues();
        for(auto&& value : valueList)
        {
            q.bindValue(":" + keyName, std::move(value));
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
    void FetchSelectFunctor(std::string&& select, std::function<void(ResultType& data, Query& q)>&& f, bool allowEmptyRecords = false)
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
                                  std::function<ValueType(Query&)>&& func = std::function<ValueType(Query&)>())
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
        int size = std::get<int>(q.value(0));
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
                result.data += std::get<typename ResultType::value_type>(q.value(fieldName.c_str()));
            else
                result.data += func(q);
        } while(q.next());
    }

    template <typename ValueType>
    void FetchLargeSelectIntoListWithoutSize(std::string&& fieldName, std::string&& actualQuery,
                                  std::function<ValueType(Query&)>&& func = std::function<ValueType(Query&)>())
    {
        qs = actualQuery;
        Prepare(qs);

        if(!ExecAndCheck())
            return;
        if(!CheckDataAvailability())
            return;

        do{
            if(!func)
                result.data += std::get<typename ResultType::value_type>(q.value(fieldName.c_str()));
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
            result.data[std::get<typename ResultType::key_type>(q.value(idFieldName.c_str()))] =  std::get<typename ResultType::mapped_type>(q.value(valueFieldName.c_str()));;
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
        result.data = std::get<T>(q.value(valueName.c_str()));
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
        for(const QueryBinding& bind : std::as_const(bindValues))
        {
            q.bindValue(":" + bind.key, bind.value);
        }
    }

    //template<typename KeyType, typename LamdaType>
    //void ProcessKeys(QList<KeyType> keys, const LamdaType& func){
    template<typename KeyType>
    void ProcessKeys(std::vector<KeyType> keys, const std::function<void(std::string key, Query&)>& func){
        for(auto key : keys)
            func(key, q);
    }

    void for_each(std::function<void(Query&)> func){
        while(q.next())
            func(q);
    }

    DiagnosticSQLResult<ResultType> ForEachInSelect(const std::function<void(Query&)>& func){
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
        bool success = q.prepare(std::string(qs));
        return success;
    }

    Query::Variant value(std::string name){return q.value(name);}
    std::string trimmedValue(std::string name){return trim_copy(std::get<std::string>(q.value(name)));}

    void bindValue(std::string&& key, const Query::Variant& value){
        auto it = std::find_if(bindValues.begin(), bindValues.end(), [key](const QueryBinding& b){
            return b.key == key;
        });
        if(it!=bindValues.cend())
            *it=QueryBinding{it->key, value};
        else
            bindValues.emplace_back(QueryBinding{key, value});
    }
    void bindValue(const std::string& key, const Query::Variant& value){
        auto it = std::find_if(bindValues.begin(), bindValues.end(), [key](const QueryBinding& b){
            return b.key == key;
        });
        if(it!=bindValues.cend())
            *it = QueryBinding{it->key, value};
        else
            bindValues.emplace_back(QueryBinding{key, value});
    }

    void bindValue(const std::string& key, Query::Variant&& value){
        auto it = std::find_if(bindValues.begin(), bindValues.end(), [key](const QueryBinding& b){
            return b.key == key;
        });
        if(it!=bindValues.cend())
            *it = QueryBinding{it->key, value};
        else
            bindValues.emplace_back(QueryBinding{key, value});
    }
    void bindValue(std::string&& key, Query::Variant&& value){
        auto it = std::find_if(bindValues.begin(), bindValues.end(), [key](const QueryBinding& b){
            return b.key == key;
        });
        if(it!=bindValues.cend())
            *it = QueryBinding{it->key, value};
        else
            bindValues.emplace_back(QueryBinding{key, value});
    }
    void SetDefaultValue(ResultType value) {result.data = value;}
    bool Success() const {return result.success;}
    bool Next() { return q.next();}
    DiagnosticSQLResult<ResultType> result;
    std::string qs;
    Query q;
    Transaction transaction;
    std::list<QueryBinding> bindValues;
    std::function<void(SqlContext<ResultType>*)> func;
};




template <typename ResultType>
struct ParallelSqlContext
{
    ParallelSqlContext(Database source, std::string&& sourceQuery, std::vector<std::string>&& sourceFields,
                       Database target, std::string&& targetQuery, std::vector<std::string>&& targetFields):
        sourceQ(source), targetQ(target),
        sourceDB(source), targetDB(target), transaction(target) {
        sourceQ.prepare(sourceQuery);
        targetQ.prepare(targetQuery);
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

        Query::Variant value;
        while(sourceQ.next())
        {
            for(int i = 0; i < sourceFields.size(); ++i )
            {
                //qDebug() << "binding field: " << sourceFields[i];
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
    Query sourceQ;
    Query targetQ;
    Database sourceDB;
    Database targetDB;
    std::vector<std::string> sourceFields;
    std::vector<std::string> targetFields;
    Transaction transaction;
    std::unordered_map<std::string,std::function<Query::Variant(const std::string&, Query, Database, DiagnosticSQLResult<ResultType>&)>> valueConverters;
};

}

