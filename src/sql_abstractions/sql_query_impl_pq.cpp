#include "sql_abstractions/sql_query_impl_pq.h"
#include "sql_abstractions/sql_database_impl_pq.h"
#include "sql_abstractions/sql_error.h"
#include <pqxx/pqxx>
#include "fmt/format.h"
#include "logger/QsLog.h"

#include "third_party/ctre.hpp"
#include <QUuid>
#include <variant>
#include <stdexcept>
#include <charconv>

namespace sql {
struct QueryImplPqImpl{
    std::shared_ptr<DatabaseImplPq> database;
    std::string preparedStatementId;
    bool namedStatement;
    std::string Statement;
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
    d->Statement = query;
}

QueryImplPq::QueryImplPq(std::string&& query, std::shared_ptr<DatabaseImplPq> impl): QueryImplPq(impl)
{
    d->Statement = query;
}





bool QueryImplPq::NeedsPreparing(const std::string& statement){
    if(statement.find("$1") == std::string::npos)
    {
        // we're not supposed to prepare this
        d->Statement = statement;
        return false;
    }
    return true;
}

void QueryImplPq::ExtractNamedPlaceholders(std::string_view view)
{
    static constexpr auto pattern = ctll::fixed_string{ "(^|[^:])(:[A-Za-z_][A-Za-z_0-9]+)" };
    using namespace ctre::literals;

    for(auto match : ctre::range<pattern>(view)){
        d->foundNamedPlaceholders.emplace_back(match.get<2>().to_string());
    }
}

std::string QueryImplPq::ReplaceNamedPlaceholders(std::string statement)
{
    auto counter = 1;
    for(const auto& placeholder: d->foundNamedPlaceholders){
        auto pos = statement.find(placeholder);
        if(pos != std::string::npos){
            statement.replace(pos, placeholder.length(), "$" + std::to_string(counter));
        }
        counter++;
    }
    return statement;
}

bool QueryImplPq::prepare(const std::string & statement, const std::string & name)
{
    ExtractNamedPlaceholders(statement);
    if(d->foundNamedPlaceholders.size() > 0){
        d->Statement = ReplaceNamedPlaceholders(statement);
    }
    if(!NeedsPreparing(d->Statement))
        return true;
    try {
        if(name.empty())
            d->uniqueQueryIdentifier = QUuid::createUuid().toString().toStdString();
        else
            d->uniqueQueryIdentifier = name;
        d->database->getConnection()->prepare(*d->uniqueQueryIdentifier, d->Statement);
    }  catch (const pqxx::failure& e) {
        d->lastError = Error(e.what(), ESqlErrors::se_generic_sql_error);
        QLOG_ERROR() << e.what();
        return false;
    }
    return true;
}


std::string VariantToString(const Variant& wrapper){
    std::string result;
    auto& v = wrapper.data;
    auto indexOfValue = v.index();
    switch(indexOfValue){
    case 0:
        result = "";
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
        result =  std::to_string(std::get<int>(v));
        break;
    case 8:
        result =  std::get<QDateTime>(v).toString("yyyy-MM-dd").toStdString();
        break;
    case 9:
        result =  std::get<QByteArray>(v).data(); // todo shitcode, verify
        break;
    default:
        throw std::logic_error("all indexes should be handled in pg driver: " + std::to_string(indexOfValue));
    }
    qDebug() << "Returning converted variant: " << result;
    return result;
}


bool QueryImplPq::exec()
{

    if(!d->database->transaction())
        return false;

    auto transaction = d->database->getTransaction();

    if(!d->uniqueQueryIdentifier.has_value()){
        try{
            d->resultSet = transaction->exec(d->Statement);
        }
        catch (const pqxx::failure& e) {
            d->lastError = Error(e.what(), ESqlErrors::se_generic_sql_error);
            QLOG_ERROR() << e.what();
            return false;
        }
    }
    else{

        auto fetchResultSet = [&](auto bindingsVector){
            auto dynamicParams = pqxx::prepare::make_dynamic_params(bindingsVector, [](const auto& v){
                if constexpr(std::is_same<typename decltype(bindingsVector)::value_type, std::reference_wrapper<QueryBinding>>::value)
                    return VariantToString(v.get().value);
                else
                    return VariantToString(v.value);
            });
            d->resultSet = transaction->exec_prepared(*d->uniqueQueryIdentifier, dynamicParams);
        };
        try{
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
                fetchResultSet(d->bindings);
            }
            d->database->getConnection()->unprepare(*d->uniqueQueryIdentifier);
        }
        catch (const pqxx::failure& e) {
            d->lastError = Error(e.what(), ESqlErrors::se_generic_sql_error);
            QLOG_ERROR() << e.what();
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

bool QueryImplPq::next()
{
    if(d->resultSet.size() == 0)
    {
        QLOG_WARN() << "Requesting next value of empty resultset";
        return false;
    }
    if(!d->current_result_iterator.has_value())
        d->current_result_iterator = d->resultSet.begin();
    else{
        std::advance((*d->current_result_iterator),1);
        if(*d->current_result_iterator == d->resultSet.end())
        {
            QLOG_WARN() << "Requesting next value past the end of the resultset";
            return false;
        }
    }
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
        case 23:
            int64_t result;
            if(auto [p, ec] = std::from_chars(field.view().begin(), field.view().end(), result); ec == std::errc())
                v = Variant(result);
            return Variant();
            break; // int4 aka bigint
        case 1043:
            QLOG_INFO() << "Date is returned as: " << field.c_str();
            v = Variant();
            break; // timestamp, test format: 1999-01-08
        case 25:
        case 1114:
            v = Variant(field.c_str());
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
    auto row = *d->current_result_iterator;
    return FieldToVariant(row.at(name));
}

Variant QueryImplPq::value(std::string && name) const
{
    auto row = *d->current_result_iterator;
    return FieldToVariant(row.at(name));
}

Variant QueryImplPq::value(const char * name) const
{
    auto row = *d->current_result_iterator;
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
    return d->Statement;
}

std::string QueryImplPq::implType() const
{
    return "PQXX";
}

}
