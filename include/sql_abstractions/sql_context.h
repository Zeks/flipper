/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#pragma once
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_query.h"
#include "sql_abstractions/sql_error.h"
#include "sql_abstractions/sql_transaction.h"
#include "sql_abstractions/string_trimmer.h"
#include <fmt/format.h>

#include <string>
#include <functional>
#include <memory>
#include <array>
#include <unordered_map>
#include <iostream>
#include <QDebug>




namespace sql{

bool ExecAndCheck(sql::Query& q, bool reportErrors = true, std::vector<ESqlErrors> expectedErrors = {});

inline std::vector<ESqlErrors> ConvertBoolToError(bool errorEnabled, std::vector<ESqlErrors> expectedErrors){
    if(errorEnabled)
        return expectedErrors;
    return {};
};

template <typename T>
struct DiagnosticSQLResult
{
    bool success = true;
    Error sqlError;
    T data;
    bool ExecAndCheck(sql::Query& q, std::vector<ESqlErrors> expectedErrors = {}) {
        bool success = sql::ExecAndCheck(q, true, expectedErrors);
        if(!success)
        {
            success = false;
            sqlError = q.lastError();
        }
        return success;
    }
    bool CheckDataAvailability(sql::Query& q, bool allowEmptyRecords = false){
        if(!q.next())
        {
            if(!allowEmptyRecords)
            {
                success = false;
                sqlError = {"no data to read", ESqlErrors::se_no_data_to_be_fetched};
            }
            return false;
        }
        return true;
    }
};


template <typename ResultType>
struct SqlContext
{
    SqlContext(sql::Database db) : q(db), transaction(db){
    }
    SqlContext(sql::Database db, std::string&& qs) :qs(qs), q(db), transaction(db) {
        Prepare(qs);
    }

    SqlContext(sql::Database db, std::vector<std::string>&& queries) : q(db), transaction(db)
    {
        for(const auto& query : queries)
        {
            Prepare(query);
            BindValues();
            ExecAndCheck();
        }
    }

    SqlContext(sql::Database db, std::string&& qs,  std::function<void(SqlContext<ResultType>*)> func) : qs(qs), q(db), transaction(db),  func(func){
        Prepare(qs);
        func(this);
    }

    SqlContext(sql::Database db, std::string&& qs, std::unordered_map<std::string, Variant>&& hash) :  qs(qs), q(db),  transaction(db){
        Prepare(qs);

        for(auto i = hash.begin(); i != hash.end(); i++)
            bindValue(i->first, std::move(i->second));
    }

    SqlContext(sql::Database db, std::unordered_map<std::string, Variant>&& hash) :  q(db),  transaction(db){
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
        bindValues.clear();
        Prepare(qs);
    }

    void ExecuteWithArgsSubstitution(std::vector<std::string>&& keys){
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
            q.bindValue(nameKeys[0], key);
            q.bindValue(nameKeys[1], args[key]);
            if(!ExecAndCheck(ignoreUniqueness))
            {
                qDebug() << "breaking out of cycle";
                break;
            }
        }
    }

    template <typename HashKey, typename HashValue>
    void ExecuteWithArgsHash(std::vector<std::string> nameKeys, QHash<HashKey, HashValue> args, bool ignoreUniqueness = false){
        BindValues();
        for(const auto& key : args.keys())
        {
            q.bindValue(nameKeys[0], key);
            q.bindValue(nameKeys[1], args[key]);
            if(!ExecAndCheck(ignoreUniqueness))
            {
                qDebug() << "breaking out of cycle";
                break;
            }
        }
    }

    template<template <class> class  ContainerType, typename KeyType>
    void ExecuteWithKeyListAndBindFunctor(ContainerType<KeyType> keyList, std::function<void(KeyType&& key, sql::Query& q)>&& functor, bool ignoreUniqueness = false){
        BindValues();
        for(auto&& key : keyList)
        {
            functor(std::move(key), q);
            if(!ExecAndCheck(ignoreUniqueness))
                break;
        }
    }

