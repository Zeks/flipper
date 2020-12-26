#include "sql_abstractions/sql_query_impl_sqlite.h"
#include "logger/QsLog.h"
#include <QSqlError>
#include <unordered_map>
namespace sql {

//std::unordered_map<std::string, std::string> namedQueries;

ESqlErrors SqliteErrorCodeToLocalErrorCode(int sqliteErrorCode){
    switch(sqliteErrorCode){
    case 19:
        return ESqlErrors::se_unique_row_violation;
    case 1:
        return ESqlErrors::se_duplicate_column;
    case 0:
        return ESqlErrors::se_none;
    default:
        QLOG_ERROR() << "Unexpected unknowm error code:" + QString::number(sqliteErrorCode).toStdString();
        return ESqlErrors::se_generic_sql_error;
    }


}

QueryImplSqlite::QueryImplSqlite()
{

}

QueryImplSqlite::QueryImplSqlite(QSqlDatabase db):q(db)
{

}

bool QueryImplSqlite::prepare(const std::string & sql)
{
    prepareErrorStorage = {};
    auto result = q.prepare(QString::fromStdString(sql));
    prepareErrorStorage = {q.lastError().text().toStdString(), SqliteErrorCodeToLocalErrorCode(q.lastError().nativeErrorCode().toInt())};
    return result;
}

bool QueryImplSqlite::prepare(const std::string & name, const std::string & query)
{
    // does the same thign as the above ignoring name
    Q_UNUSED(name);
    return prepare(query);
}

bool QueryImplSqlite::prepare(std::string && sql)
{
    prepareErrorStorage = {};
    auto result = q.prepare(QString::fromStdString(sql));
    prepareErrorStorage = {q.lastError().text().toStdString(), SqliteErrorCodeToLocalErrorCode(q.lastError().nativeErrorCode().toInt())};
    return result;
}

bool QueryImplSqlite::exec()
{
    auto result = q.exec();
//    if(namedQueries.find(queryName) == std::end(namedQueries))
//        namedQueries[queryName] = q.lastQuery().toStdString();
    return result;
}

void QueryImplSqlite::setForwardOnly(bool value)
{
    q.setForwardOnly(value);
}

void QueryImplSqlite::bindVector(const std::vector<QueryBinding>& bindList)
{
    for(const auto& bind: bindList)
    {
        //qDebug() << QString::fromStdString(bind.key) << " " << bind.value.toQVariant();
        q.bindValue(":" + QString::fromStdString(bind.key), bind.value.toQVariant());
    }
}

void QueryImplSqlite::bindVector(std::vector<QueryBinding> && bindList)
{
    for(auto&& bind: bindList)
    {
        //qDebug() << QString::fromStdString(bind.key) << " " << bind.value.toQVariant();
        q.bindValue(":" + QString::fromStdString(bind.key), bind.value.toQVariant());
    }
}

void QueryImplSqlite::bindValue(const std::string & key, const Variant & value)
{
    //qDebug() << QString::fromStdString(key) << " " << value.toQVariant();
    q.bindValue(":" + QString::fromStdString(key), value.toQVariant());
}

void QueryImplSqlite::bindValue(const std::string & key, Variant && value)
{
    //qDebug() << QString::fromStdString(key) << " " << value.toQVariant();
    q.bindValue(":" + QString::fromStdString(key), value.toQVariant());
}

void QueryImplSqlite::bindValue(std::string && key, const Variant & value)
{
    //qDebug() << QString::fromStdString(key) << " " << value.toQVariant();
    q.bindValue(":" + QString::fromStdString(key), value.toQVariant());
}

void QueryImplSqlite::bindValue(std::string && key, Variant && value)
{
    //qDebug() << QString::fromStdString(key) << " " << value.toQVariant();
    q.bindValue(":" + QString::fromStdString(key), value.toQVariant());
}

void QueryImplSqlite::bindValue(const QueryBinding & bind)
{
    //qDebug() << QString::fromStdString(bind.key) << " " << bind.value.toQVariant();
    q.bindValue(":" + QString::fromStdString(bind.key), bind.value.toQVariant());
}

void QueryImplSqlite::bindValue(QueryBinding && bind)
{
    //qDebug() << QString::fromStdString(bind.key) << " " << bind.value.toQVariant();
    q.bindValue(":" + QString::fromStdString(bind.key), bind.value.toQVariant());
}

bool QueryImplSqlite::next()
{
    return q.next();
}

constexpr bool QueryImplSqlite::supportsVectorizedBind() const
{
    return true;
}

Variant QueryImplSqlite::value(int fieldPosition) const
{
    return Variant(q.value(fieldPosition));
}

Variant QueryImplSqlite::value(const std::string & fieldName) const
{
    return Variant(q.value(QString::fromStdString(fieldName)));
}

Variant QueryImplSqlite::value(std::string && fieldName) const
{
    return Variant(q.value(QString::fromStdString(fieldName)));
}

Variant QueryImplSqlite::value(const char * fieldName) const
{
    return Variant(q.value(fieldName));
}

//QSqlRecord QueryImplSqlite::record()
//{
//    return q.record();
//}

Error QueryImplSqlite::lastError() const
{
    if(prepareErrorStorage.isValid()){
        if(prepareErrorStorage.text().find("duplicate column") != std::string::npos)
        {
            QLOG_WARN() << prepareErrorStorage.text();
            return {};
        }
        return prepareErrorStorage;
    }
//    if(q.lastError().isValid()){
//        QLOG_ERROR() << q.lastError().nativeErrorCode();
//        QLOG_ERROR() << q.lastError().driverText();
//        QLOG_ERROR() << q.lastError().databaseText();
//    }
    return Error(q.lastError().text().toStdString(), SqliteErrorCodeToLocalErrorCode(q.lastError().nativeErrorCode().toInt()));
}

std::string QueryImplSqlite::lastQuery() const
{
    return q.lastQuery().toStdString();
}

std::string QueryImplSqlite::implType() const
{
    return "QSQLITE";
}

}
