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

void database::SetFandomTracked(QString fandom, bool crossover, bool tracked)
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QString trackedPart = crossover ? "_crossovers" : "";
    QSqlQuery q1(db);
    QString qsl = " UPDATE fandoms SET tracked%1 = %2 where fandom = '%3'";
    qsl = qsl.arg(trackedPart).arg(QString(tracked ? "1" : "0")).arg(fandom);
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

bool database::FetchTrackStateForFandom( QString fandom, bool crossover)
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QString trackedPart = crossover ? "_crossovers" : "";
    QSqlQuery q1(db);
    QString qsl = " select tracked%1 from fandoms where fandom = '%2' ";
    qsl = qsl.arg(trackedPart).arg(fandom);
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
    if(!QFile::exists("CrawlerDB.sqlite"))
        QFile::copy("CrawlerDB.sqlite.default", "CrawlerDB.sqlite");
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

bool LoadIntoDB(Section & section)
{

    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    bool loaded = false;
    bool isUpdate = false;
    bool isInsert = false;
    QString getKeyQuery = "Select ( select count(*) from FANFICS where AUTHOR = '%1' and TITLE = '%2') as COUNT_NAMED,"
                          " ( select count(*) from FANFICS where AUTHOR = '%1' and TITLE = '%2' and updated <> :updated) as count_updated"
                            " FROM FANFICS WHERE 1=1 ";
    getKeyQuery = getKeyQuery.arg(QString(section.author).replace("'","''")).arg(QString(section.title).replace("'","''"));
    QSqlQuery keyQ(db);
    keyQ.prepare(getKeyQuery);
    keyQ.bindValue(":updated", section.updated);
    keyQ.exec();
    keyQ.next();
    //qDebug() << keyQ.lastQuery();
    //qDebug() << " named: " << keyQ.value(0).toInt();
    //qDebug() << " updated: " << keyQ.value(1).toInt();
    if(keyQ.value(0).toInt() > 0 && keyQ.value(1).toInt() > 0)
        isUpdate = true;
    if(keyQ.value(0).toInt() == 0)
        isInsert = true;

    if(keyQ.lastError().isValid())
    {
        qDebug() << keyQ.lastQuery();
        qDebug() << keyQ.lastError();
    }
    if(isInsert)
    {

        //qDebug() << "Inserting: " << section.author << " " << section.title << " " << section.fandom << " " << section.genre;
        QString query = "INSERT INTO FANFICS (FANDOM, AUTHOR, TITLE,WORDCOUNT, CHAPTERS, FAVOURITES, REVIEWS, CHARACTERS, COMPLETE, RATED, SUMMARY, GENRES, PUBLISHED, UPDATED, URL, ORIGIN) "
                        "VALUES (  :fandom, :author, :title, :wordcount, :CHAPTERS, :FAVOURITES, :REVIEWS, :CHARACTERS, :COMPLETE, :RATED, :summary, :genres, :published, :updated, :url, :origin)";

        QSqlQuery q(db);
        q.prepare(query);
        q.bindValue(":fandom",section.fandom);
        q.bindValue(":author",section.author);
        q.bindValue(":title",section.title);
        q.bindValue(":wordcount",section.wordCount.toInt());
        q.bindValue(":CHAPTERS",section.chapters.trimmed().toInt());
        q.bindValue(":FAVOURITES",section.favourites.toInt());
        q.bindValue(":REVIEWS",section.reviews.toInt());
        q.bindValue(":CHARACTERS",section.characters);
        q.bindValue(":RATED",section.rated);

        q.bindValue(":summary",section.summary);
        q.bindValue(":COMPLETE",section.complete);
        q.bindValue(":genres",section.genre);
        q.bindValue(":published",section.published);
        q.bindValue(":updated",section.updated);
        q.bindValue(":url",section.url);
        q.bindValue(":origin",section.origin);
        q.exec();
        if(q.lastError().isValid())
        {
            qDebug() << "failed to insert: " << section.author << " " << section.title;
            qDebug() << q.lastError();
        }
        loaded=true;
    }

    if(isUpdate)
    {
        //qDebug() << "Updating: " << section.author << " " << section.title;
        QString query = "UPDATE FANFICS set fandom = :fandom, wordcount= :wordcount, CHAPTERS = :CHAPTERS,  COMPLETE = :COMPLETE, FAVOURITES = :FAVOURITES, REVIEWS= :REVIEWS, CHARACTERS = :CHARACTERS, RATED = :RATED, summary = :summary, genres= :genres, published = :published, updated = :updated, url = :url "
                        " where author = :author and title = :title";

        QSqlQuery q(db);
        q.prepare(query);
        q.bindValue(":fandom",section.fandom);
        q.bindValue(":author",section.author);
        q.bindValue(":title",section.title);

        q.bindValue(":wordcount",section.wordCount.toInt());
        q.bindValue(":CHAPTERS",section.chapters.trimmed().toInt());
        q.bindValue(":FAVOURITES",section.favourites.toInt());
        q.bindValue(":REVIEWS",section.reviews.toInt());
        q.bindValue(":CHARACTERS",section.characters);
        q.bindValue(":RATED",section.rated);

        q.bindValue(":summary",section.summary);
        q.bindValue(":COMPLETE",section.complete);
        q.bindValue(":genres",section.genre);
        q.bindValue(":published",section.published);
        q.bindValue(":updated",section.updated);
        q.bindValue(":url",section.url);
        q.exec();
        if(q.lastError().isValid())
        {
            qDebug() << "failed to update: " << section.author << " " << section.title;
            qDebug() << q.lastError();
        }
        loaded=true;
    }
    return loaded;
}

