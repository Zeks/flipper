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
#include "sql_abstractions/sql_context.h"
#include "sql_abstractions/sql_query.h"
#include "sql_abstractions/sql_error.h"
#include "logger/QsLog.h"
namespace sql{
bool ExecAndCheck(sql::Query& q, bool reportErrors, std::vector<ESqlErrors> expectedErrors)
{
    bool success = q.exec();

    Q_UNUSED(success)

    if(q.lastError().isValid())
    {
        auto error = q.lastError().getActualErrorType();
        auto it = std::find(expectedErrors.begin(), expectedErrors.end(), error);
        if(it != std::end(expectedErrors))
        {
            QLOG_WARN() << q.lastError().text();
            return true;
        }
        if(reportErrors)
        {
            if(q.lastError().text().find("record") != std::string::npos)
                QLOG_ERROR() << "Error while performing a query: ";
            QLOG_ERROR() << " ";
            QLOG_ERROR() << " ";
            QLOG_ERROR() << "Error while performing a query: ";
            QLOG_ERROR_PURE()<< q.lastQuery();
            QLOG_ERROR() << "Error was: " <<  q.lastError().text();
        }
        return false;
    }
    return true;
}

}

