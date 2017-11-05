#pragma once
#include "Interfaces/base.h"
#include "Interfaces/db_interface.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace interfaces {
class IDBWrapper;
class Authors {
public:
    virtual ~Authors();
    void Reindex();
    void IndexAuthors();
    void ClearIndex();
    void Clear();
    virtual bool EnsureId(QSharedPointer<core::Author>) = 0;
    //virtual bool EnsureAuthor(int) = 0;
    bool LoadAuthors(QString website, bool additionMode);

    //for the future, not strictly necessary atm
//    bool LoadAdditionalInfo(QSharedPointer<core::Author>) = 0;
//    bool LoadAdditionalInfo() = 0;

    QSharedPointer<core::Author> GetSingleByName(QString name, QString website);
    QList<QSharedPointer<core::Author>> GetAllByName(QString name);
    QSharedPointer<core::Author> GetByUrl(QString url);
    QSharedPointer<core::Author> GetById(int id);
    QList<QSharedPointer<core::Author>> GetAllAuthors(QString website);
    QStringList GetAllAuthorsUrls(QString website);
    QList<int> GetAllAuthorIds();
    int GetFicCount(int authorId);
    int GetCountOfRecsForTag(int authorId, QString tag);
    QSharedPointer<core::AuthorRecommendationStats> GetStatsForTag(int authorId, QSharedPointer<core::RecommendationList> list);
    bool EnsureId(QSharedPointer<core::Author> author, QString website);
    void AddAuthorToIndex(QSharedPointer<core::Author>);
    bool AssignNewNameForAuthor(QSharedPointer<core::Author>, QString name);
    //void AssignNewNameForAuthor(int id, QString name);
    virtual bool  RemoveAuthor(int id);
    virtual bool  RemoveAuthor(QSharedPointer<core::Author>) = 0;
    bool RemoveAuthor(QSharedPointer<core::Author>, QString website);

    // queued by webid
    QList<QSharedPointer<core::Author>> authors;

    //QMultiHash<QString, QSharedPointer<core::Author>> authorsByName;
    QHash<QString, QHash<QString, QSharedPointer<core::Author>>> authorsNamesByWebsite;
    QHash<int, QSharedPointer<core::Author>> authorsById;
    QHash<QString, QSharedPointer<core::Author>> authorsByUrl;
    QHash<QString, QStringList> cachedAuthorUrls;
    QList<int> cachedAuthorIds;

    QHash<int, QList<QSharedPointer<core::AuthorRecommendationStats>>> cachedAuthorToTagStats;

    QSqlDatabase db;
    QSharedPointer<database::IDBWrapper> portableDBInterface;

    // IDBPersistentData interface
public:
    bool IsDataLoaded();
    bool Sync(bool forcedSync);
    bool Load();
};

}
