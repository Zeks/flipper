#include "include/db_synch.h"
#include "include/sqlcontext.h"

#include "logger/QsLog.h"


#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QSqlDatabase>

namespace database{
    bool ExecuteCommandsFromDbScriptFile(QString file, QSqlDatabase db, bool usePrepare){
    QFile data(file);

    QLOG_INFO() << "Reading init file: " << file;
    if (data.open(QFile::ReadOnly))
    {
        QTextStream in(&data);
        QString dbCode = in.readAll();
        data.close();
        QStringList statements = dbCode.split(";");
        //auto transactionResult = db.transaction();
        qDebug() << "Error before query:" << db.lastError().text();
        QSqlQuery q(db);
        for(QString statement: statements)
        {
            statement = statement.replace(QRegExp("\\t"), " ");
            statement = statement.replace(QRegExp("\\r"), " ");
            statement = statement.replace(QRegExp("\\n"), " ");

            if(statement.trimmed().isEmpty() || statement.trimmed().left(2) == "--")
                continue;
            bool result = true;
            if(usePrepare)
            {
                auto prepareResult = q.prepare(statement.trimmed());
                QLOG_TRACE() << "Executing: " << statement;
                result = database::puresql::ExecAndCheck(q, true);
            }
            else{
                QLOG_TRACE() << "Executing: " << statement;
                result = database::puresql::ExecAndCheck(q, statement, true);
            }
            if(!result)
            {
                //db.rollback();
                return false;
            }
        }
        //db.commit();
    }
    else
        return false;
    return true;
    }
    
}
