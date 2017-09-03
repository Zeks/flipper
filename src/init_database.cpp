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
#include <QSqlDriver>
#include <QPluginLoader>
#include "sqlite/sqlite3.h"

namespace database{

bool ExecAndCheck(QSqlQuery& q)
{
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return false;
    }
    return true;
}
bool CheckExecution(QSqlQuery& q)
{
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return false;
    }
    return true;
}
bool ExecuteQuery(QSqlQuery& q, QString query)
{
    QString qs = query;
    q.prepare(qs);
    q.exec();

    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return false;
    }
    return true;
}

bool ExecuteQueryChain(QSqlQuery& q, QStringList queries)
{
    for(auto query: queries)
    {
        if(!ExecuteQuery(q, query))
            return false;
    }
    return true;
}

void cfRegexp(sqlite3_context* ctx, int argc, sqlite3_value** argv)
{
    QRegExp regex;
    QString str1((const char*)sqlite3_value_text(argv[0]));
    QString str2((const char*)sqlite3_value_text(argv[1]));

    regex.setPattern(str1);
    regex.setCaseSensitivity(Qt::CaseInsensitive);

    bool b = str2.contains(regex);

    if (b)
    {
        sqlite3_result_int(ctx, 1);
    }
    else
    {
        sqlite3_result_int(ctx, 0);
    }
}
void cfReturnCapture(sqlite3_context* ctx, int argc, sqlite3_value** argv)
{
    QString pattern((const char*)sqlite3_value_text(argv[0]));
    QString str((const char*)sqlite3_value_text(argv[1]));
    QRegExp regex(pattern);
    regex.indexIn(str);
    QString cap = regex.cap(1);
    sqlite3_result_text(ctx, qPrintable(cap), cap.length(), SQLITE_TRANSIENT);
}
void cfGetFirstFandom(sqlite3_context* ctx, int argc, sqlite3_value** argv)
{
    QRegExp regex("&");
    QString result;
    QString str((const char*)sqlite3_value_text(argv[0]));
    int index = regex.indexIn(str);
    if(index == -1)
        result = str;
    else
        result = str.left(index);
    sqlite3_result_text(ctx, qPrintable(result), result.length(), SQLITE_TRANSIENT);
}
void cfGetSecondFandom(sqlite3_context* ctx, int argc, sqlite3_value** argv)
{
    QRegExp regex("&");
    QString result;
    QString str((const char*)sqlite3_value_text(argv[0]));
    int index = regex.indexIn(str);
    if(index == -1)
        result = "";
    else
        result = str.mid(index+1);
    sqlite3_result_text(ctx, qPrintable(result), result.length(), SQLITE_TRANSIENT);
}
void InstallCustomFunctions()
{
    QSqlDatabase db = QSqlDatabase::database();
    QVariant v = db.driver()->handle();
    if (v.isValid() && qstrcmp(v.typeName(), "sqlite3*")==0) {
        sqlite3 *db_handle = *static_cast<sqlite3 **>(v.data());
        if (db_handle != 0) {
            sqlite3_initialize();
            sqlite3_create_function(db_handle, "cfRegexp", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &cfRegexp, NULL, NULL);
            sqlite3_create_function(db_handle, "cfGetFirstFandom", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &cfGetFirstFandom, NULL, NULL);
            sqlite3_create_function(db_handle, "cfGetSecondFandom", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &cfGetSecondFandom, NULL, NULL);
            sqlite3_create_function(db_handle, "cfReturnCapture", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &cfReturnCapture, NULL, NULL);
        }
    }
}
//"dbcode/dbinit.sql"
bool database::ReadDbFile(QString file, QString connectionName)
{
    QFile data(file);
     if (data.open(QFile::ReadOnly))
     {
         QTextStream in(&data);
         QString db = in.readAll();
         QStringList statements = db.split(";");
         for(QString statement: statements)
         {
             if(statement.trimmed().isEmpty() || statement.trimmed().left(2) == "--")
                 continue;

             QSqlDatabase db ;
             if(!connectionName.isEmpty())
                db = QSqlDatabase::database(connectionName);
             else
                db = QSqlDatabase::database();
             QSqlQuery q(db);
             ExecAndCheck(q);
         }
     }
     else
         return false;
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
    ExecAndCheck(q1);
}

