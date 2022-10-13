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
#include "include/web/pageconsumer.h"
#include "include/web/pagegetter.h"
#include "include/transaction.h"
PageConsumer::PageConsumer(QObject* obj):QObject(obj){}


void PageConsumer::CreatePageThreadWorker()
{
    worker.reset(new PageThreadWorker);
    worker->moveToThread(&pageThread);
    connect(worker.data(), &PageThreadWorker::pageResult, this, &PageConsumer::OnNewPage);
}

void PageConsumer::StartPageWorker()
{
    pageQueue.data.clear();
    pageQueue.pending = true;
    pageThread.start(QThread::HighPriority);
}

void PageConsumer::StopPageWorker()
{
    pageThread.quit();
}
void PageConsumer::OnNewPage(PageResult result)
{
    if(result.data.isValid)
        pageQueue.data.push_back(result.data);
    if(result.finished)
        pageQueue.pending = false;
}
