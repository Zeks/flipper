/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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
#include "sql_abstractions/sql_context.h"
#include "logger/QsLog.h"
namespace database {
namespace puresql{
bool ExecAndCheck(sql::Query& q, bool reportErrors ,  bool ignoreUniqueness )
{
    bool success = q.exec();

Q_UNUSED(success)

    if(q.lastError().isValid())
    {
        if(reportErrors && !(ignoreUniqueness && q.lastError().text().find("UNIQUE constraint failed") != std::string::npos))
        {
            if(q.lastError().text().find("record") != std::string::npos)
                QLOG_ERROR() << "Error while performing a query: ";
            QLOG_ERROR() << " ";
            QLOG_ERROR() << " ";
            QLOG_ERROR() << "Error while performing a query: ";
            QLOG_ERROR_PURE() << QString::fromStdString(q.lastQuery());
            QLOG_ERROR() << "Error was: " <<  QString::fromStdString(q.lastError().text());
//            QLOG_ERROR() << q.lastError().nativeErrorCode();
//            QLOG_ERROR() << q.lastError().driverText();
//            QLOG_ERROR() << q.lastError().databaseText();
        }
        return false;
    }
    return true;
}
}}