void database::PushFandom(QString fandom)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString upsert1 ("UPDATE recent_fandoms SET seq_num= (select max(seq_num) +1 from  recent_fandoms ) WHERE fandom = '%1'; ");
    QString upsert2 ("INSERT INTO recent_fandoms(fandom, seq_num) select '%1',  (select max(seq_num)+1 from recent_fandoms) WHERE changes() = 0;");
    QSqlQuery q1(db);
    upsert1 = upsert1.arg(fandom);
    q1.prepare(upsert1);
    ExecAndCheck(q1);
    QSqlQuery q2(db);
    upsert2 = upsert2.arg(fandom);
    q2.prepare(upsert2);
    ExecAndCheck(q2);
}

void database::RebaseFandoms()
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = "UPDATE recent_fandoms SET seq_num = seq_num - (select min(seq_num) from recent_fandoms where fandom is not 'base') where fandom is not 'base'";
    q1.prepare(qsl);
    ExecAndCheck(q1);
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
    CheckExecution(q1);
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
    CheckExecution(q1);
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

    CheckExecution(q1);
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

    CheckExecution(q1);

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

bool LoadIntoDB(Fic & section)
{
    qDebug() << "Loading:" << section.title;
    QSqlDatabase db = QSqlDatabase::database();
    bool loaded = false;
    bool isUpdate = false;
    bool isInsert = false;
    QString getKeyQuery = "Select ( select count(*) from FANFICS where web_uid = %1) as COUNT_NAMED,"
                          " ( select count(*) from FANFICS where web_uid = %1 and updated <> :updated) as count_updated"
                            " FROM FANFICS WHERE 1=1 ";
    getKeyQuery = getKeyQuery.arg(QString::number(section.webId));
    QSqlQuery keyQ(db);
    keyQ.prepare(getKeyQuery);
    keyQ.bindValue(":updated", section.updated);
    keyQ.exec();
    keyQ.next();
    if(keyQ.value(0).toInt() > 0 && keyQ.value(1).toInt() > 0)
        isUpdate = true;
    if(keyQ.value(0).toInt() == 0)
        isInsert = true;
    if(isUpdate == false && isInsert == false)
        return false;

    CheckExecution(keyQ);
    if(isInsert)
    {

        QString query = "INSERT INTO FANFICS (web_uid FANDOM, AUTHOR, TITLE,WORDCOUNT, CHAPTERS, FAVOURITES, REVIEWS, CHARACTERS, COMPLETE, RATED, SUMMARY, GENRES, PUBLISHED, UPDATED, URL, ORIGIN) "
                        "VALUES (  :web_uid, :fandom, :author, :title, :wordcount, :CHAPTERS, :FAVOURITES, :REVIEWS, :CHARACTERS, :COMPLETE, :RATED, :summary, :genres, :published, :updated, :url, :origin)";

        QSqlQuery q(db);
        q.prepare(query);
        q.bindValue(":fandom",section.fandom);
        q.bindValue(":author",section.author.name);
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
        q.bindValue(":url",section.url("ffn"));
        q.bindValue(":origin",section.origin);
        q.bindValue(":web_uid",section.webId);
        q.exec();
        if(!CheckExecution(q))
            qDebug() << "failed to insert: " << section.author.name << " " << section.title;

        loaded=true;
    }

    if(isUpdate)
    {
        //qDebug() << "Updating: " << section.author << " " << section.title;
        QString query = "UPDATE FANFICS set fandom = :fandom, wordcount= :wordcount, CHAPTERS = :CHAPTERS,  COMPLETE = :COMPLETE, FAVOURITES = :FAVOURITES, REVIEWS= :REVIEWS, CHARACTERS = :CHARACTERS, RATED = :RATED, summary = :summary, genres= :genres, published = :published, updated = :updated, url = :url "
                        " where web_uid = :web_uid";

        QSqlQuery q(db);
        q.prepare(query);
        q.bindValue(":fandom",section.fandom);
        q.bindValue(":author",section.author.name);
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
        q.bindValue(":url",section.url("ffn"));
        q.bindValue(":web_uid",section.webId);
        q.exec();
        if(!CheckExecution(q))
            qDebug() << "failed to update: " << section.author.name << " " << section.title;

        loaded=true;
    }
    return loaded;
}

