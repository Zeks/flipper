/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2018  Marchenko Nikolai

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
#include "Interfaces/base.h"
#include "Interfaces/db_interface.h"
#include "core/section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"



namespace interfaces {
class IDBWrapper;
class Fandoms;
class Authors {
public:
    virtual ~Authors();
    void Reindex();
    void AddAuthorToIndex(core::AuthorPtr);
    void IndexAuthors();

    void Clear();
    void ClearIndex();
    void ClearCache();

    void AddPreloadedAuthor(core::AuthorPtr);

    virtual bool EnsureId(core::AuthorPtr author) = 0;
    bool EnsureId(core::AuthorPtr author, QString website);

    bool EnsureAuthorLoaded(QString name, QString website);
    bool EnsureAuthorLoaded(QString website, int id);
    //bool EnsureAuthorLoaded(QString url);
    bool EnsureAuthorLoaded(int id);

    bool CreateAuthorRecord(core::AuthorPtr author);
    bool UpdateAuthorRecord(core::AuthorPtr author);
    bool UpdateAuthorFavouritesUpdateDate(core::AuthorPtr author);

    bool LoadAuthors(QString website, bool forced = false);
    QHash<int, QSet<int>> LoadFullFavouritesHashset();

    //for the future, not strictly necessary atm
//    bool LoadAdditionalInfo(core::AuthorPtr) = 0;
//    bool LoadAdditionalInfo() = 0;

    core::AuthorPtr GetAuthorByNameAndWebsite(QString name, QString website);
    QList<core::AuthorPtr> GetAllByName(QString name);
    QSet<int> GetAllMatchesWithRecsUID(QSharedPointer<core::RecommendationList> params, QString uid);
    //core::AuthorPtr GetByUrl(QString url);
    core::AuthorPtr GetByWebID(QString website, int id);
    core::AuthorPtr GetById(int id);
    QList<core::AuthorPtr> GetAllAuthors(QString website, bool forced = false);
    QList<core::AuthorPtr> GetAllAuthorsLimited(QString website, int limit);
    QList<core::AuthorPtr> GetAllAuthorsWithFavUpdateSince(QString website, QDateTime date, int limit = 0);
    QList<core::AuthorPtr> GetAllAuthorsWithFavUpdateBetween(QString website,
                                                            QDateTime dateStart,
                                                            QDateTime dateEnd,
                                                            int limit = 0);
    QStringList GetAllAuthorsUrls(QString website, bool forced = false);
    QStringList GetAllAuthorsFavourites(int id);
    QList<int> GetAllAuthorIds();
    int GetFicCount(int authorId);
    QList<int> GetFicList(core::AuthorPtr author) const;
    int GetCountOfRecsForTag(int authorId, QString tag);
    QSharedPointer<core::AuthorRecommendationStats> GetStatsForTag(int authorId, QSharedPointer<core::RecommendationList> list);
    //bool UploadLinkedAuthorsForAuthor(int authorId, QStringList);
    bool UploadLinkedAuthorsForAuthor(int authorId, QString , QList<int>);
    bool DeleteLinkedAuthorsForAuthor(int authorId);
    QVector<int> GetAllUnprocessedLinkedAuthors();

    bool WipeAuthorStatisticsRecords();
    bool CreateStatisticsRecordsForAuthors();
    bool CalculateSlashStatisticsPercentages(QString fieldUsed);

    bool AssignNewNameForAuthor(core::AuthorPtr, QString name);
    QSet<int> GetAuthorsForFics(QSet<int>);

    bool AssignAuthorNamesForWebIDsInFanficTable();

    QHash<int, std::array<double, 22>> GetListGenreData();
    QHash<int, QSet<int>> GetListFandomSet();
    QHash<int, core::AuthorFavFandomStatsPtr> GetAuthorListFandomStatistics(QList<int> authors);
    QHash<int, genre_stats::ListMoodData> GetMoodDataForLists();

    //! todo those are required for managing recommendation lists and somewhat outdated
    // moved them to dump temporarily
//    virtual bool  RemoveAuthor(int id);
//    virtual bool  RemoveAuthor(core::AuthorPtr) = 0;
//    bool RemoveAuthor(core::AuthorPtr, QString website);

    // queued by webid
    QList<core::AuthorPtr> authors;

    // index
    QHash<QString, QHash<QString, core::AuthorPtr>> authorsNamesByWebsite;
    QHash<QString, QHash<int, core::AuthorPtr>> authorsByWebID;

    QHash<int, core::AuthorPtr> authorsById;
    //QHash<QString, core::AuthorPtr> authorsByUrl;

    // cache
    QHash<QString, QStringList> cachedAuthorUrls;
    QList<int> cachedAuthorIds;
    QHash<int, QList<QSharedPointer<core::AuthorRecommendationStats>>> cachedAuthorToTagStats;

    QSqlDatabase db;
    QSharedPointer<database::IDBWrapper> portableDBInterface;

public:
    bool LoadAuthor(QString name, QString website);
    bool LoadAuthor(QString website, int id);
    bool LoadAuthor(QString url);
    bool LoadAuthor(int id);

    QStringList ListWebsites();
};

}
