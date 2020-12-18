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