bool LoadRecommendationIntoDB(FavouritesPage &recommender,  Fic &section)
{
    // get recommender id from database if not write him
    // try to update fic inside the database via usual Load
    // find id of the fic in question
    // insert if not exists recommender/ficid pair
    LoadIntoDB(section);
    int fic_id = GetFicIdByWebId(section.webId);
    //int rec_id = GetRecommenderId(recommender.url);
    if(!WriteRecommendation(recommender.author, fic_id))
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
    int result = -1;
    while(q1.next())
        result = q1.value(0).toInt();
    CheckExecution(q1);
    return result;
}


int GetFicIdByWebId(int webId)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = " select id from fanfics where web_uid = :web_uid";
    q1.prepare(qsl);
    q1.bindValue(":web_uid",webId);
    q1.exec();
    int result = -1;
    while(q1.next())
        result = q1.value(0).toInt();
    CheckExecution(q1);

    return result;
}

bool EnsureIdForAuthor(Author &author)
{
    if(author.GetIdStatus() == AuthorIdStatus::unassigned)
        author.AssignId(GetAuthorIdFromUrl(author.url("ffn")));
    if(author.GetIdStatus() == AuthorIdStatus::not_found)
    {
        WriteRecommender(author);
        author.AssignId(GetAuthorIdFromUrl(author.url("ffn")));
    }
    if(author.id < 0)
        return false;
    return true;
}

bool WriteRecommendation(Author &author, int fic_id)
{
    if(!EnsureIdForAuthor(author))
        return false;

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    // atm this pairs favourite story with an author
    QString qsl = " insert into recommendations (recommender_id, fic_id) values(:recommender_id,:fic_id); ";
    q1.prepare(qsl);
    q1.bindValue(":recommender_id", author.id);
    q1.bindValue(":fic_id", fic_id);
    q1.exec();

    if(q1.lastError().isValid() && !q1.lastError().text().contains("UNIQUE constraint failed"))
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }
    return true;
}

int GetAuthorIdFromUrl(QString url)
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

    if(!CheckExecution(q1))
        return -2;

    return result;
}
void WriteRecommender(const FavouritesPage& recommender)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = " insert into recommenders(name, url, page_updated) values(:name, :url,  date('now'))";
    q1.prepare(qsl);
    q1.bindValue(":name", recommender.name);
    q1.bindValue(":url", recommender.url);
    q1.exec();
    CheckExecution(q1);
}

QHash<QString, FavouritesPage> FetchRecommenders()
{
    limitingWave=1;
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = "select * from recommenders authors order by name asc";
    q1.prepare(qsl);
    q1.exec();
    QHash<QString, FavouritesPage> result;
    while(q1.next())
    {
        FavouritesPage rec;
        rec.id = q1.value("ID").toInt();
        rec.name= q1.value("name").toString();
        rec.url= q1.value("url").toString();
        rec.wave= q1.value("wave").toInt();
        result[rec.name] = rec;
    }
    CheckExecution(q1);
    return result;
}

void RemoveAuthor(const Author &recommender)
{
    int id = GetAuthorIdFromUrl(recommender.url("ffn"));
    RemoveAuthor(id);
}
void RemoveAuthor(int id)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = "delete from recommendations where recommender_id = %1";
    qsl=qsl.arg(QString::number(id));
    q1.prepare(qsl);
    ExecAndCheck(q1);

    QSqlQuery q2(db);
    qsl = "delete from recommenders where id = %1";
    qsl=qsl.arg(id);
    q2.prepare(qsl);
    ExecAndCheck(q2);
}

int GetMatchCountForRecommenderOnList(int recommender_id, int list)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q1(db);
    QString qsl = "select fic_count from RecommendationTagStats where list_id = :list_id and author_id = :author_id";
    q1.prepare(qsl);
    q1.bindValue(":list_id", list);
    q1.bindValue(":author_id", recommender_id);
    q1.exec();
    q1.next();
    CheckExecution(q1);
    int matches  = q1.value(0).toInt();
    qDebug() << "Using query: " << q1.lastQuery();
    qDebug() << "Matches found: " << matches;
    return matches;
}

