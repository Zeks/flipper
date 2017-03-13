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
#include <algorithm>

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

             QSqlDatabase db = QSqlDatabase::database();
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
    QSqlDatabase db = QSqlDatabase::database();
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
    QSqlDatabase db = QSqlDatabase::database();
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
    QSqlDatabase db = QSqlDatabase::database();
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
    QSqlDatabase db = QSqlDatabase::database();
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
    QSqlDatabase db = QSqlDatabase::database();
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
    QSqlDatabase db = QSqlDatabase::database();
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
    QSqlDatabase db = QSqlDatabase::database();
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
    QSqlDatabase db = QSqlDatabase::database();
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
    QDir dir("backups");
    QStringList filters;
    filters << "*.zip";
    dir.setNameFilters(filters);
    auto entries = dir.entryList(QDir::NoFilter, QDir::Time|QDir::Reversed);
    qSort(entries.begin(),entries.end());
    std::reverse(entries.begin(),entries.end());
    qDebug() << entries;

    if(!QFile::exists("CrawlerDB.sqlite"))
        QFile::copy("CrawlerDB.sqlite.default", "CrawlerDB.sqlite");
    QString backupName = "backups/CrawlerDB." + QDateTime::currentDateTime().date().toString("yyyy-MM-dd") + ".sqlite.zip";
    if(!QFile::exists(backupName))
        JlCompress::compressFile(backupName, "CrawlerDB.sqlite");



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
    qDebug() << "Loading:" << section.title;
    QSqlDatabase db = QSqlDatabase::database();
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
    if(isUpdate == false && isInsert == false)
        return false;

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

bool LoadRecommendationIntoDB(Recommender &recommender,  Section &section)
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
    QSqlDatabase db = QSqlDatabase::database();
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

bool WriteRecommendation( Recommender &recommender, int fic_id)
{
    if(recommender.id == -3)
        recommender.id =  GetRecommenderId(recommender.url);
    if(recommender.id == -1)
    {
        WriteRecommender(recommender);
        recommender.id =  GetRecommenderId(recommender.url);
    }

    if(recommender.id < 0)
        return false;

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = " insert into recommendations (recommender_id, fic_id) values( :recommender_id,:fic_id); ";
    q1.prepare(qsl);
    q1.bindValue(":recommender_id", recommender.id);
    q1.bindValue(":fic_id", fic_id);
    q1.exec();

    if(q1.lastError().isValid()&& !q1.lastError().text().contains("UNIQUE constraint failed"))
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    return true;
}

int GetRecommenderId(QString url)
{
    QSqlDatabase db = QSqlDatabase::database();
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
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = " insert into recommenders(name, url, page_data, page_updated, wave) values(:name, :url, :page_data, date('now'), :wave)";
    q1.prepare(qsl);
    q1.bindValue(":name", recommender.name);
    q1.bindValue(":url", recommender.url);
    q1.bindValue(":page_data", recommender.pageData);
    q1.bindValue(":wave", recommender.wave);
    q1.exec();

    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
}

QHash<QString, Recommender> FetchRecommenders(int limitingWave)
{
    limitingWave=1;
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = "select * from recommenders  where wave <= :wave order by name asc";
    q1.prepare(qsl);
    q1.bindValue(":wave", limitingWave);
    q1.exec();
    QHash<QString, Recommender> result;
    while(q1.next())
    {
        Recommender rec;
        rec.id = q1.value("ID").toInt();
        rec.name= q1.value("name").toString();
        rec.url= q1.value("url").toString();
        rec.wave= q1.value("wave").toInt();
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
    int id = GetRecommenderId(recommender.url);
    RemoveRecommender(id);
}
void RemoveRecommender(int id)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = "delete from recommendations where recommender_id = %1";
    qsl=qsl.arg(QString::number(id));
    q1.prepare(qsl);
    q1.exec();

    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    QSqlQuery q2(db);
    qsl = "delete from recommenders where id = %1";
    qsl=qsl.arg(id);
    q2.prepare(qsl);
    q2.exec();
    if(q2.lastError().isValid())
    {
        qDebug() << q2.lastError();
        qDebug() << q2.lastQuery();
    }
}

