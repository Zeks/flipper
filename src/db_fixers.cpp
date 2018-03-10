#include "include/db_fixers.h"
#include "include/pure_sql.h"
#include "include/transaction.h"
#include "include/url_utils.h"
#include <QSet>
#include <QDebug>
#include <QMap>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
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

void FillFFNId(QSqlDatabase db)
{
    database::Transaction transaction(db);
    QString qs = "select id, url from recommenders";
    QSqlQuery q(db);
    q.prepare(qs);
    database::puresql::ExecAndCheck(q);
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
    QSqlQuery fixQ(db);
    fixQ.prepare("update recommenders set ffn_id = :ffn_id where id = :id");
    for(auto id : idToFFNId.keys())
    {
        fixQ.bindValue(":id", id);
        auto ffnId= idToFFNId[id];
        fixQ.bindValue(":ffn_id", ffnId);
        database::puresql::ExecAndCheck(fixQ);
    }
    transaction.finalize();
}


void TrimUserUrls(QSqlDatabase db)
{
    database::puresql::DiagnosticSQLResult<bool> result;
    database::Transaction transaction(db);
    QString qs = "select rowid, url from pagecache";
    QSqlQuery q(db);
    q.prepare(qs);
    q.exec();
    result.CheckDataAvailability(q);
    if(!result.success)
        return;

    QSqlQuery fixQ(db);
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


void ReplaceUrlInLinkedAuthorsWithID(QSqlDatabase db)
{
    database::Transaction transaction(db);
    QString qs = "select rowid, recommender_id, url from LinkedAuthors";
    QSqlQuery q(db);
    q.prepare(qs);
    database::puresql::ExecAndCheck(q);
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
    QSqlQuery fixQ(db);
    fixQ.prepare("update LinkedAuthors set ffn_id = :ffn_id where rowid = :row_id");
    for(auto id : idToFFNId.keys())
    {
        fixQ.bindValue(":row_id", id);
        auto ffnId= idToFFNId[id];
        fixQ.bindValue(":ffn_id", ffnId);
        database::puresql::ExecAndCheck(fixQ);
    }
    transaction.finalize();
}

void RebindDuplicateFandoms(QSqlDatabase db)
{
    auto fandoms = database::puresql::GetAllFandoms(db);
    for(auto fandom: fandoms)
        fandom->SetName(core::Fandom::ConvertName(fandom->GetName()));

    int currentId = 0;
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

    QHash<int, QList<int>> ficfandoms = database::puresql::GetWholeFicFandomsTable(db);
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
        result = result && database::puresql::EraseFicFandomsTable(db);
        for(auto key : resultingList.keys())
        {
            auto list = resultingList[key];
            auto last = std::unique(list.begin(), list.end());
            list.erase(last, list.end());
            for(auto id : list)
                result = result && database::puresql::AddFandomForFic(id, key, db);
        }
        if(!result)
            throw std::logic_error("");


//        for(auto key: fandomNameToNewId.keys())
//            result = result && database::puresql::CreateFandomIndexRecord(fandomNameToNewId[key], key, db);
//        if(!result)
//            throw std::logic_error("");
        for(auto id: fandomIdToNewId.keys())
        {

            database::puresql::DiagnosticSQLResult<bool> diag;
            for(auto url : idToFandom[id]->GetUrls())
            {
                if(url.GetUrl().isEmpty())
                {
                    auto fandom = idToFandom[id];
                    continue;
                }
                diag = database::puresql::AddUrlToFandom(fandomIdToNewId[id], url, db);
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
            database::puresql::DeleteFandom(id, db);
    }

    transaction.finalize();
    database::Transaction transaction1(db);
}

}
