/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
#include <QReadLocker>
#include <QWriteLocker>
#include <QDebug>
QReadWriteLock database::Transaction::lock;
QSet<QString>  database::Transaction::transactionSet;
database::Transaction::Transaction(QSqlDatabase db)
{
    this->db = db;
    QWriteLocker locker(&lock);
    QString connetionName = db.connectionName();
    if(!transactionSet.contains(connetionName))
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
        qDebug() << "deleting transaction";
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
    qDebug() << "cancelling transaction";
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
    qDebug() << "finalizing transaction";
    transactionSet.remove(db.connectionName());
    isOpen = false;
    return true;
}
