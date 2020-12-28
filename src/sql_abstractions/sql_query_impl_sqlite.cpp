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

bool QueryImplSqlite::prepare(const std::string & query, const std::string & name)
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
    return q.exec();
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

bool QueryImplSqlite::next(bool warnOnEmpty)
{
    Q_UNUSED(warnOnEmpty);
    return q.next();
}

int QueryImplSqlite::rowCount() const
{
    return 0;
}

bool QueryImplSqlite::supportsImmediateResultSize() const
{
    return false;
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
    // with wrapped qt driver error can actually happen in prepare statement
    // for this I keep the error that happened there and return it here
    if(prepareErrorStorage.isValid()){
        if(prepareErrorStorage.text().find("duplicate column") != std::string::npos)
        {
            QLOG_WARN() << prepareErrorStorage.text();
            return {};
        }
        return prepareErrorStorage;
    }
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
