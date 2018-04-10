#include "sqlcontext.h"
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
                qDebug() << "Error while performing a query: ";
            qDebug() << "Error while performing a query: ";
            qDebug().noquote() << q.lastQuery();
            qDebug() << "Error was: " <<  q.lastError();
            qDebug() << q.lastError().nativeErrorCode();
            qDebug() << q.lastError().driverText();
            qDebug() << q.lastError().databaseText();
        }
        return false;
    }
    return true;
}
}}
