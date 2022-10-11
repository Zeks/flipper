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
#pragma once

#include "core/section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "sql_abstractions/sql_database.h"
#include "QReadWriteLock"


namespace interfaces {
class Fandoms;
struct TagIDFetcherSettings{
    QStringList tags;
    bool useAND = false;
    bool allowSnoozed = false;
};
class Tags{
public:
    void LoadAlltags();
    void EnsureTag(QString tag);
    bool DeleteTag(QString);
    bool CreateTag(QString);
    QStringList ReadUserTags();
    bool SetTagForFic(int ficId, QString tag);
    bool RemoveTagFromFic(int ficId, QString tag);
    bool ExportToFile(QString);
    bool ImportFromFile(QString);
    QSet<int> GetAllTaggedFics(TagIDFetcherSettings settings = {});
    QSet<int> GetFicsTaggedWith(TagIDFetcherSettings settings = {});
    QSet<int> GetAuthorsForTags(QStringList);
    QVector<core::Identity> GetAllFicsThatDontHaveDBID();
    QHash<QString, int> GetTagSizes(QStringList);
    bool FillDBIDsForFics(QVector<core::Identity>);
    bool FetchTagsForFics(QVector<core::Fanfic>*);
    bool RemoveTagsFromEveryFic(QStringList);


sql::Database db;
QSharedPointer<Fandoms> fandomInterface;
private:
    QStringList CreateDefaultTagList();
    QStringList allTags;



};


}
