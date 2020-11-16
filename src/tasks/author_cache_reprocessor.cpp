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
#include "tasks/author_cache_reprocessor.h"
#include "include/pagegetter.h"
#include "include/pagetask.h"
#include "include/parsers/ffn/fandomparser.h"
#include "include/parsers/ffn/desktop_favparser.h"
#include "include/transaction.h"
#include "include/Interfaces/fanfics.h"
#include "include/Interfaces/fandoms.h"
#include "include/Interfaces/authors.h"
#include "include/Interfaces/pagetask_interface.h"
#include "include/url_utils.h"
#include "include/page_utils.h"
#include "include/timeutils.h"
#include "include/statistics_utils.h"

#include <QThread>
#include <QtConcurrent>
#include <QSqlQuery>

AuthorCacheReprocessor::AuthorCacheReprocessor(QSqlDatabase db,
                                           QSharedPointer<interfaces::Fanfics> fanficInterface,
                                           QSharedPointer<interfaces::Fandoms> fandomsInterface,
                                           QSharedPointer<interfaces::Authors> authorsInterface,
                                           QObject *obj): PageConsumer(obj)
{
    this->fanficsInterface = fanficInterface;
    this->fandomsInterface = fandomsInterface;
    this->authorsInterface = authorsInterface;

    this->db = db;
//    QSqlQuery q("PRAGMA synchronous = OFF", db);
//    bool result = q.exec();
//    qDebug() << "synchro result: " << result;
    CreatePageThreadWorker();
}

struct ParsedAuthorData
{
    QSharedPointer<FavouriteStoryParser> parser;
    void Log() {
        qDebug() << "Sink accepting: "  << parser->recommender.author->GetWebID("ffn");
    }
};

static core::AuthorPtr CreateAuthorFromNameAndUrl(QString name, QString url)
{
    core::AuthorPtr author(new core::Author);
    author->name = name;
    author->SetWebID("ffn", url_utils::GetWebId(url, "ffn").toInt());
    return author;
}

void AuthorCacheReprocessor::ReprocessAllAuthorsStats()
{

        auto authors = authorsInterface->GetAllUnprocessedLinkedAuthors();
        std::sort(authors.begin(), authors.end());
        statistics_utils::UserPageSource source;
        statistics_utils::UserPageSink<ParsedAuthorData> sink;

        //QHash<int, QList<WebPage>>pages;
        int counter = 1;

        int chunkSize = 500;
        source.Reserve(chunkSize);
        qDebug() << "loading pages";
        int authorsSize = authors.size();
        qDebug() << "authors size: " << authorsSize;
        for(auto author: authors)
        {

            if(!author)
                continue;

            auto page = env::RequestPage("https://www.fanfiction.net/u/" + QString::number(author), ECacheMode::use_only_cache);
            if(!page.isValid)
            {
                //qDebug() << page.url << " is invalid";
            }
            else
            {
                counter++;
                source.Push(page);
            }
            if(counter%(chunkSize - 1) == 0 || counter == authorsSize)
            {
                database::Transaction transaction(db);
                source.initialSize = source.pages.size();
                qDebug() << "starting to process pack: " << counter/1000 << " of size: " << source.GetInitialSize();


                int threadCount = 12;
                auto fanficsInterface = this->fanficsInterface;
                auto authorsInterface = this->authorsInterface;
                auto fandomsInterface = this->fandomsInterface;
                auto processor = [&source, &sink, &fanficsInterface](){
                    statistics_utils::UserPagePageResult result;
                    forever{
                        result = source.Get();
                        if(result.finished)
                        {
                            qDebug() << "thread stopping";
                            break;
                        }

                        QSharedPointer<FavouriteStoryParser> parser;

                        page_utils::SplitJobs splittings;
                        splittings = page_utils::SplitJob(result.page.content, false);
                        QString name = ParseAuthorNameFromFavouritePage(result.page.content);
                        // need to create author when there is no data to parse

                        parser->ProcessPage(result.page.url, result.page.content);
                        auto author = CreateAuthorFromNameAndUrl(name, result.page.url);
                        author->favCount = splittings.favouriteStoryCountInWhole;
                        author->ficCount = splittings.authorStoryCountInWhole;
                        auto webId = url_utils::GetWebId(result.page.url, "ffn").toInt();
                        author->SetWebID("ffn", webId);
                        parser->SetAuthor(author);
                        ParsedAuthorData data;
                        data.parser = parser;
                        //qDebug() << "processed page url: " << result.page.url << " webid: " << webId;
                        sink.Push(data);
                    }
                };
                qDebug() << "running threads pages";
                for(int i = 0; i <threadCount; i++)
                {
                    QtConcurrent::run(processor);
                }
                TimedAction processSlash("Process Pack", [&](){
                    while(sink.GetSize() != source.GetInitialSize())
                    {
                        QCoreApplication::processEvents();
                    }

                });
                processSlash.run();
                QThread::msleep(100);
                qDebug() << "writing results, have " << sink.tokens.size() << " tokens to merge";
                for(const auto& token: std::as_const(sink.tokens))
                {
                    //qDebug() << "original new webid: " << token.parser->recommender.author->GetWebID("ffn");
                    authorsInterface->EnsureId(token.parser->recommender.author);
                    //qDebug() << "new webid: " << token.parser->recommender.author->GetWebID("ffn");
                    FavouriteStoryParser::MergeStats(token.parser->recommender.author,
                                                     fandomsInterface,
                    {token.parser->statToken});
                    authorsInterface->UpdateAuthorRecord(token.parser->recommender.author);
                    token.parser->recommender.author = authorsInterface->GetByWebID("ffn", token.parser->recommender.author->GetWebID("ffn"));
                    WriteProcessedFavourites(*token.parser, token.parser->recommender.author, fanficsInterface, authorsInterface, fandomsInterface);
                }
                sink.tokens.clear();
                sink.currentSize = 0;
                qDebug() << "written results, loading more pages";
                source.Reserve(chunkSize);
                counter = 1;
                //break;
                transaction.finalize();
            }
        }

}
