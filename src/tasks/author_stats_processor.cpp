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
#include "tasks/author_stats_processor.h"
#include "include/pagegetter.h"
#include "include/pagetask.h"
#include "include/parsers/ffn/fandomparser.h"
#include "include/parsers/ffn/favparser.h"
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

AuthorStatsProcessor::AuthorStatsProcessor(QSqlDatabase db,
                                           QSharedPointer<interfaces::Fanfics> fanficInterface,
                                           QSharedPointer<interfaces::Fandoms> fandomsInterface,
                                           QSharedPointer<interfaces::Authors> authorsInterface,
                                           QObject *obj): PageConsumer(obj)
{
    this->fanficsInterface = fanficInterface;
    this->fandomsInterface = fandomsInterface;
    this->authorsInterface = authorsInterface;

    this->db = db;
    CreatePageThreadWorker();
    //connect(this, &AuthorStatsProcessor::pageTaskList, worker.data(), &PageThreadWorker::TaskList);
}

void AuthorStatsProcessor::ReprocessAllAuthorsStats(ECacheMode cacheMode)
{
        database::Transaction transaction(db);
        auto authors = this->authorsInterface->GetAllAuthors("ffn", true);

        statistics_utils::UserPageSource source;
        statistics_utils::UserPageSink<core::FicSectionStatsTemporaryToken> sink;

        QHash<int, QList<WebPage>>pages;
        int counter = 0;

        int chunkSize = 10001;
        source.Reserve(chunkSize);
        qDebug() << "loading pages";
        int authorsSize = authors.size();
        for(auto author: authors)
        {
            counter++;
            if(!author)
                continue;

            auto page = env::RequestPage(author->url("ffn"), cacheMode);
            page.id = author->id;
            if(!page.isValid)
                qDebug() << page.url << " is invalid";
            source.Push(page);
            if(counter%(chunkSize - 1) == 0 || counter == authorsSize)
            {
                source.initialSize = source.pages.size();
                qDebug() << "starting to process pack: " << counter/1000 << " of size: " << source.GetInitialSize();


                int threadCount = 12;
                auto fanficsInterface = this->fanficsInterface;
                auto authorsInterface = this->authorsInterface;
                auto fandomsInterface = this->fandomsInterface;
                auto slashRepo = fanficsInterface->GetAllKnownSlashFics();

                auto processor = [&source, &sink, &slashRepo, &fanficsInterface](){
                    statistics_utils::UserPagePageResult result;
                    FavouriteStoryParser parser(fanficsInterface);
                    parser.knownSlashFics = slashRepo;
                    forever{
                        result = source.Get();
                        if(result.finished)
                        {
                            qDebug() << "thread stopping";
                            break;
                        }

                        page_utils::SplitJobs splittings;
                        splittings = page_utils::SplitJob(result.page.content, false);

                        QList<core::FicSectionStatsTemporaryToken> tokens;
                        for(auto part: splittings.parts)
                        {
                            parser.ClearProcessed();
                            parser.ProcessPage(result.page.url, part.data);
                            tokens.push_back(parser.statToken);
                        }
                        core::FicSectionStatsTemporaryToken resultingToken;
                        {
                            for(auto statToken : tokens)
                            {
                                resultingToken.chapterKeeper += statToken.chapterKeeper;
                                resultingToken.ficCount+= statToken.ficCount;

                                if(!resultingToken.firstPublished.isValid())
                                    resultingToken.firstPublished = statToken.firstPublished;
                                resultingToken.firstPublished = std::min(statToken.firstPublished, resultingToken.firstPublished);
                                if(!resultingToken.lastPublished.isValid())
                                    resultingToken.lastPublished = statToken.lastPublished;
                                resultingToken.lastPublished = std::min(statToken.lastPublished, resultingToken.lastPublished);

                                resultingToken.sizes += statToken.sizes;

                                statistics_utils::Combine(resultingToken.crossKeeper, statToken.crossKeeper);
                                statistics_utils::Combine(resultingToken.esrbKeeper, statToken.esrbKeeper);
                                statistics_utils::Combine(resultingToken.fandomKeeper, statToken.fandomKeeper);
                                statistics_utils::Combine(resultingToken.favouritesSizeKeeper, statToken.favouritesSizeKeeper);
                                statistics_utils::Combine(resultingToken.ficSizeKeeper, statToken.ficSizeKeeper);
                                statistics_utils::Combine(resultingToken.genreKeeper, statToken.genreKeeper);
                                statistics_utils::Combine(resultingToken.moodKeeper, statToken.moodKeeper);
                                statistics_utils::Combine(resultingToken.popularUnpopularKeeper, statToken.popularUnpopularKeeper);
                                statistics_utils::Combine(resultingToken.unfinishedKeeper, statToken.unfinishedKeeper);
                                statistics_utils::Combine(resultingToken.wordsKeeper, statToken.wordsKeeper);
                                resultingToken.wordCount += statToken.wordCount;
                                if(statToken.bioLastUpdated.isValid())
                                    resultingToken.bioLastUpdated = statToken.bioLastUpdated;
                                if(statToken.pageCreated.isValid())
                                    resultingToken.pageCreated = statToken.pageCreated;
                            }

                        }

                        resultingToken.authorId = result.page.id;
                        sink.Push(resultingToken);
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
                for(auto token: sink.tokens)
                {
                    auto author = authorsInterface->GetById(token.authorId);
                    FavouriteStoryParser::MergeStats(author,fandomsInterface, {token});
                    authorsInterface->UpdateAuthorRecord(author);
                }
                sink.tokens.clear();
                sink.currentSize = 0;
                qDebug() << "written results, loading more pages";
                source.Reserve(chunkSize);
            }
        }
        transaction.finalize();
}
