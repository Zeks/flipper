#include "include/db_fixers.h"
#include "include/pure_sql.h"
#include "include/transaction.h"
#include <QSet>
#include <QDebug>
#include <QMap>

namespace dbfix{

QString ConvertName(QString name)
{
    static QHash<QString, QString> cache;
    QString result;
    if(cache.contains(name))
        result = cache[name];
    else
    {
        QRegExp rx = QRegExp("/[A-Za-z0-9.]{0,}[^A-Z0-9.]");
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

void EnsureFandomIndexExists(QSqlDatabase db, bool forcedReload)
{
    // get the state of curent tables.
    // if fandomindex is empty load all the fandoms locally and merge the links into single table
    // for every fandom that has ???? atatmpt to cut the part prior to / and amtch it to something
    // track the list of merged fandoms and to wchi they ahave been merged
    // reassign ficfandoms of merged ones to merged fandoms
    // recalculate fandom statistics

    auto fandoms = database::puresql::GetAllFandomsFromSingleTable(db);
    for(auto fandom: fandoms)
        fandom->name = ConvertName(fandom->name);
    //QString badToken = "????";
    //I need to sort fandoms in such a way that every fandom containing ???? goes after its proper counterpart
//    std::sort(std::begin(fandoms),std::end(fandoms),[badToken](core::FandomPtr f1, core::FandomPtr f2) -> bool {

//        bool f1BadToken = f1->name.contains(QRegExp());
//        bool f2BadToken = f2->name.contains(badToken);
//        if(!f1BadToken && f2BadToken)
//            return true;
//        if(f1BadToken && !f2BadToken)
//            return false;
//        if(f1BadToken && f2BadToken)
//            return f1->name < f2->name;
//        if(f1BadToken || f2BadToken)
//            return f1BadToken < f2BadToken;
//        return f1->name < f2->name;
//    });

    int currentId = 0;
    QMap<QString,int> fandomNameToNewId;
    //qDebug() << fandomNameToNewId.keys();
    //QHash<int,core::FandomPtr> idToFandom;
    QHash<int,int> fandomIdToNewId;
    QSet<int> affectedFics;
//    for(auto fandom : fandoms)
//    {
//        if(fandom)
//            qDebug() << fandom->name;
//    }

    for(auto fandom : fandoms)
    {
        if(!fandom)
            continue;
        //idToFandom[fandom->id] = fandom;
        auto convertedName = ConvertName(fandom->name);
        if(!fandomNameToNewId.contains(fandom->name))
        {
            int id = currentId++;
            fandomNameToNewId[fandom->name] = id;
            fandomIdToNewId[fandom->id] = id;
        }
        else
            fandomIdToNewId[fandom->id] = fandomNameToNewId[fandom->name];
    }
//    for(auto fandom : fandomNameToNewId.keys())
//        qDebug() << fandom;

    database::Transaction transaction(db);
    // create records for each id = newId in the index table
    bool result = true;
    try {
        for(auto key: fandomNameToNewId.keys())
            result = result && database::puresql::CreateFandomIndexRecord(fandomNameToNewId[key], key, db);
        if(!result)
            throw std::logic_error("");
        for(auto id: fandomIdToNewId.keys())
        {
            result = result && database::puresql::AddFandomLink(id,fandomIdToNewId[id], db);
            result = result && database::puresql::RebindFicsToIndex(id,fandomIdToNewId[id], db);
            //qDebug() << " rebinding  " << id << " to: " << fandomIdToNewId[id];
        }
        if(!result)
            throw std::logic_error("");
    }
     catch (const std::logic_error&) {
        transaction.cancel();
    }
    transaction.finalize();
    database::Transaction transaction1(db);
}

}
