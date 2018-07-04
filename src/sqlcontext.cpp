#include "sqlcontext.h"
#include "logger/QsLog.h"
namespace database {
namespace puresql{
bool ExecAndCheck(QSqlQuery& q, bool reportErrors)
{
    bool success = q.exec();
    if(q.lastError().isValid())
    {
        if(reportErrors)
        {
            if(q.lastError().text().contains("record"))
                QLOG_ERROR() << "Error while performing a query: ";
            QLOG_ERROR() << " ";
            QLOG_ERROR() << " ";
            QLOG_ERROR() << "Error while performing a query: ";
            QLOG_ERROR_PURE()<< q.lastQuery();
            QLOG_ERROR() << "Error was: " <<  q.lastError();
            QLOG_ERROR() << q.lastError().nativeErrorCode();
            QLOG_ERROR() << q.lastError().driverText();
            QLOG_ERROR() << q.lastError().databaseText();
        }
        return false;
    }
    return true;
}
}}
