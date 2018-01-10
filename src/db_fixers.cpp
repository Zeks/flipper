#include "include/db_fixers.h"
#include "include/pure_sql.h"
#include "include/transaction.h"
#include <QSet>
#include <QDebug>
#include <QMap>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>

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

//void EnsureFandomIndexExists(QSqlDatabase db)
//{
//    auto fandoms = database::puresql::GetAllFandomsFromSingleTable(db);
//    for(auto fandom: fandoms)
//        fandom->SetName(ConvertName(fandom->GetName()));

//    int currentId = 0;
//    QMap<QString,int> fandomNameToNewId;
//    QHash<int,core::FandomPtr> idToFandom;
//    for(auto fandom : fandoms)
//        idToFandom[fandom->id] = fandom;

//    QHash<int,int> fandomIdToNewId;
//    QSet<int> affectedFics;
//    QSet<int> trackedNewFandomIds;
//    QSet<int> trackedOldFandomIds;
//    QSet<QString> trackedFandomNames = {"railgun", "Alex Rider", "Diary of a Wimpy Kid"};
//    for(auto fandom : fandoms)
//    {
//        if(!fandom)
//            continue;
//        auto convertedName = fandom->GetName();
//        if(!fandomNameToNewId.contains(fandom->GetName()))
//        {
//            int id = currentId++;
//            fandomNameToNewId[fandom->GetName()] = id;
//            fandomIdToNewId[fandom->id] = id;
//        }
//        else
//            fandomIdToNewId[fandom->id] = fandomNameToNewId[fandom->GetName()];
//        if(convertedName.contains("railgun", Qt::CaseInsensitive))
//        {
//            //qDebug() << "railgun is converted to: " << fandomIdToNewId[fandom->id];
//            trackedNewFandomIds.insert(fandomIdToNewId[fandom->id]);
//            trackedOldFandomIds.insert(fandom->id);
//        }
//        if(convertedName.contains("Alex Rider", Qt::CaseInsensitive))
//        {
//            //qDebug() << "Alex rider is converted to: " << fandomIdToNewId[fandom->id];
//            trackedNewFandomIds.insert(fandomIdToNewId[fandom->id]);
//            trackedOldFandomIds.insert(fandom->id);
//        }
//        if(convertedName.contains("Diary of a Wimpy Kid", Qt::CaseInsensitive))
//        {
//            trackedNewFandomIds.insert(fandomIdToNewId[fandom->id]);
//            trackedOldFandomIds.insert(fandom->id);
////            qDebug() << "Wimpy was" << fandom->id;
////            qDebug() << "Wimpy is converted to: " << fandomIdToNewId[fandom->id];
//        }
//        //fandom->name = convertedName;
//    }
//    QMap<QString, QList<int>> rebinds;
//    for(auto fandom : fandoms)
//    {
//        //auto convertedName = ConvertName(fandom->name);
//        auto newFandomId = fandomIdToNewId[fandom->id];
////        if(trackedOldFandomIds.contains(fandom->id))
////            qDebug() << "pushing fandom" <<  fandom->name << "to" << newFandomId;
//        rebinds[fandom->GetName()].push_back(newFandomId);
//    }

//    QHash<int, QList<int>> ficfandoms = database::puresql::GetWholeFicFandomsTable(db);
//    QHash<int, QList<int>> resultingList;
//    for(int fandom : ficfandoms.keys())
//    {
////        if(idToFandom.contains(fandom))
////        {
////            auto fandomPtr = idToFandom[fandom];
////            if(trackedOldFandomIds.contains(fandomPtr->id))
////                qDebug() << "registering ficlist: " << fandomPtr->name << ":" << ficfandoms[fandom];
//        //}
////        else if(ficfandoms[fandom].contains(1189773))
////            qDebug() << "fandom id not found in fandom list" << fandom;
//        resultingList[fandomIdToNewId[fandom]] += ficfandoms[fandom];
//    }

//    database::Transaction transaction(db);
//    try {

//        bool result = true;
//        result = result && database::puresql::EraseFicFandomsTable(db);
//        for(auto key : resultingList.keys())
//        {
//            auto list = resultingList[key];
//            auto last = std::unique(list.begin(), list.end());
//            list.erase(last, list.end());
//            for(auto id : list)
//                result = result && database::puresql::AddFandomForFic(id, key, db);
//        }
//        if(!result)
//            throw std::logic_error("");


//        for(auto key: fandomNameToNewId.keys())
//            result = result && database::puresql::CreateFandomIndexRecord(fandomNameToNewId[key], key, db);
//        if(!result)
//            throw std::logic_error("");
//        for(auto id: fandomIdToNewId.keys())
//        {
//            result = result && database::puresql::AddFandomLink(id,fandomIdToNewId[id], db);
//        }
//        if(!result)
//            throw std::logic_error("");
//    }
//    catch (const std::logic_error&) {
//        transaction.cancel();
//    }
//    transaction.finalize();
//    database::Transaction transaction1(db);
//}

}
