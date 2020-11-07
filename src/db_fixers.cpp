/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#include "include/db_fixers.h"
#include "include/pure_sql.h"
#include "include/transaction.h"
#include "include/url_utils.h"
#include <QSet>
#include <QDebug>
#include <QMap>
#include "sql_abstractions/sql_query.h"
#include <QSqlError>
#include "sql_abstractions/sql_database.h"
#include <QRegularExpression>

namespace dbfix{

QString ConvertName(QString name)
{
    static QHash<QString, QString> cache;
    QString result;
    if(cache.contains(name))
        result = cache[name];
    else
    {
        QRegExp rx = QRegExp("(/(.|\\s){0,}[^\\x0000-\\x007F])|(/(.|\\s){0,}[?][?][?])");
        rx.setMinimal(true);
        int index = name.indexOf(rx);
        if(index != -1)
            cache[name] = name.left(index).trimmed();
        else
            cache[name] = name.trimmed();
        result = cache[name];
    }
    return result;
}

void FillFFNId(sql::Database db)
{
    database::Transaction transaction(db);
    QString qs = "select id, url from recommenders";
    sql::Query q(db);
    q.prepare(qs);
    sql::ExecAndCheck(q);
    QHash<int, int> idToFFNId;
    while(q.next())
    {
        QString url = q.value("url").toString();
        QRegExp rxEnd("(/u/(\\d+)/)(.*)");
        rxEnd.setMinimal(true);
        rxEnd.indexIn(url);
        auto idPart = rxEnd.cap(2);
        qDebug() << idPart;
        bool ok = true;
        idToFFNId[q.value("id").toInt()] = idPart.toInt(&ok);
        if(!ok)
            qDebug() << "cannot convert value to int";


    }
    sql::Query fixQ(db);
    fixQ.prepare("update recommenders set ffn_id = :ffn_id where id = :id");
    for(auto id : idToFFNId.keys())
    {
        fixQ.bindValue(":id", id);
        auto ffnId= idToFFNId[id];
        fixQ.bindValue(":ffn_id", ffnId);
        sql::ExecAndCheck(fixQ);
    }
    transaction.finalize();
}


void TrimUserUrls(sql::Database db)
{
    sql::DiagnosticSQLResult<bool> result;
    database::Transaction transaction(db);
    QString qs = "select rowid, url from pagecache";
    sql::Query q(db);
    q.prepare(qs);
    q.exec();
    result.CheckDataAvailability(q);
    if(!result.success)
        return;

    sql::Query fixQ(db);
    fixQ.prepare("update pagecache set url = :url where rowid = :row_id");
    //QRegularExpression rxEnd("(/u/(\\d+)/)(.*)", QRegularExpression::InvertedGreedinessOption);
    bool success = true;
    while(q.next())
    {
        int rowid = q.value("rowid").toInt();
        QString originalurl = q.value("url").toString();
        QString url = originalurl;

        if(!url.contains("/u/"))
            continue;
        auto id = url_utils::GetWebId(originalurl, "ffn");
        bool ok = true;
        id.toInt(&ok);
        if(!ok)
            continue;

        url = QString("https://www.fanfiction.net/u/") + id;

        fixQ.bindValue(":row_id", rowid);
        fixQ.bindValue(":url", url);
        if(!result.ExecAndCheck(fixQ, true))
        {
            success = false;
            break;
        }
    }
    if(success)
        transaction.finalize();
    else
        transaction.cancel();
}


sql::DiagnosticSQLResult<bool> PassSlashDataIntoNewTable(sql::Database db)
{
    sql::DiagnosticSQLResult<bool> result;

    database::Transaction transaction(db);
    // first we wipe the original slash table
    QString qs = QString("delete from slash_data");
    sql::Query q(db);
    q.prepare(qs);
    if(!result.ExecAndCheck(q))
        return result;

    QString insertQS = QString("insert into slash_data(fic_id, keywords_yes, keywords_no, keywords_result, first_iteration, second_iteration)"
                               "  values(:fic_id, :keywords_yes, :keywords_no, :keywords_result, :first_iteration, :second_iteration)");
    sql::Query insertQ(db);
    insertQ.prepare(insertQS);

    qs = QString("select id, slash_keywords, not_slash_keywords,slash_keywords_result, first_slash_iteration, second_slash_iteration from fanfics ");
    if(!db.isOpen())
        qDebug() << "not open";
    sql::Query importTagsQ(db);
    importTagsQ.prepare(qs);
    if(!result.ExecAndCheck(importTagsQ))
        return result;

    while(importTagsQ.next())
    {
        bool slashresult = importTagsQ.value("slash_keywords").toBool() && !importTagsQ.value("not_slash_keywords").toBool();
        insertQ.bindValue(":fic_id", importTagsQ.value("id").toInt());
        insertQ.bindValue(":keywords_yes", importTagsQ.value("slash_keywords").toInt());
        insertQ.bindValue(":keywords_no", importTagsQ.value("not_slash_keywords").toInt());
        insertQ.bindValue(":keywords_result",  slashresult);
        insertQ.bindValue(":first_iteration", importTagsQ.value("first_slash_iteration").toInt());
        insertQ.bindValue(":second_iteration", importTagsQ.value("second_slash_iteration").toInt());

        if(!result.ExecAndCheck(insertQ))
            return result;
    }
    transaction.finalize();
    result.data = true;
    return result;
}


void ReplaceUrlInLinkedAuthorsWithID(sql::Database db)
{
    database::Transaction transaction(db);
    QString qs = "select rowid, recommender_id, url from LinkedAuthors";
    sql::Query q(db);
    q.prepare(qs);
    sql::ExecAndCheck(q);
    QHash<int, int> idToFFNId;
    while(q.next())
    {
        QString url = q.value("url").toString();
        QRegExp rxEnd("(/u/(\\d+)/)(.*)");
        rxEnd.setMinimal(true);
        rxEnd.indexIn(url);
        auto idPart = rxEnd.cap(2);
        qDebug() << idPart;
        bool ok = true;
        idToFFNId[q.value("rowid").toInt()] = idPart.toInt(&ok);
        if(!ok)
            qDebug() << "cannot convert value to int";


    }
    sql::Query fixQ(db);
    fixQ.prepare("update LinkedAuthors set ffn_id = :ffn_id where rowid = :row_id");
    for(auto id : idToFFNId.keys())
    {
        fixQ.bindValue(":row_id", id);
        auto ffnId= idToFFNId[id];
        fixQ.bindValue(":ffn_id", ffnId);
        sql::ExecAndCheck(fixQ);
    }
    transaction.finalize();
}

bool RebindDuplicateFandoms(sql::Database db)
{
    auto result = sql::GetAllFandoms(db);
    if(!result.success)
        return false;
    auto fandoms = result.data;
    for(auto fandom: fandoms)
        fandom->SetName(core::Fandom::ConvertName(fandom->GetName()));

    QMap<QString,int> fandomNameToNewId;
    QHash<int,core::FandomPtr> idToFandom;
    for(auto fandom : fandoms)
    {
        if(!fandom)
            continue;
        idToFandom[fandom->id] = fandom;
    }

    QHash<int,int> fandomIdToNewId;
    QSet<int> affectedFics;
    QSet<int> trackedNewFandomIds;
    QSet<int> trackedOldFandomIds;
    QHash<int,QStringList> fandomChain;
    for(auto fandom : fandoms)
    {
        if(!fandom)
            continue;
        auto convertedName = fandom->GetName();
        if(!fandomNameToNewId.contains(fandom->GetName()))
        {
            int id = fandom->id;
            fandomNameToNewId[fandom->GetName()] = id;
            fandomIdToNewId[fandom->id] = id;
        }
        else
        {
            fandomIdToNewId[fandom->id] = fandomNameToNewId[fandom->GetName()];
            fandomChain[fandomIdToNewId[fandom->id]].push_back(QString::number(fandom->id));
        }
    }
    QMap<QString, QList<int>> rebinds;
    for(auto fandom : fandoms)
    {
        auto newFandomId = fandomIdToNewId[fandom->id];
        rebinds[fandom->GetName()].push_back(newFandomId);
    }

    auto ficfandomsResult = sql::GetWholeFicFandomsTable(db);
    QHash<int, QList<int>> ficfandoms  = ficfandomsResult.data;
    if(!ficfandomsResult.success)
        return false;

    QHash<int, QList<int>> resultingList;
    for(int fandom : ficfandoms.keys())
        resultingList[fandomIdToNewId[fandom]] += ficfandoms[fandom];

    for(auto id: fandomIdToNewId.keys())
    {
        if(id != fandomIdToNewId[id])
        {
            if(!idToFandom.contains(id))
            {
                qDebug() << "no fandom in index:" << id;
            }
            if(!idToFandom[id])
                qDebug() << "null fandom:" << id;
            qDebug() << "deleting fandom record:" << id << idToFandom[id]->GetName();
            auto newId = fandomIdToNewId[id];
            if(!idToFandom.contains(newId))
                qDebug() << "null fandom:" << id;
            qDebug() << "because it is now:" << fandomIdToNewId[id] << idToFandom[newId]->GetName();
            qDebug().noquote() << "chain:" << newId << "," << fandomChain[newId].join(",");
        }
    }

    database::Transaction transaction(db);
    try {

        bool result = true;
        result = result && sql::EraseFicFandomsTable(db).success;
        for(auto key : resultingList.keys())
        {
            auto list = resultingList[key];
            auto last = std::unique(list.begin(), list.end());
            list.erase(last, list.end());
            for(auto id : list)
                result = result && sql::AddFandomForFic(id, key, db).success;
        }
        if(!result)
            throw std::logic_error("");


//        for(auto key: fandomNameToNewId.keys())
//            result = result && sql::CreateFandomIndexRecord(fandomNameToNewId[key], key, db);
//        if(!result)
//            throw std::logic_error("");
        for(auto id: fandomIdToNewId.keys())
        {

            sql::DiagnosticSQLResult<bool> diag;
            for(auto url : idToFandom[id]->GetUrls())
            {
                if(url.GetUrl().isEmpty())
                {
                    auto fandom = idToFandom[id];
                    continue;
                }
                diag = sql::AddUrlToFandom(fandomIdToNewId[id], url, db);
                result = result && diag.success;
            }
        }
        if(!result)
            throw std::logic_error("");
    }
    catch (const std::logic_error&) {
        transaction.cancel();
    }
    // need to delete all the old fandom records
    for(auto id: fandomIdToNewId.keys())
    {
        if(id != fandomIdToNewId[id])
            sql::DeleteFandom(id, db);
    }

    transaction.finalize();
    database::Transaction transaction1(db);
    return true;
}

}
