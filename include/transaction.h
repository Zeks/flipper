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
#pragma once

#include <QSqlDatabase>
#include <QReadWriteLock>
#include <QSet>

namespace database{
class Transaction{
public:
    Transaction(QSqlDatabase);
    Transaction(const Transaction&) = default;
    ~Transaction();
    bool start();
    bool cancel();
    bool finalize();
    QSqlDatabase db;
    bool isOpen = false;
    static QReadWriteLock lock;
    static QSet<QString> transactionSet;
};

}
