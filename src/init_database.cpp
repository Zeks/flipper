#include "include/init_database.h"
#include <quazip/quazip.h>
#include <quazip/JlCompress.h>

#include <QFile>
#include <QTextStream>
#include <QSqlQuery>
#include <QString>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>

namespace database{
bool database::ReadDbFile()
{
    QFile data("dbcode/dbinit.sql");
     if (data.open(QFile::ReadOnly))
     {
         QTextStream in(&data);
         QString db = in.readAll();
         QStringList statements = db.split(";");
         for(QString statement: statements)
         {
             if(statement.trimmed().isEmpty() || statement.trimmed().left(2) == "--")
                 continue;

             QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
             QSqlQuery q(db);
             q.prepare(statement.trimmed());
             q.exec();
             if(q.lastError().isValid())
             {
                 qDebug() << q.lastError();
                 qDebug() << q.lastQuery();
             }
         }
     }
     else return false;
     return true;
}

bool database::ReindexTable(QString table)
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QSqlQuery q(db);
    QString qs = "alter table %1 add column id integer";
    q.prepare(qs.arg(table));
    q.exec();


    QSqlQuery q1(db);
    QString qs1 = " UPDATE %1 SET id = rowid where id is null ";
    q1.prepare(qs1.arg(table));
    q1.exec();
    return true;
}

void database::SetFandomTracked(QString section, QString fandom, bool crossover, bool tracked)
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QString trackedPart = crossover ? "_crossovers" : "";
    QSqlQuery q1(db);
    QString qsl = " UPDATE fandoms SET tracked%1 = %2 where fandom = '%3' and section = '%4' ";
    qsl = qsl.arg(trackedPart).arg(QString(tracked ? "1" : "0")).arg(fandom).arg(section);
    q1.prepare(qsl);
    q1.exec();
    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
}

void database::PushFandom(QString fandom)
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QString upsert1 ("UPDATE recent_fandoms SET seq_num= (select max(seq_num) +1 from  recent_fandoms ) WHERE fandom = '%1'; ");
    QString upsert2 ("INSERT INTO recent_fandoms(fandom, seq_num) select '%1',  (select max(seq_num)+1 from recent_fandoms) WHERE changes() = 0;");
    QSqlQuery q1(db);
    upsert1 = upsert1.arg(fandom);
    q1.prepare(upsert1);
    q1.exec();
    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    QSqlQuery q2(db);
    upsert2 = upsert2.arg(fandom);
    q2.prepare(upsert2);
    q2.exec();
    if(q2.lastError().isValid())
    {
        qDebug() << q2.lastError();
        qDebug() << q2.lastQuery();
    }
}

void database::RebaseFandoms()
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QSqlQuery q1(db);
    QString qsl = "UPDATE recent_fandoms SET seq_num = seq_num - (select min(seq_num) from recent_fandoms where fandom is not 'base') where fandom is not 'base'";
    q1.prepare(qsl);
    q1.exec();
    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
}

QStringList database::FetchRecentFandoms()
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QSqlQuery q1(db);
    QString qsl = "select fandom from recent_fandoms where fandom is not 'base' order by seq_num desc";
    q1.prepare(qsl);
    q1.exec();
    QStringList result;
    while(q1.next())
    {
        result.push_back(q1.value(0).toString());
    }
    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    return result;
}

bool database::FetchTrackStateForFandom(QString section, QString fandom, bool crossover)
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QString trackedPart = crossover ? "_crossovers" : "";
    QSqlQuery q1(db);
    QString qsl = " select tracked%1 from fandoms where fandom = '%2' and section = '%3'";
    qsl = qsl.arg(trackedPart).arg(fandom).arg(section);
    q1.prepare(qsl);
    q1.exec();
    q1.next();

    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    return q1.value(0).toBool();
}

QStringList database::FetchTrackedFandoms()
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QSqlQuery q1(db);
    QString qsl = " select fandom from fandoms where tracked = 1";
    q1.prepare(qsl);
    q1.exec();
    QStringList result;
    while(q1.next())
        result.push_back(q1.value(0).toString());

    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    return result;
}

QStringList database::FetchTrackedCrossovers()
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QSqlQuery q1(db);
    QString qsl = " select fandom from fandoms where tracked_crossovers = 1";
    q1.prepare(qsl);
    q1.exec();
    QStringList result;
    while(q1.next())
        result.push_back(q1.value(0).toString());

    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    return result;
}

void database::BackupDatabase()
{
    QString backupName = "backups/CrawlerDB." + QDateTime::currentDateTime().date().toString("yyyy-MM-dd") + ".sqlite.zip";
    if(!QFile::exists(backupName))
        JlCompress::compressFile(backupName, "CrawlerDB.sqlite");
    QDir dir("backups");
    QStringList filters;
    filters << "*.zip";
    dir.setNameFilters(filters);
    auto entries = dir.entryList(QDir::NoFilter, QDir::Time|QDir::Reversed);
    int i = 0;
    for(QString entry : entries)
    {
        i++;
        QFileInfo fi(entry);
        if(i < 10)
            continue;
        QFile::remove("backups/" + entry);
    };

}
}
