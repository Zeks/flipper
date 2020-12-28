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
#include "sql_abstractions/sql_query_impl_pq.h"
#include "sql_abstractions/sql_database_impl_pq.h"
#include "sql_abstractions/sql_error.h"
#include <pqxx/pqxx>
#include "fmt/format.h"
#include "logger/QsLog.h"
#include "GlobalHeaders/scope_guard.hpp"

#include "third_party/ctre.hpp"
#include <QUuid>
#include <variant>
#include <stdexcept>
#include <charconv>

namespace sql {

struct Statement{
    std::string originalStatement;
    std::string editedStatement;
};

struct QueryImplPqImpl{
    std::shared_ptr<DatabaseImplPq> database;
    Statement statement;
    std::optional<std::string> uniqueQueryIdentifier;
    std::vector<QueryBinding> bindings;
    std::vector<std::string> foundNamedPlaceholders;
    pqxx::result resultSet;
    std::optional<pqxx::result::const_iterator> current_result_iterator;
    Error lastError;
};
QueryImplPq::QueryImplPq() : d(new QueryImplPqImpl())
{

}

QueryImplPq::QueryImplPq(std::shared_ptr<DatabaseImplPq> db) : QueryImplPq()
{
    d->database = db;
}

QueryImplPq::QueryImplPq(const std::string& query, std::shared_ptr<DatabaseImplPq> impl): QueryImplPq(impl)
{
    d->statement = {query,query};
}

QueryImplPq::QueryImplPq(std::string&& query, std::shared_ptr<DatabaseImplPq> impl): QueryImplPq(impl)
{
    d->statement = {query,query};
}





bool QueryImplPq::NeedsPreparing(const std::string& statement){
    if(statement.find("$1") != std::string::npos)
        return true;
    return false;
}

void QueryImplPq::ExtractNamedPlaceholders(std::string_view view)
{
    static constexpr auto pattern = ctll::fixed_string{ "(^|[^:])(:[A-Za-z_][A-Za-z_0-9]+)" };
    using namespace ctre::literals;

    for(auto match : ctre::range<pattern>(view)){
        d->foundNamedPlaceholders.emplace_back(match.get<2>().to_string());
    }
}

void QueryImplPq::ReplaceNamedPlaceholders(std::string& statement)
{
    auto counter = 1;
    for(const auto& placeholder: d->foundNamedPlaceholders){
        auto pos = statement.find(placeholder);
        if(pos != std::string::npos){
            statement.replace(pos, placeholder.length(), "$" + std::to_string(counter));
        }
        counter++;
    }
}

void QueryImplPq::ResetLocalData()
{
    d->statement = {};
    d->bindings = {};
    d->resultSet = {};
    d->lastError = {};
    d->uniqueQueryIdentifier = {};
    d->foundNamedPlaceholders = {};
    d->current_result_iterator = {};
}

bool QueryImplPq::prepare(const std::string & statement, const std::string & name)
{
    ResetLocalData();
    d->statement = {statement,statement};
    ExtractNamedPlaceholders(d->statement.originalStatement);
    // if there are named placeholders this query by deafult needs preparing
    if(d->foundNamedPlaceholders.size() > 0){
        ReplaceNamedPlaceholders(d->statement.editedStatement);
    }
    else if(!NeedsPreparing(d->statement.editedStatement))
        return true;
    try {
        if(name.empty())
            d->uniqueQueryIdentifier = QUuid::createUuid().toString().toStdString();
        else
            d->uniqueQueryIdentifier = name;
        d->database->getConnection()->prepare(*d->uniqueQueryIdentifier, d->statement.editedStatement);
    }  catch (const pqxx::failure& e) {
        d->lastError = Error(e.what(), ESqlErrors::se_generic_sql_error);
        QLOG_ERROR() << e.what();
        return false;
    }
    return true;
}


std::optional<std::string> VariantToOptionalString(const Variant& wrapper){
    std::optional<std::string> result;
    try{
        auto& v = wrapper.data;
        int indexOfValue = v.index();
        switch(indexOfValue){
        case 0:
            //monostate = null, leaving empty optional as is
            break;
        case 1:
            result = std::get<std::string>(v);
            break;
        case 2:
            result = std::to_string(std::get<int>(v));
            break;
        case 3:
            result = std::to_string(std::get<uint>(v));
            break;
        case 4:
            result = std::to_string(std::get<int64_t>(v));
            break;
        case 5:
            result = std::to_string(std::get<uint64_t>(v));
            break;
        case 6:
            result = std::to_string(std::get<double>(v));
            break;
        case 7:
            result =  std::get<QDateTime>(v).toString("yyyy-MM-dd").toStdString();
            break;
        case 8:
            result =  std::to_string(std::get<bool>(v));
            break;
        case 9:
            result =  std::get<QByteArray>(v).data(); // todo shitcode, verify
            break;
        default:
            throw std::logic_error("all indexes should be handled in pg driver: " + std::to_string(indexOfValue));
        }
        qDebug() << "Passing converted variant to sql query: " << result.value_or("");
    }
    catch (const std::bad_variant_access& error){
        QLOG_INFO() << "error:" << error.what();
        throw;
    }
    return result;
}


bool QueryImplPq::exec()
{

    auto transaction = d->database->getTransaction();
    bool ownTransaction = transaction.get() == nullptr;


    if(!d->database->transaction())
        return false;

    transaction = d->database->getTransaction();
    auto guard = sg::make_scope_guard([ownTransaction, transaction=std::ref(transaction), this](){
        if(ownTransaction && transaction.get())
            d->database->commit();
    });

    auto onSqlError = [&](const pqxx::failure& e){
        d->lastError = Error(e.what(), ESqlErrors::se_generic_sql_error);
        d->database->rollback();
        transaction = nullptr;
        ownTransaction = false;
        QLOG_ERROR() << e.what();
    };

    if(!d->uniqueQueryIdentifier.has_value()){
        try{
            d->resultSet = transaction->exec(d->statement.editedStatement);
        }
        catch (const pqxx::failure& e) {
            onSqlError(e);
            return false;
        }
    }
    else{

        try{
            auto fetchResultSet = [&](auto bindingsVector){
                auto dynamicParams = pqxx::prepare::make_dynamic_params(bindingsVector, [](const auto& v){
                    if constexpr(std::is_same<typename decltype(bindingsVector)::value_type, std::reference_wrapper<QueryBinding>>::value)
                            return VariantToOptionalString(v.get().value);
                    else
                    return VariantToOptionalString(v.value);
                });
                d->resultSet = transaction->exec_prepared(*d->uniqueQueryIdentifier, dynamicParams);
            };

            // todo, this is temporary and needs to go once I am sure that all of the queries work
            for(const auto& bind: d->bindings)
            {
                auto it = std::find_if(std::begin(d->foundNamedPlaceholders), std::end(d->foundNamedPlaceholders), [bind](const auto& value){
                    return value == ":" + bind.key;
                });
                if(it == d->foundNamedPlaceholders.end()){
                    std::string hasPlaceholders;
                    std::for_each(std::begin(d->foundNamedPlaceholders), std::end(d->foundNamedPlaceholders),[&](auto value){
                        hasPlaceholders+=value + " ";
                    });
                    throw std::logic_error("trying to use the placeholder not in the query: " + bind.key + " query has: " + hasPlaceholders);
                }
            }

            // making sure named placeholders can be reused in more than one place
            // requires creating a fictional reference wrapped vector at the place of execution
            // in which the same value can be used more than once
            // in this code foundNamedPlaceholders is the actual repeating placeholders in the query
            // and bindings are what the user has bound
            std::vector<std::reference_wrapper<QueryBinding>> adjustedBindings;
            if(d->foundNamedPlaceholders.size() > 0){
                for(const auto& placeholder: d->foundNamedPlaceholders){
                    auto it = std::find_if(d->bindings.begin(),d->bindings.end(),[placeholder](const auto& binding){
                        return ":" + binding.key == placeholder;
                    });
                    if(it != d->bindings.end())
                        adjustedBindings.push_back(std::ref(*it));
                    else
                        throw std::logic_error("trying to execute a query with bound param not in bound vector");
                }
                fetchResultSet(adjustedBindings);
            }
            else{
                // a case where positional bindings are used as is
                // it should't trigger in current code, but still...
                fetchResultSet(d->bindings);
            }
            d->database->getConnection()->unprepare(*d->uniqueQueryIdentifier);
        }
        catch (const pqxx::failure& e) {
            try{
                d->database->getConnection()->unprepare(*d->uniqueQueryIdentifier);
            }
            catch(...){} // not intersted in the chain of errors after exception has already been thrown
            onSqlError(e);
            return false;
        }
    }
    return true;
}

void QueryImplPq::setForwardOnly(bool )
{
    // not supported in postgres, does nothing
}

void QueryImplPq::bindVector(const std::vector<QueryBinding> & vec)
{
    d->bindings = vec;
}

void QueryImplPq::bindVector(std::vector<QueryBinding> && vec)
{
    d->bindings = vec;
}

void QueryImplPq::bindValue(const std::string & name, const Variant & value)
{
    d->bindings.emplace_back(QueryBinding{name, value});
}

void QueryImplPq::bindValue(const std::string & name, Variant && value)
{
    d->bindings.emplace_back(QueryBinding{name, value});
}

void QueryImplPq::bindValue(std::string && name, const Variant & value)
{
    d->bindings.emplace_back(QueryBinding{name, value});
}

void QueryImplPq::bindValue(std::string && name, Variant && value)
{
    d->bindings.emplace_back(QueryBinding{name, value});
}

void QueryImplPq::bindValue(const QueryBinding & value)
{
    d->bindings.push_back(value);
}

void QueryImplPq::bindValue(QueryBinding && value)
{
    d->bindings.emplace_back(value);
}

bool QueryImplPq::next(bool warnOnEmpty)
{
    if(warnOnEmpty && !d->current_result_iterator.has_value() && d->resultSet.size() == 0){
        QLOG_WARN() << "Requesting first value of empty resultset";
    }
    if(d->resultSet.size() == 0)
        return false;
    if(!d->current_result_iterator.has_value())
        d->current_result_iterator = d->resultSet.begin();
    else{
        std::advance((*d->current_result_iterator),1);
        if(*d->current_result_iterator == d->resultSet.end())
        {
            //QLOG_WARN() << "Requesting next value past the end of the resultset";
            return false;
        }
    }
    return true;
}

int QueryImplPq::rowCount() const
{
    return d->resultSet.size();
}

bool QueryImplPq::supportsImmediateResultSize() const
{
    return true;
}

bool QueryImplPq::supportsVectorizedBind() const
{
    return true;
}

Variant FieldToVariant(const pqxx::row::reference& field){
    Variant v;
    try{
        switch(field.type()){
        case 20:
        case 23:
            int64_t result;
            if(auto [p, ec] = std::from_chars(field.view().begin(), field.view().end(), result); ec == std::errc())
                v = result;
            break; // int4 aka bigint
        case 1043:
            QLOG_INFO() << "Date is returned as: " << field.c_str();
            v = field.c_str();
            break; // timestamp, test format: 1999-01-08
        case 25:
        case 1114:
            v = field.c_str();
            break; // varchar
        default:
            throw std::logic_error("Received type that isn't available for processing:" + std::to_string(field.type()));
            break;
        }
    }
    catch(const std::invalid_argument& e){
        QLOG_ERROR() << "could not convert string to number, invalid argument: " + std::string(field.view());
    }
    catch(const std::out_of_range& e){
        QLOG_ERROR() << "could not convert string to number, out of range: " + std::string(field.view());
    }
    //qDebug() << "Fetched variant with value: " << v;
    return v;
}

Variant QueryImplPq::value(int index) const
{
    if(!d->current_result_iterator.has_value())
        throw std::logic_error("cannot fetch data from invalid iterator");
    auto row = d->current_result_iterator.operator->();
    auto field = row->at(index);
    return FieldToVariant(field);
}

Variant QueryImplPq::value(const std::string & name) const
{
    const auto& row = *d->current_result_iterator;
    //qDebug() << "fetching field: " << name;
    return FieldToVariant(row.at(name));
}

Variant QueryImplPq::value(std::string && name) const
{
    const auto& row = *d->current_result_iterator;
    //qDebug() << "fetching field: " << name;
    return FieldToVariant(row.at(name));
}

Variant QueryImplPq::value(const char * name) const
{
    const auto& row = *d->current_result_iterator;
    //qDebug() << "fetching field: " << name;
    return FieldToVariant(row.at(name));
}

//QSqlRecord QueryImplPq::record()
//{
//    throw std::logic_error("use of nulld sql driver");
//    return {};
//}

Error QueryImplPq::lastError() const
{
    return d->lastError;
}

std::string QueryImplPq::lastQuery() const
{
    return d->statement.originalStatement;
}

std::string QueryImplPq::implType() const
{
    return "PQXX";
}

}
