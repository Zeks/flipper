#include "sqlcontext.h"
namespace database {
namespace puresql{
bool ExecAndCheck(QSqlQuery& q)
{
    q.exec();
    if(q.lastError().isValid())
    {
        if(q.lastError().text().contains("record"))
            qDebug() << "Error while performing a query: ";
        qDebug() << "Error while performing a query: ";
        qDebug().noquote() << q.lastQuery();
        qDebug() << "Error was: " <<  q.lastError();
        qDebug() << q.lastError().nativeErrorCode();
        return false;
    }
    return true;
}
}}