WriteStats ProcessFicsIntoUpdateAndInsert(const QList<Fic> & sections)
{
    WriteStats result;
    result.requiresInsert.reserve(sections.size());
    result.requiresUpdate.reserve(sections.size());
    QSqlDatabase db = QSqlDatabase::database();
    QString getKeyQuery = "Select ( select count(*) from FANFICS where  web_uid = :web_uid) as COUNT_NAMED,"
                          " ( select count(*) from FANFICS where  web_uid = :web_uid and updated <> :updated) as count_updated"
                            " FROM FANFICS WHERE 1=1 ";
    for(const Fic& section : sections)
    {
        QString filledQuery = getKeyQuery;
        QSqlQuery keyQ(db);
        keyQ.prepare(filledQuery);
        keyQ.bindValue(":updated", section.updated);
        keyQ.bindValue(":web_uid", section.webId);
        keyQ.exec();
        keyQ.next();
        if(keyQ.value(0).toInt() > 0 && keyQ.value(1).toInt() > 0)
            result.requiresUpdate.push_back(section);
        if(keyQ.value(0).toInt() == 0)
            result.requiresInsert.push_back(section);

        CheckExecution(keyQ);
    }
    return result;
}

bool UpdateInDB(Fic &section)
{
    bool loaded = false;
    QSqlDatabase db = QSqlDatabase::database();
    QString query = "UPDATE FANFICS set fandom = :fandom, wordcount= :wordcount, CHAPTERS = :CHAPTERS,  "
                    "COMPLETE = :COMPLETE, FAVOURITES = :FAVOURITES, REVIEWS= :REVIEWS, CHARACTERS = :CHARACTERS, RATED = :RATED, "
                    "summary = :summary, genres= :genres, published = :published, updated = :updated, url = :url "
                    " where web_uid = :web_uid";

    QSqlQuery q(db);
    q.prepare(query);
    q.bindValue(":fandom",section.fandom);
    q.bindValue(":author",section.author.name);
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
    q.bindValue(":url",section.url("ffn"));
    q.bindValue(":web_uid",section.webId);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << "failed to update: " << section.author.name << " " << section.title;
        qDebug() << q.lastError();
        return false;
    }
    loaded=true;
    return loaded;

}

bool InsertIntoDB(Fic &section)
{
    QString query = "INSERT INTO FANFICS (web_uid, FANDOM, AUTHOR, TITLE,WORDCOUNT, CHAPTERS, FAVOURITES, REVIEWS, "
                    " CHARACTERS, COMPLETE, RATED, SUMMARY, GENRES, PUBLISHED, UPDATED, URL, ORIGIN) "
                    "VALUES ( :web_uid,  :fandom, :author, :title, :wordcount, :CHAPTERS, :FAVOURITES, :REVIEWS, "
                    " :CHARACTERS, :COMPLETE, :RATED, :summary, :genres, :published, :updated, :url, :origin)";
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare(query);
    q.bindValue(":web_uid",section.webId);
    q.bindValue(":fandom",section.fandom);
    q.bindValue(":author",section.author.name);
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
    q.bindValue(":url",section.url("ffn"));
    q.bindValue(":origin",section.origin);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << "failed to insert: " << section.author.name << " " << section.title;
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
    commands.push_back("Drop index if exists main.I_RECOMMENDATIONS_FIC_TAG");
    commands.push_back("Drop index if exists main.I_RECOMMENDATIONS_TAG");
    commands.push_back("Drop index if exists main.I_RECOMMENDATIONS_FIC");

    QSqlDatabase db = QSqlDatabase::database();
    for(QString command: commands)
    {
        QSqlQuery q1(db);
        q1.prepare(command);
        ExecAndCheck(q1);
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

    commands.push_back("CREATE INDEX if not exists I_RECOMMENDATIONS_REC_FIC ON Recommendations (recommender_id ASC, fic_id asc)");
    commands.push_back("CREATE INDEX if not exists I_RECOMMENDATIONS_TAG ON Recommendations (tag ASC)");
    commands.push_back("CREATE INDEX if not exists I_RECOMMENDATIONS_FIC ON Recommendations (fic_id ASC)");

    QSqlDatabase db = QSqlDatabase::database();
    for(QString command: commands)
    {
        QSqlQuery q1(db);
        q1.prepare(command);
        ExecAndCheck(q1);
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
        result.append(q.value(0).toString());
    CheckExecution(q);

    return result;
}


void AssignTagToFandom(QString tag, QString fandom)
{
    QSqlDatabase db = QSqlDatabase::database();
    //QString qs = QString("update fanfics set tags = tags || ' ' || '%1' where fandom like '%%2%' and tags not like '%%1%'").arg(tag).arg(fandom);QString qs =
    //!!! check
    QString qs = "INSERT INTO FicTags(fic_id, tag) SELECT id, %1 as tag from fanfics f WHERE fandom like '%%2%' and NOT EXISTS(SELECT 1 FROM FicTags WHERE fic_id = f.id and tag = %1)";
    qs=qs.arg(tag).arg(fandom);
    QSqlQuery q(qs, db);
    q.exec();
    if(q.lastError().isValid() && !q.lastError().text().contains("UNIQUE constraint failed"))
    {
        qDebug() << q1.lastError();
        qDebug() << q1.lastQuery();
    }

}


QDateTime GetMaxUpdateDateForFandom(QStringList sections)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("Select max(updated) as updated from fanfics where 1 = 1 %1");
    QString append;
    if(sections.size() == 1)
        append+= QString(" and fandom = '%1' ").arg(sections.at(0));
    else
    {
        for(auto section : sections)
        {
            if(!section.trimmed().isEmpty())
                append+= QString(" and fandom like '%%1%' ").arg(section);
        }
    }
    qs=qs.arg(append);
    QSqlQuery q(qs, db);
    q.next();
    //qDebug() << q.lastQuery();
    //QDateTime result = q.value("updated").toDateTime();
    QString resultStr = q.value("updated").toString();
    QDateTime result;
    result = QDateTime::fromString(resultStr, "yyyy-MM-ddThh:mm:ss.000");
    return result;
}


void EnsureFandomsFilled()
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("update fanfics set fandom1 = cfGetFirstFandom(fandom), fandom2 = cfGetSecondfandom(fandom) where fandom1 is null and fandom2 is null");
    QSqlQuery q(db);
    q.prepare(qs);
    ExecAndCheck(q);
}