bool LoadRecommendationIntoDB(const Recommender &recommender,  Section &section)
{
    // get recommender id from database if not write him
    // try to update fic inside the database via usual Load
    // find id of the fic in question
    // insert if not exists recommender/ficid pair
    LoadIntoDB(section);
    int fic_id = GetFicIdByAuthorAndName(section.author, section.title);
    //int rec_id = GetRecommenderId(recommender.url);
    if(!WriteRecommendation(recommender, fic_id))
        return false;
    return true;
}

int GetFicIdByAuthorAndName(QString author, QString title)
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QSqlQuery q1(db);
    QString qsl = " select id from fanfics where author = :author and title = :title";
    q1.prepare(qsl);
    q1.bindValue(":author", author);
    q1.bindValue(":title", title);
    q1.exec();
    int result;
    while(q1.next())
        result = q1.value(0).toInt();

    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    return result;
}

bool WriteRecommendation(const Recommender &recommender, int fic_id)
{
    int recommender_id;
    while(recommender_id = GetRecommenderId(recommender.url), recommender_id == -1)
    {
        WriteRecommender(recommender);
    }
    if(recommender_id < 0)
        return false;

    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QSqlQuery q1(db);
    QString qsl = " insert into recommendations (recommender_id, fic_id) values( :recommender_id,:fic_id); ";
    q1.prepare(qsl);
    q1.bindValue(":recommender_id", recommender_id);
    q1.bindValue(":fic_id", fic_id);
    q1.exec();

    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    return true;
}

int GetRecommenderId(QString url)
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QSqlQuery q1(db);
    QString qsl = " select id from recommenders where url = :url ";
    q1.prepare(qsl);
    q1.bindValue(":url", url);
    q1.exec();
    int result = -1;
    while(q1.next())
        result = q1.value(0).toInt();

    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
        return -2;
    }
    return result;
}
void WriteRecommender(const Recommender& recommender)
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QSqlQuery q1(db);
    QString qsl = " insert into recommenders(name, url, page_data, page_updated) values(:name, :url, :page_data, date('now'))";
    q1.prepare(qsl);
    q1.bindValue(":name", recommender.name);
    q1.bindValue(":url", recommender.url);
    q1.bindValue(":page_data", recommender.pageData);
    q1.exec();

    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
}

QHash<QString, Recommender> FetchRecommenders()
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QSqlQuery q1(db);
    QString qsl = "select * from recommenders  order by name asc";
    q1.prepare(qsl);
    q1.exec();
    QHash<QString, Recommender> result;
    while(q1.next())
    {
        Recommender rec;
        rec.id = q1.value("ID").toInt();
        rec.name= q1.value("name").toString();
        rec.url= q1.value("url").toString();
        result[rec.name] = rec;
    }
    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    return result;
}

void RemoveRecommender(const Recommender &recommender)
{
    QSqlDatabase db = QSqlDatabase::database("QSQLITE_R");
    QSqlQuery q1(db);
    QString qsl = "delete from recommendations where recommender_id = %1";
    qsl=qsl.arg(GetRecommenderId(recommender.url));
    q1.prepare(qsl);
    q1.exec();

    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    QSqlQuery q2(db);
    qsl = "delete from recommenders where url = '%1'";
    qsl=qsl.arg(recommender.url);
    q2.prepare(qsl);
    q2.exec();
    if(q2.lastError().isValid())
    {
        qDebug() << q2.lastError();
        qDebug() << q2.lastQuery();
    }
}

}