    template<template <class> class  ContainerType, template <class> class  PointerType, typename KeyType>
    void ExecuteWithKeyListAndBindFunctor(ContainerType<PointerType<KeyType>> keyList, std::function<void(PointerType<KeyType> key, sql::Query& q)>&& functor, bool ignoreUniqueness = false){
        BindValues();
        for(auto&& key : keyList)
        {
            functor(std::move(key), q);
            if(!ExecAndCheck(ignoreUniqueness))
                break;
        }
    }

    template <template <class> class  ContainerType, typename ValueType>
    void ExecuteWithValueList(std::string keyName, ContainerType<ValueType>&& valueList, bool ignoreUniqueness = false){
        BindValues();
        for(auto&& value : valueList)
        {
            q.bindValue(keyName, std::move(value));
            if(!ExecAndCheck(ignoreUniqueness))
                break;
        }
    }
    template <template <class> class  ContainerType, typename ValueType>
    void ExecuteWithValueList(std::string keyName, const ContainerType<ValueType>& valueList, bool ignoreUniqueness = false){
        BindValues();
        for(auto value : valueList)
        {
            q.bindValue(keyName, value);
            if(!ExecAndCheck(ignoreUniqueness))
                break;
        }
    }


    bool ExecAndCheck(bool ignoreUniqueness = false){
        BindValues();
        return result.ExecAndCheck(q, ConvertBoolToError(ignoreUniqueness, {ESqlErrors::se_unique_row_violation}));
    }
    bool CheckDataAvailability(bool allowEmptyRecords = false){
        return result.CheckDataAvailability(q, allowEmptyRecords);
    }
    bool ExecAndCheckForData(){
        BindValues();
        result.ExecAndCheck(q);
        if(!result.success)
            return false;
        result.CheckDataAvailability(q, true);
        if(!result.success)
            return false;
        return true;
    }
    void FetchSelectFunctor(std::string&& select, std::function<void(ResultType& data, sql::Query& q)>&& f, bool allowEmptyRecords = false)
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
    void FetchLargeSelectIntoList(std::string&& actualQuery, std::function<ValueType(sql::Query&)>&& func, std::string&& countQuery = "")
    {
        int size = 0;
        if(!q.supportsImmediateResultSize()){
            if(countQuery.length() == 0)
                qs = "select count(*) from ( " + actualQuery + " ) as aliased_count ";
            else
                qs = countQuery;
            Prepare(qs);
            if(!ExecAndCheck())
                return;


            if(!CheckDataAvailability())
                return;
            size = q.value(0).toInt();
            if(size == 0)
                return;
        }

        qs = actualQuery;
        Prepare(qs);
        //BindValues();

        if(!ExecAndCheck())
            return;

        if(q.supportsImmediateResultSize())
            size = q.rowCount();
        result.data.reserve(size);

        if(!CheckDataAvailability(true))
            return;

        do{
                result.data += func(q);
        } while(q.next());
    }

    template <typename ValueType>
    void FetchLargeSelectIntoList(std::string&& fieldName, std::string&& actualQuery, std::string&& countQuery = "")
    {
        int size = 0;
        if(!q.supportsImmediateResultSize()){
            if(countQuery.length() == 0)
                qs = "select count(*) from ( " + actualQuery + " ) as aliased_count ";
            else
                qs = countQuery;
            Prepare(qs);
            if(!ExecAndCheck())
                return;


            if(!CheckDataAvailability(true))
                return;
            size = q.value(0).toInt();
            if(size == 0)
                return;
        }
        //qDebug () << "query size: " << size;



        qs = actualQuery;
        Prepare(qs);
        //BindValues();

        if(!ExecAndCheck())
            return;
        if(q.supportsImmediateResultSize())
            size = q.rowCount();
        if(!CheckDataAvailability(true))
            return;

        result.data.reserve(size);
        do{
           result.data += q.value(fieldName.c_str()).template value<typename ResultType::value_type>();
        } while(q.next());
    }

    template <typename ValueType>
    void FetchLargeSelectIntoListWithoutSize(std::string&& fieldName, std::string&& actualQuery,
                                  std::function<ValueType(sql::Query&)>&& func = std::function<ValueType(sql::Query&)>())
    {
        qs = actualQuery;
        Prepare(qs);

        if(!ExecAndCheck())
            return;
        if(!CheckDataAvailability(true))
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
        if(!CheckDataAvailability(true))
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
        if(!CheckDataAvailability(true))
        {
            if(!requireExisting)
                result.success = true;
            return;
        }
        result.data = q.value(valueName.c_str()).template value<T>();
    }

