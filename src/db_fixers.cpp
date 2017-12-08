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
        QRegExp rx = QRegExp("/[A-Za-z0-9.\\s]{0,}[^A-Za-z0-9.\\s]");
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

void EnsureFandomIndexExists(QSqlDatabase db)
{
    auto fandoms = database::puresql::GetAllFandomsFromSingleTable(db);
    for(auto fandom: fandoms)
        fandom->name = ConvertName(fandom->name);

    int currentId = 0;
    QMap<QString,int> fandomNameToNewId;
    QHash<int,core::FandomPtr> idToFandom;
    for(auto fandom : fandoms)
        idToFandom[fandom->id] = fandom;

    QHash<int,int> fandomIdToNewId;
    QSet<int> affectedFics;
    for(auto fandom : fandoms)
    {
        if(!fandom)
            continue;
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
    QMap<QString, QList<int>> rebinds;
    for(auto fandom : fandoms)
    {
        rebinds[fandom->name].push_back(fandomIdToNewId[fandom->id]);

    }
    QHash<int, QList<int>> ficfandoms = database::puresql::GetWholeFicFandomsTable(db);
    QHash<int, QList<int>> resultingList;
    for(int fandom : ficfandoms.keys())
        resultingList[fandomIdToNewId[fandom]] += ficfandoms[fandom];

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


        for(auto key: fandomNameToNewId.keys())
            result = result && database::puresql::CreateFandomIndexRecord(fandomNameToNewId[key], key, db);
        if(!result)
            throw std::logic_error("");
        for(auto id: fandomIdToNewId.keys())
        {
            result = result && database::puresql::AddFandomLink(id,fandomIdToNewId[id], db);
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
