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
}
namespace database{
class IDBWrapper;
}
class PageTask;
typedef QSharedPointer<PageTask> PageTaskPtr;


class AuthorLoadProcessor: public PageConsumer{
Q_OBJECT
public:
    AuthorLoadProcessor(QSqlDatabase db,
                        QSqlDatabase taskDb,
                        QSharedPointer<interfaces::Fanfics> fanficInterface,
                        QSharedPointer<interfaces::Fandoms> fandomsInterface,
                        QSharedPointer<interfaces::Authors> authorsInterface,
                        QSharedPointer<interfaces::PageTask> pageTaskInterface,
                        QObject* obj = nullptr);
    void Run(PageTaskPtr task);
    QSharedPointer<database::IDBWrapper> dbInterface;
private:
    QSqlDatabase db;
    QSqlDatabase taskDB;
    QSharedPointer<interfaces::Fanfics> fanfics;
    QSharedPointer<interfaces::Fandoms> fandoms;
    QSharedPointer<interfaces::Authors> authors;
    QSharedPointer<interfaces::PageTask> pageInterface;

    bool cancelCurrentTaskPressed = false;
public slots:
    void OnCancelCurrentTask();
signals:
    // page task when iterating through favourites pages
    // contains urls from a SUBtask
    void pageTaskList(QStringList, ECacheMode, int delay);
    void requestProgressbar(int);
    void updateCounter(int);
    void updateInfo(QString);
    void resetEditorText();
};
class FavouriteStoryParser;
void WriteProcessedFavourites(FavouriteStoryParser& parser,
                              core::AuthorPtr author,
                              QSharedPointer<interfaces::Fanfics> fanficsInterface,
                              QSharedPointer<interfaces::Authors> authorsInterface,
                              QSharedPointer<interfaces::Fandoms> fandomsInterface);