void EnsureWebIdsFilled()
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("update fanfics set web_uid = cfReturnCapture('/s/(\\d+)', url) where web_uid is null");
    QSqlQuery q(db);
    q.prepare(qs);
    ExecAndCheck(q);
}


//bool EnsureTagForRecommendations()
//{
//    QSqlDatabase db = QSqlDatabase::database();
//    QString qs = QString("PRAGMA table_info(recommendations)");
//    QSqlQuery q(db);
//    q.prepare(qs);
//    q.exec();
//    while(q.next())
//    {
//        if(q.value("name").toString() == "tag")
//            return true;
//    }
//    if(q.lastError().isValid())
//    {
//        qDebug() << q.lastError();
//        qDebug() << q.lastQuery();
//        return false;
//    }

//    bool result =  ExecuteQueryChain(q,
//                     {"CREATE TABLE if not exists Recommendations2(recommender_id INTEGER NOT NULL , fic_id INTEGER NOT NULL , tag varchar default 'core', PRIMARY KEY (recommender_id, fic_id, tag) );",
//                      "insert into Recommendations2 (recommender_id,fic_id, tag) SELECT recommender_id, fic_id, 'core' from Recommendations;",
//                      "DROP TABLE Recommendations;",
//                      "ALTER TABLE Recommendations2 RENAME TO Recommendations;",
//                      "drop index i_recommendations;",
//                      "CREATE INDEX if not exists  I_RECOMMENDATIONS ON Recommendations (recommender_id ASC);"
//                      "CREATE INDEX if not exists  I_RECOMMENDATIONS_TAG ON Recommendations (tag ASC);"
//                      "CREATE INDEX if not exists I_FIC_ID ON Recommendations (fic_id ASC);",
//                      });
//    return result;
//}

QStringList ReadAvailableRecommendationLists()
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select distinct name from RecommendationLists order by name");
    QSqlQuery q(db);
    q.prepare(qs);
    QStringList result;
    if(!ExecAndCheck(q))
        return result;

    while(q.next())
        result.push_back(q.value(0).toString());

    return result;
}

//static QString WrapTag(QString tag)
//{
//    tag= "(.{0,}" + tag + ".{0,})";
//    return tag;
//}


AuthorRecommendationStats CreateRecommenderStats(int recommenderId, QString listName)
{
    AuthorRecommendationStats result;
    result.tag = listName;
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select count(distinct fic_id) from recommendations where recommender_id = :id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id",recommenderId);
    q.exec();
    q.next();
    if(!CheckExecution(q))
        return result;

    result.authorId = recommenderId;
    result.totalFics = q.value(0).toInt();

    auto listId= GetRecommendationListIdFromName(listName);
    //!!! проверить
    qs = QString("select count(distinct fic_id) from RecommendationListData rld where rld.list_id = :list_id and exists (select 1 from Recommendations where rld.fic_id = fic_id and recommeder_id = :recommeder_id)");
    q.prepare(qs);
    q.bindValue(":id",recommenderId);
    q.bindValue(":list_id",listId);
    q.bindValue(":recommeder_id",recommenderId);
    q.exec();
    if(!CheckExecution(q))
        return result;

    q.next();
    result.matchesWithReferenceTag = q.value(0).toInt();
    result.matchRatio = (double)result.totalFics/(double)result.matchesWithReferenceTag;
    result.isValid = true;
    return result;
}