    void ExecuteList(std::vector<std::string>&& queries){
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
            q.bindValue(bind.key, bind.value);
        }
    }

    //template<typename KeyType, typename LamdaType>
    //void ProcessKeys(std::vector<KeyType> keys, const LamdaType& func){
    template<template <class> class  ContainerType, typename KeyType>
    void ProcessKeys(ContainerType<KeyType> keys, const std::function<void(const std::string& key, sql::Query&)>& func){
        for(auto key : keys)
            func(key, q);
    }
    template<template <class> class  ContainerType, typename KeyType>
    void ProcessKeys(ContainerType<KeyType> keys, const std::function<void(const QString& key, sql::Query&)>& func){
        for(auto key : keys)
            func(key, q);
    }

    void for_each(std::function<void(sql::Query&)> func){
        while(q.next())
            func(q);
    }

    DiagnosticSQLResult<ResultType> ForEachInSelect(const std::function<void(sql::Query&)>& func){
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

    Variant value(std::string name){return q.value(name);}
    std::string trimmedValue(std::string name){return trim_copy(q.value(name).toString());}

    void bindValue(std::string&& key, const Variant& value){
        auto it = std::find_if(bindValues.begin(), bindValues.end(), [key](const QueryBinding& b){
            return b.key == key;
        });
        if(it!=bindValues.cend())
            *it=QueryBinding{it->key, value};
        else
            bindValues.emplace_back(QueryBinding{key, value});
    }
    void bindValue(const std::string& key, const Variant& value){
        auto it = std::find_if(bindValues.begin(), bindValues.end(), [key](const QueryBinding& b){
            return b.key == key;
        });
        if(it!=bindValues.cend())
            *it = QueryBinding{it->key, value};
        else
            bindValues.emplace_back(QueryBinding{key, value});
    }

    void bindValue(const std::string& key, Variant&& value){
        auto it = std::find_if(bindValues.begin(), bindValues.end(), [key](const QueryBinding& b){
            return b.key == key;
        });
        if(it!=bindValues.cend())
            *it = QueryBinding{it->key, value};
        else
            bindValues.emplace_back(QueryBinding{key, value});
    }
    void bindValue(std::string&& key, Variant&& value){
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
    sql::Query q;
    Transaction transaction;
    std::vector<QueryBinding> bindValues;
    std::function<void(SqlContext<ResultType>*)> func;
};




template <typename ResultType>
struct ParallelSqlContext
{
    ParallelSqlContext(sql::Database source, std::string&& sourceQuery, std::vector<std::string>&& sourceFields,
                       sql::Database target, std::string&& targetQuery, std::vector<std::string>&& targetFields):
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

        Variant value;
        while(sourceQ.next())
        {
            for(int i = 0; i < sourceFields.size(); ++i )
            {
                //qDebug() << "binding field: " << sourceFields[i];
                auto it = valueConverters.find(sourceFields[i]);
                if(it != valueConverters.end())
                {
                    value = it->second(sourceFields.at(i), sourceQ, targetDB, result);
                    if(!result.success)
                        return result;
                }
                else
                {
                    value = sourceQ.value(sourceFields.at(i).c_str());
                    //qDebug() << "binding value: " << value;
                }
                //qDebug() << "to target field: " << targetFields[i];
                targetQ.bindValue((targetFields[i]).c_str(), value);
            }

            if(!result.ExecAndCheck(targetQ, ConvertBoolToError(ignoreUniqueness, {ESqlErrors::se_unique_row_violation})))
                return result;
        }
        return result;
    }
    bool Success() const {return result.success;}
    DiagnosticSQLResult<ResultType> result;
    sql::Query sourceQ;
    sql::Query targetQ;
    sql::Database sourceDB;
    sql::Database targetDB;
    std::vector<std::string> sourceFields;
    std::vector<std::string> targetFields;
    Transaction transaction;
    std::unordered_map<std::string,std::function<Variant(const std::string&, sql::Query, sql::Database, DiagnosticSQLResult<ResultType>&)>> valueConverters;
};

}

