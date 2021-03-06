/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

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
#include "transaction.h"
#include "logger/QsLog.h"
#include <QReadLocker>
#include <QWriteLocker>
#include <QDebug>
QReadWriteLock database::Transaction::lock;
QSet<std::string>  database::Transaction::transactionSet;
database::Transaction::Transaction(sql::Database db)
{
    this->db = db;
    QWriteLocker locker(&lock);
    auto connectionName = db.connectionName();
    if(!transactionSet.contains(connectionName))
    {
        //qDebug() << "opening transaction";
        start();
        transactionSet.insert(db.connectionName());
    }

}

database::Transaction::~Transaction()
{
    if(isOpen)
    {
        QLOG_TRACE() << "deleting transaction";
        db.rollback();
        QWriteLocker locker(&lock);
        transactionSet.remove(db.connectionName());
    }
}

bool database::Transaction::start()
{
    if(!db.isOpen())
        return false;
    isOpen = db.transaction();
    return isOpen;
}

bool database::Transaction::cancel()
{
    if(!db.isOpen())
        return false;
    if(!db.rollback())
        return false;
    QLOG_ERROR() << "cancelling transaction";
    transactionSet.remove(db.connectionName());
    isOpen = false;
    return true;
}

bool database::Transaction::finalize()
{
    if(!isOpen)
        return true;
    if(!db.isOpen())
        return false;
    if(!db.commit())
        return false;
    //QLOG_TRACE() << "finalizing transaction";
    transactionSet.remove(db.connectionName());
    isOpen = false;
    return true;
}
