#include "sql_abstractions/sql_context.h"
#include "sql_abstractions/sql_query.h"
#include "logger/QsLog.h"
namespace sql{
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
            QLOG_ERROR_PURE()<< q.lastQuery();
            QLOG_ERROR() << "Error was: " <<  q.lastError().text();
//            QLOG_ERROR() << q.lastError().nativeErrorCode();
//            QLOG_ERROR() << q.lastError().driverText();
//            QLOG_ERROR() << q.lastError().databaseText();
        }
        return false;
    }
    return true;
}

}
