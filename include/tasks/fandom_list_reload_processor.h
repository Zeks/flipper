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
#include "include/web/pageconsumer.h"
#include "include/core/section.h"

namespace interfaces{
class Fanfics;
class Fandoms;
class PageTask;
}

class PageTask;
typedef QSharedPointer<PageTask> PageTaskPtr;
class FFNFandomIndexParserBase;

class FandomListReloadProcessor: public QObject{
    Q_OBJECT
public:
    FandomListReloadProcessor(sql::Database db,
                              QSharedPointer<interfaces::Fanfics> fanficInterface,
                              QSharedPointer<interfaces::Fandoms> fandomsInterface,
                              QSharedPointer<interfaces::PageTask> pageInterface,
                              QObject* obj = nullptr);
    virtual ~FandomListReloadProcessor();
    void UpdateFandomList();
    void UpdateCategory(QString cat,
                        FFNFandomIndexParserBase* parser,
                        QSharedPointer<interfaces::Fandoms> fandomInterface);


private:
    sql::Database db;
    QSharedPointer<interfaces::Fanfics> fanficsInterface;
    QSharedPointer<interfaces::Fandoms> fandomsInterface;
    QSharedPointer<interfaces::PageTask> pageInterface;

signals:
    void requestProgressbar(int);
    void updateCounter(int);
    void updateInfo(QString);
    void displayWarning(QString);
};