int FilterRecommenderByRecField(int recommender_id, int rec_threshhold, int favCount)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = "select count(id) from fanfics where tags like '% rec%' and id in (select fic_id from recommendations where recommender_id = %1)";
    qsl=qsl.arg(QString::number(recommender_id));
    q1.prepare(qsl);
    q1.exec();
    q1.next();
    if(q1.lastError().isValid())
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }

    int matches  = q1.value(0).toInt();
    qDebug() << "Using query: " << q1.lastQuery();
    qDebug() << "Matches found: " << matches;
    bool passesOnLimitedFaves = favCount < 130 && (matches >= rec_threshhold - 1);
    if(!passesOnLimitedFaves && matches < rec_threshhold)
    {
        RemoveRecommender(recommender_id);
    }
    return matches;
}
//CREATE INDEX if not exists  I_RECOMMENDATIONS ON Recommendations (recommender_id ASC);
//CREATE INDEX if not exists  I_FIC_ID ON Recommendations (fic_id ASC);
void DropFanficIndexes()
{
    QStringList commands;
    //commands.push_back("Drop index if exists main.I_FANFICS_IDENTITY");
    commands.push_back("Drop index if exists main.I_FANFICS_FANDOM");
    commands.push_back("Drop index if exists main.I_FANFICS_WORDCOUNT");
    commands.push_back("Drop index if exists main.I_FANFICS_TAGS");
    //commands.push_back("Drop index if exists main.I_FANFICS_ID");
    commands.push_back("Drop index if exists main.I_FANFICS_GENRES");
    commands.push_back("Drop index if exists main.I_FIC_ID");
    //commands.push_back("Drop index if exists main.I_RECOMMENDATIONS");

    QSqlDatabase db = QSqlDatabase::database();
    for(QString command: commands)
    {
        QSqlQuery q1(db);
        q1.prepare(command);
        q1.exec();
        if(q1.lastError().isValid())
        {
            qDebug() << q1.lastError();
            qDebug() << q1.lastQuery();
        }
    }

}
//CREATE INDEX if not exists  I_RECOMMENDATIONS ON Recommendations (recommender_id ASC);
//CREATE INDEX if not exists  I_FIC_ID ON Recommendations (fic_id ASC);
void RebuildFanficIndexes()
{
    QStringList commands;
    //commands.push_back("CREATE INDEX  if  not exists  main.I_FANFICS_IDENTITY ON FANFICS (AUTHOR ASC, TITLE ASC)");
    commands.push_back("CREATE  INDEX if  not exists  main.I_FANFICS_FANDOM ON FANFICS (FANDOM ASC)");
    commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_WORDCOUNT ON FANFICS (WORDCOUNT ASC)");
    commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_TAGS ON FANFICS (TAGS ASC)");
    //commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_ID ON FANFICS (ID ASC)");
    commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_GENRES ON FANFICS (GENRES ASC)");
    //commands.push_back("CREATE INDEX if not exists  I_RECOMMENDATIONS ON Recommendations (recommender_id ASC)");
    commands.push_back("CREATE INDEX if not exists  I_FIC_ID ON Recommendations (fic_id ASC)");

    QSqlDatabase db = QSqlDatabase::database();
    for(QString command: commands)
    {
        QSqlQuery q1(db);
        q1.prepare(command);
        q1.exec();
        if(q1.lastError().isValid())
        {
            qDebug() << q1.lastError();
            qDebug() << q1.lastQuery();
        }
    }
}

WriteStats ProcessSectionsIntoUpdateAndInsert(const QList<Section> & sections)
{
    WriteStats result;
    result.requiresInsert.reserve(sections.size());
    result.requiresUpdate.reserve(sections.size());
    QSqlDatabase db = QSqlDatabase::database();
    QString getKeyQuery = "Select ( select count(*) from FANFICS where AUTHOR = '%1' and TITLE = '%2') as COUNT_NAMED,"
                          " ( select count(*) from FANFICS where AUTHOR = '%1' and TITLE = '%2' and updated <> :updated) as count_updated"
                            " FROM FANFICS WHERE 1=1 ";
    for(const Section& section : sections)
    {
        QString filledQuery = getKeyQuery.arg(QString(section.author).replace("'","''")).arg(QString(section.title).replace("'","''"));
        QSqlQuery keyQ(db);
        keyQ.prepare(filledQuery);
        keyQ.bindValue(":updated", section.updated);
        keyQ.exec();
        keyQ.next();
        if(keyQ.value(0).toInt() > 0 && keyQ.value(1).toInt() > 0)
            result.requiresUpdate.push_back(section);
        if(keyQ.value(0).toInt() == 0)
            result.requiresInsert.push_back(section);

        if(keyQ.lastError().isValid())
        {
            qDebug() << keyQ.lastQuery();
            qDebug() << keyQ.lastError();
        }
    }
    return result;
}