bool DeleteRecommendationList(QString listName)
{
    QSqlDatabase db = QSqlDatabase::database();
    auto listId= GetRecommendationListIdFromName(listName);
    QString qs = QString("delete from RecommendationLists where list_id = :list_id");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(ExecAndCheck(q))
        return false;

    qs = QString("delete from RecommendationTagStats where list_id = :list_id");
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(ExecAndCheck(q))
        return false;

    qs = QString("delete from RecommendationFicStats where list_id = :list_id");
    q.prepare(qs);
    q.bindValue(":list_id",listId);
    if(ExecAndCheck(q))
        return false;

    return true;
}

bool CopyAllRecommenderFicsToTag(int recommenderId, QString tag)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("insert into recommendations (fic_id, recommender_id, tag) select fic_id, recommender_id, '%1' as tag from recommendations where recommender_id = :id and tag = 'none' ");
    qs=qs.arg(tag);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":id",recommenderId);
    q.bindValue(":tag",tag);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return false;
    }
    return true;
}

QList<int> GetFulLRecommenderList()
{
    QList<int> result;

    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select id from recommenders");
    QSqlQuery q(db);
    q.prepare(qs);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return result;
    }
    while(q.next())
    {
        result.push_back(q.value(0).toInt());
    }
    return result;
}

void WriteRecommenderStatsForTag(AuthorRecommendationStats stats)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("insert into RecommendationTagStats (author_id, fic_count, match_count, match_ratio, tag) "
                         "values(:author_id, :fic_count, :match_count, :match_ratio, :tag)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":author_id",stats.authorId);
    q.bindValue(":fic_count",stats.totalFics);
    q.bindValue(":match_count",stats.matchesWithReferenceTag);
    q.bindValue(":match_ratio",stats.matchRatio);
    q.bindValue(":tag",stats.tag);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
    }
}

QList<AuthorRecommendationStats> GetRecommenderStatsForTag(QString tag, QString sortOn, QString order)
{
    QList<AuthorRecommendationStats> result;

    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select rts.match_count as match_count,"
                         "rts.match_ratio as match_ratio,"
                         "rts.author_id as author_id,"
                         "rts.fic_count as fic_count,"
                         "r.name as name"
                         "  from RecommendationTagStats rts, recommenders r where rts.author_id = r.id and tag = :tag order by %1 %2");
    qs=qs.arg(sortOn).arg(order);
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":tag",tag);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return result;
    }
    while(q.next())
    {
        AuthorRecommendationStats stats;
        stats.isValid = true;
        stats.matchesWithReferenceTag = q.value("match_count").toInt();
        stats.matchRatio= q.value("match_ratio").toDouble();
        stats.authorId= q.value("author_id").toInt();
        stats.tag= tag;
        stats.totalFics= q.value("fic_count").toInt();
        stats.authorName = q.value("name").toString();
        result.push_back(stats);
    }
    return result;
}

bool AssignNewNameForRecommenderId(FavouritesPage recommender)
{
    if(recommender.GetIdStatus() != AuthorIdStatus::valid)
        return true;
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    QString qsl = " UPDATE recommenders SET name = :name where id = :id";
    q.prepare(qsl);
    q.bindValue(":name",recommender.name);
    q.bindValue(":id",recommender.id);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return false;
    }
    return true;
}

QVector<FavouritesPage> GetAllAuthors(QString website)
{
    QVector<FavouritesPage> result;
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select count(id) from recommenders where website_type = :site");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":site",website);
    q.exec();
    q.next();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return result;
    }
    int size = q.value(0).toInt();

    qs = QString("select id, url from recommenders where website_type = :site");
    q.prepare(qs);
    q.bindValue(":site",website);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return result;
    }
    result.reserve(size);
    while(q.next())
    {
        FavouritesPage rec;
        rec.AssignId(q.value("id").toInt());
        rec.url = q.value("url").toString();
        result.push_back(rec);
    }
    return result;
}

