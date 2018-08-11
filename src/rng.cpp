#include "include/rng.h"
#include "include/Interfaces/db_interface.h"
#include <functional>
#include <random>
#include <QReadLocker>
#include <QWriteLocker>

namespace core{
QString DefaultRNGgenerator::Get(QSharedPointer<Query> query, QString userToken, QSqlDatabase db)
{
    QString where = userToken + query->str;
    bool containsWhere = false;
    {
        // locking to make sure it's not modified when we search
        QReadLocker locker(&lock);
        containsWhere = randomIdLists.contains(where);

    }
    if(!containsWhere)
    {
        QWriteLocker locker(&lock);
        QStringList idList = portableDBInterface->GetIdListForQuery(query, db);
        if(idList.size() == 0)
            idList.push_back("-1");
        randomIdLists[where] = idList;
    }

    QReadLocker locker(&lock);
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    // at this point we can be sure lists contain current where so readlocker is sufficient
    auto& currentList = randomIdLists[where];
    std::uniform_int_distribution<> distr(0, currentList.size()-1); // define the range
    auto value = distr(eng);
    return currentList[value];
}
}