bool UpdateInDB(Section &section)
{
    bool loaded = false;
    QSqlDatabase db = QSqlDatabase::database();
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
        return false;
    }
    loaded=true;
    return loaded;

}

bool InsertIntoDB(Section &section)
{
    QString query = "INSERT INTO FANFICS (FANDOM, AUTHOR, TITLE,WORDCOUNT, CHAPTERS, FAVOURITES, REVIEWS, CHARACTERS, COMPLETE, RATED, SUMMARY, GENRES, PUBLISHED, UPDATED, URL, ORIGIN) "
                    "VALUES (  :fandom, :author, :title, :wordcount, :CHAPTERS, :FAVOURITES, :REVIEWS, :CHARACTERS, :COMPLETE, :RATED, :summary, :genres, :published, :updated, :url, :origin)";
    QSqlDatabase db = QSqlDatabase::database();
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
        return false;
    }
    return true;
}

void DropAllFanficIndexes()
{
    QStringList commands;
    commands.push_back("Drop index if exists main.I_FANFICS_IDENTITY");
    commands.push_back("Drop index if exists main.I_FANFICS_FANDOM");
    commands.push_back("Drop index if exists main.I_FANFICS_AUTHOR");
    commands.push_back("Drop index if exists main.I_FANFICS_TITLE");
    commands.push_back("Drop index if exists main.I_FANFICS_WORDCOUNT");
    commands.push_back("Drop index if exists main.I_FANFICS_TAGS");
    commands.push_back("Drop index if exists main.I_FANFICS_ID");
    commands.push_back("Drop index if exists main.I_FANFICS_GENRES");

    QSqlDatabase db = QSqlDatabase::database();
    for(QString command: commands)
    {
        QSqlQuery q1(db);
        q1.prepare(command);
        q1.exec();
        if(q1.lastError().isValid())
        {
            qDebug() << q1.lastError();
            qDebug() << q1.lastQuery();
        }
    }
}

void RebuildAllFanficIndexes()
{
    QStringList commands;
    commands.push_back("CREATE INDEX  if  not exists  main.I_FANFICS_IDENTITY ON FANFICS (AUTHOR ASC, TITLE ASC)");
    commands.push_back("CREATE  INDEX if  not exists  main.I_FANFICS_AUTHOR ON FANFICS (AUTHOR ASC)");
    commands.push_back("CREATE  INDEX if  not exists  main.I_FANFICS_TITLE ON FANFICS (TITLE ASC)");
    commands.push_back("CREATE  INDEX if  not exists  main.I_FANFICS_FANDOM ON FANFICS (FANDOM ASC)");
    commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_WORDCOUNT ON FANFICS (WORDCOUNT ASC)");
    commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_TAGS ON FANFICS (TAGS ASC)");
    commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_ID ON FANFICS (ID ASC)");
    commands.push_back("CREATE  INDEX if  not exists main.I_FANFICS_GENRES ON FANFICS (GENRES ASC)");

    QSqlDatabase db = QSqlDatabase::database();
    for(QString command: commands)
    {
        QSqlQuery q1(db);
        q1.prepare(command);
        q1.exec();
        if(q1.lastError().isValid())
        {
            qDebug() << q1.lastError();
            qDebug() << q1.lastQuery();
        }
    }
}
QStringList GetFandomListFromDB(QString section)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("Select fandom from fandoms where section ='%1' and normal_url is not null").arg(section);
    QSqlQuery q(qs, db);
    QStringList result;
    result.append("");
    while(q.next())
    {
        result.append(q.value(0).toString());
    }
    return result;
}


void AssignTagToFandom(QString tag, QString fandom)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("update fanfics set tags = tags || ' ' || '%1' where fandom like '%%2%' and tags not like '%%1%'").arg(tag).arg(fandom);
    QSqlQuery q(qs, db);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
    }
}


QDateTime GetMaxUpdateDateForSection(QStringList sections)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("Select max(updated) as updated from fanfics where 1 = 1 %1");
    QString append;
    if(sections.size() == 1)
    {
        append+= QString(" and fandom = '%1' ").arg(sections.at(0));
    }
    else
    {
        for(auto section : sections)
        {
            append+= QString(" and fandom like '%%1%' ").arg(section);
        }
    }
    qs=qs.arg(append);
    QSqlQuery q(qs, db);
    q.next();
    qDebug() << q.lastQuery();
    //QDateTime result = q.value("updated").toDateTime();
    QString resultStr = q.value("updated").toString();
    QDateTime result;
    result = QDateTime::fromString(resultStr, "yyyy-MM-ddThh:mm:ss.000");
    return result;
}


}
