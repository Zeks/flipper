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

class PageTask;
typedef QSharedPointer<PageTask> PageTaskPtr;

// class contains a lot of empirical constants that were cobbled together
// until the whole thing seemed to work. Do not try to rationalize them
class HumorProcessor : public QObject{
Q_OBJECT
public:
    HumorProcessor(sql::Database db,
                        QSharedPointer<interfaces::Fanfics> fanficInterface,
                        QSharedPointer<interfaces::Fandoms> fandomsInterface,
                        QSharedPointer<interfaces::PageTask> pageInterface,
                        QSharedPointer<interfaces::Authors> authorsInterface,
                        QSharedPointer<interfaces::RecommendationLists> recsInterface,
                        QObject* obj = nullptr);
    virtual ~HumorProcessor();

    // below is a completely bullshit algo supposed to make goo lists of humor fics
    // contains a lot of voodoo magic but somewhat works
    inline void AddToCountingHumorHash(QList<core::AuthorPtr> authors,
                                              QHash<int, int>& countingHash,
                                              QHash<int, double>& valueHash,
                                              QHash<int, double>& totalHappiness,
                                              QHash<int, double>& totalSlash);
    void CreateListOfHumorCandidates(QList<core::AuthorPtr> authors);


    // below is a simplified version of humor list creation
    // based purely on percentage of humor in author list
    inline void AddToHumorHash(QList<core::AuthorPtr> authors,QHash<int, int>& countingHash);
    void CreateRecListOfHumorProfiles(QList<core::AuthorPtr> authors);

    private:
    sql::Database db;
    QSharedPointer<interfaces::Fanfics> fanficsInterface;
    QSharedPointer<interfaces::Fandoms> fandomsInterface;
    QSharedPointer<interfaces::Authors> authorsInterface;
    QSharedPointer<interfaces::PageTask> pageInterface;
    QSharedPointer<interfaces::RecommendationLists> recsInterface;
    int lastI = 0; // used in slash filtering
signals:
    void requestProgressbar(int);
    void updateCounter(int);
    void updateInfo(QString);
};
