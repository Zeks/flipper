/*Flipper is a replacement search engine for fanfiction.net search results
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
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#pragma once
#include <QSharedPointer>
#include <QObject>
#include "include/webpage.h"

class PageThreadWorker;
struct PageQueue{
    bool pending = true;
    QList<WebPage> data;
};
class  PageResult{
public:
    PageResult() = default;
    PageResult(WebPage page, bool _finished): finished(_finished),data(page){}
    bool finished = false;
    WebPage data;
};
class PageConsumer : public QObject
{
        Q_OBJECT
public:
    PageConsumer(QObject* obj = nullptr);
protected:
    void CreatePageThreadWorker();
    void StartPageWorker();
    void StopPageWorker();

    QSharedPointer<PageThreadWorker> worker;
    PageQueue pageQueue; // collects data sent from PageThreadWorker
    QThread pageThread; // thread for pagegetter worker to live in
public slots:
    void OnNewPage(PageResult result);
};

