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
#include "tasks/fandom_list_reload_processor.h"
#include "include/pagegetter.h"
#include "include/pagetask.h"
#include "include/parsers/ffn/fandomparser.h"
#include "include/parsers/ffn/fandomindexparser.h"
#include "include/transaction.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/fandoms.h"
#include "include/Interfaces/pagetask_interface.h"
#include "include/url_utils.h"
#include "include/timeutils.h"

#include <QThread>

FandomListReloadProcessor::FandomListReloadProcessor(QSqlDatabase db,
                                         QSharedPointer<interfaces::Fanfics> fanficInterface,
                                         QSharedPointer<interfaces::Fandoms> fandomsInterface,
                                         QSharedPointer<interfaces::PageTask> pageInterface,
                                         QSharedPointer<database::IDBWrapper> dbInterface,
                                         QObject *obj) : QObject(obj)
{
    this->fanficsInterface = fanficInterface;
    this->fandomsInterface = fandomsInterface;
    this->pageInterface = pageInterface;
    this->dbInterface = dbInterface;
    this->db = db;
}

FandomListReloadProcessor::~FandomListReloadProcessor()
{

}

void FandomListReloadProcessor::UpdateFandomList()
{
    emit updateInfo("Started fandom update task.<br>");
    FFNFandomParser fandomParser;
    FFNCrossoverFandomParser crossoverParser;
    QStringList categoryList = {"anime", "book", "cartoon", "comic", "game", "misc", "movie", "play", "tv"};
    for(const auto& cat : categoryList)
        UpdateCategory("/" + cat, &fandomParser, fandomsInterface);
    for(const auto& cat : std::as_const(categoryList))
        UpdateCategory("/crossovers/" + cat + "/", &crossoverParser, fandomsInterface);

}

void FandomListReloadProcessor::UpdateCategory(QString cat,
                                FFNFandomIndexParserBase* parser,
                                QSharedPointer<interfaces::Fandoms> fandomInterface)
{
    An<PageManager> pager;
    QString link = "https://www.fanfiction.net" + cat;
    emit updateInfo("Processing: " + link + "<br>");
    WebPage result = pager->GetPage(link, ECacheMode::dont_use_cache);
    database::Transaction transaction(fandomInterface->db);
    if(result.isValid)
    {
        parser->SetPage(result);
        parser->Process();
        if(!parser->HadErrors())
        {
            for(auto fandom : std::as_const(parser->results))
            {
                fandom->section = cat;
                bool exists = fandomInterface->EnsureFandom(fandom->GetName());
                if(!exists)
                    fandomInterface->CreateFandom(fandom);
                const auto urls = fandom->GetUrls();
                for(auto url : urls)
                {
                    url.SetType(cat);
                    fandomInterface->AddFandomLink(fandom->GetName(), url);
                }
            }
        }
        else
        {
            QString pageMissingPrototype = "Page format unexpected: %1\nNew fandoms from it will be unavailable until the next succeesful reload";
            pageMissingPrototype=pageMissingPrototype.arg(link);
            emit displayWarning(pageMissingPrototype);
            transaction.cancel();
            return;
        }

    }
    else
    {
        QString pageMissingPrototype = "Could not load page: %1\nNew fandoms from it will be unavailable until the next succeesful reload";
        pageMissingPrototype=pageMissingPrototype.arg(link);
        emit displayWarning(pageMissingPrototype);
        transaction.cancel();
        return;
    }
    transaction.finalize();

}
