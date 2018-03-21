#pragma once
#include <functional>
#include <QString>
#include <memory>
#include <array>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QSharedPointer>
#include "transaction.h"


namespace database {
namespace puresql{
bool ExecAndCheck(QSqlQuery& q);

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
        else
            transaction.finalize();
    }

    DiagnosticSQLResult<ResultType> operator()(bool ignoreUniqueness = false){
        BindValues();
        ExecAndCheck(ignoreUniqueness);
        return result;
    }

    void ReplaceQuery(QString query){
        q.prepare(query);
        bindValues.clear();
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
    template <typename KeyType>
    void ExecuteWithKeyListAndBindFunctor(QList<KeyType> keyList, std::function<void(KeyType key, QSqlQuery& q)> functor, bool ignoreUniqueness = false){
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


    bool ExecAndCheck(bool ignoreUniqueness = false){
        BindValues();
        return result.ExecAndCheck(q, ignoreUniqueness);
    }
    bool CheckDataAvailability(){
        return result.CheckDataAvailability(q);
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

    template <typename T>
    void FetchSingleValue(QString valueName, ResultType defaultValue, QString select = ""){
        result.data = defaultValue;
        if(!select.isEmpty())
        {
            qs = select;
            q.prepare(qs);
            BindValues();
        }
        if(!ExecAndCheck())
            return;
        if(!CheckDataAvailability())
            return;
        result.data = q.value(valueName).template value<T>();
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

    void BindValues(){
        for(auto bind : bindValues.keys())
            q.bindValue(bind, bindValues[bind]);
    }

    template<typename KeyType>
    void ProcessKeys(QList<KeyType> keys, std::function<void(QString key, QSqlQuery&)> func){
        for(auto key : keys)
            func(key, q);
    }

    void for_each(std::function<void(QSqlQuery&)> func){
        while(q.next())
            func(q);
    }

    DiagnosticSQLResult<ResultType> ForEachInSelect(std::function<void(QSqlQuery&)> func){
        if(!ExecAndCheck())
            return result;
        for_each(func);
        return result;
    }

    QVariant value(QString name){return q.value(name);}
    QString trimmedValue(QString name){return q.value(name).toString().trimmed();}
    void bindValue(QString key, QVariant value){
        bindValues[":" + key] = value;
    }
    void SetDefaultValue(ResultType value) {result.data = value;}
    DiagnosticSQLResult<ResultType> result;
    QString qs;
    QSqlQuery q;
    Transaction transaction;
    QVariantHash bindValues;
    std::function<void(SqlContext<ResultType>*)> func;
};




template <typename ResultType>
struct ParallelSqlContext
{
    ParallelSqlContext(QSqlDatabase source, QString sourceQuery, QStringList sourceFields,
                       QSqlDatabase target, QString targetQuery, QStringList targetFields):sourceQ(source), targetQ(target),
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
        while(sourceQ.next())
        {
            for(int i = 0; i < sourceFields.size(); ++i )
            {
                QVariant value;
                if(valueConverters.contains(sourceFields[i]))
                {
                    value = valueConverters[sourceFields[i]](sourceQ, targetDB, result);
                    if(!result.success)
                        return result;
                }
                else
                    value = sourceQ.value(sourceFields[i]);
                targetQ.bindValue(":" + targetFields[i], value);
            }

            if(!result.ExecAndCheck(targetQ, ignoreUniqueness))
                return result;
        }
        return result;
    }

    DiagnosticSQLResult<ResultType> result;
    QSqlQuery sourceQ;
    QSqlQuery targetQ;
    QSqlDatabase sourceDB;
    QSqlDatabase targetDB;
    QStringList sourceFields;
    QStringList targetFields;
    Transaction transaction;
    QHash<QString,std::function<QVariant(QString, QSqlQuery, QSqlDatabase, DiagnosticSQLResult<ResultType>&)>> valueConverters;
};

}
}