bool UpdateTagStatsPerFic(QString tag)
{
    auto allFics = GetAllFicIDsFromRecommendations(tag);

    QString insertStatementPrototype = "INSERT INTO RecommendationFicStats(fic_id,tag, sumrecs) "
            "SELECT %1, '%2', 0 "
            "WHERE NOT EXISTS(SELECT 1 FROM RecommendationFicStats where fic_id = :fic_id AND tag = :tag)";
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    db.transaction();
    qDebug() << "processing tag: " << tag;
    for(auto fic_id: allFics)
    {

        QString qs = insertStatementPrototype.arg(fic_id).arg(tag);
        q.prepare(qs);
        q.bindValue(":fic_id",fic_id);
        q.bindValue(":tag",tag);
        q.exec();
        if(q.lastError().isValid())
        {
            qDebug() << q.lastError();
            qDebug() << q.lastQuery();
        }

        qs = "update RecommendationFicStats set sumrecs = (SELECT  count(recommender_id) FROM recommendations r "
             " where r.fic_id = :fic_id"
             " and r.tag = :tag) "
             " where fic_id = :fic_id and tag = :tag";
        q.prepare(qs);
        q.bindValue(":fic_id",fic_id);
        q.bindValue(":tag",tag);
        q.exec();
        if(q.lastError().isValid())
        {
            qDebug() << q.lastError();
            qDebug() << q.lastQuery();
        }
    }
    db.commit();
    return true;
}

QVector<int> GetAllFicIDsFromRecommendations(QString tag)
{
    QVector<int> result;
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select count(fic_id) from recommendations where tag = :tag");
    QSqlQuery q(db);
    q.prepare(qs);
    q.bindValue(":tag",tag);
    q.exec();
    q.next();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return result;
    }
    int size = q.value(0).toInt();

    qs = QString("select fic_id from recommendations where tag = :tag");
    q.prepare(qs);
    q.bindValue(":tag",tag);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return result;
    }
    result.reserve(size);
    while(q.next())
    {
        result.push_back(q.value("fic_id").toInt());
    }
    return result;
}

int GetFicDBIdByDelimitedSiteId(QString id)
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select id from fanfics where url like '%%1%'");
    qs= qs.arg(id);
    QSqlQuery q(db);
    int result = -1;
    q.prepare(qs);
    q.exec();
    q.next();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return result;
    }
    result = q.value(0).toInt();
    return result;
}

QStringList ObtainIdList(core::Query query)
{
    QString where = query.str;
    QStringList result;
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("select group_concat(id, ',') as merged, " + where);
    qs= qs.arg(where);
    QSqlQuery q(db);
    q.prepare(qs);
    auto it = query.bindings.begin();
    auto end = query.bindings.end();
    while(it != end)
    {
        qDebug() << it.key() << " " << it.value();
        q.bindValue(it.key(), it.value());
        ++it;
    }
    q.exec();

    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
        return result;
    }
    q.next();
    auto temp = q.value("merged").toString();
    result = temp.split(",");
    return result;
}

void EnsureFFNUrlsShort()
{
    QSqlDatabase db = QSqlDatabase::database();
    QString qs = QString("update fanfics set url = cfReturnCapture('(/s/\\d+/)', url)");
    QSqlQuery q(db);
    q.prepare(qs);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
    }
}

void PassTagsIntoTagsTable()
{
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QString qs = QString("select id, tags from fanfics where tags <> ' none ' order by tags");
    QSqlQuery q(db);
    q.prepare(qs);
    q.exec();
    if(q.lastError().isValid())
    {
        qDebug() << q.lastError();
        qDebug() << q.lastQuery();
    }
    while(q.next())
    {
        auto id = q.value("id").toInt();
        auto tags = q.value("tags").toString();
        if(tags.trimmed() == "none")
            continue;
        QStringList split = tags.split(" ", QString::SkipEmptyParts);
        split.removeDuplicates();
        split.removeAll("none");
        qDebug() << split;
        for(auto tag : split)
        {
            qs = QString("insert into fictags(fic_id, tag) values(:fic_id, :tag)");
            QSqlQuery iq(db);
            iq.prepare(qs);
            iq.bindValue(":fic_id",id);
            iq.bindValue(":tag", tag);
            iq.exec();
            if(iq.lastError().isValid())
            {
                qDebug() << iq.lastError();
                qDebug() << iq.lastQuery();
            }
        }
    }
    db.commit();
}





}
