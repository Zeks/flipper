/*Flipper is a recommendation and search engine for fanfiction.net
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
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#pragma once
#include <QObject>
#include <QDate>
#include <QSharedPointer>
#include "ECacheMode.h"
#include "include/pageconsumer.h"
#include "include/core/section.h"

namespace interfaces{
class Fanfics;
class Fandoms;
class Authors;
class PageTask;
class RecommendationLists;
}
namespace database{
class IDBWrapper;
}
class PageTask;
typedef QSharedPointer<PageTask> PageTaskPtr;

// class contains a lot of empirical constants that were cobbled together
// until the whole thing seemed to work. Do not try to rationalize them
class SlashProcessor : public QObject{
Q_OBJECT
public:
    SlashProcessor(sql::Database db,
                        QSharedPointer<interfaces::Fanfics> fanficInterface,
                        QSharedPointer<interfaces::Fandoms> fandomsInterface,
                        QSharedPointer<interfaces::Authors> authorsInterface,
                        QSharedPointer<interfaces::RecommendationLists> recsInterface,
                        QSharedPointer<database::IDBWrapper> dbInterface,
                        QObject* obj = nullptr);
    virtual ~SlashProcessor();
    inline void AddToSlashHash(QList<core::AuthorPtr> authors,
                               QSet<int> knownSlashFics,
                               QHash<int, int>& slashHash, bool checkRx = true);
    void CreateListOfSlashCandidates(double neededNotslashMatchesCoeff, QList<core::AuthorPtr> authors);
    void DoFullCycle(sql::Database db, int passCount);
    void AssignSlashKeywordsMetaInfomation(sql::Database db);

    private:
    sql::Database db;
    QSharedPointer<interfaces::Fanfics> fanficsInterface;
    QSharedPointer<interfaces::Fandoms> fandomsInterface;
    QSharedPointer<interfaces::Authors> authorsInterface;
    QSharedPointer<interfaces::PageTask> pageInterface;
    QSharedPointer<interfaces::RecommendationLists> recsInterface;
    QSharedPointer<database::IDBWrapper> dbInterface;
    int lastI = 0; // used in slash filtering
signals:
    void requestProgressbar(int);
    void updateCounter(int);
    void updateInfo(QString);
};
