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
#include "Interfaces/tags.h"
#include "Interfaces/db_interface.h"

#include "pure_sql.h"
#include "sqlitefunctions.h"
#include <QFile>
#include <QDebug>

namespace interfaces {

void Tags::LoadAlltags()
{
    allTags = ReadUserTags();
}

bool Tags::DeleteTag(QString tag)
{
    return sql::DeleteTagFromDatabase(tag, db).success;
}

bool Tags::CreateTag(QString tag)
{
    return sql::CreateTagInDatabase(tag, db).success;
}

QStringList Tags::ReadUserTags()
{
    QStringList tags = sql::ReadUserTags(db).data;
    QSet<QString> baseSet = {"Liked", "Disliked","Limbo", "Hide", "Meh", "Spoiler", "Wait", "Stem", "Queue", "Finished", "Gems", "Rec", "Series"};
    for(const auto& basicTag: baseSet)
    {
        if(!tags.contains(basicTag))
            tags.push_back(basicTag);
    }
    return tags;
}

bool Tags::SetTagForFic(int ficId, QString tag)
{
    tag = tag.trimmed();
    if(!allTags.contains(tag))
        CreateTag(tag);
    allTags.push_back(tag);
    sql::AssignTagToFanfic(tag, ficId, db);
    return true;
}

bool Tags::RemoveTagFromFic(int ficId, QString tag)
{
    return sql::RemoveTagFromFanfic(tag, ficId, db).success;
}

QStringList Tags::CreateDefaultTagList()
{
    QStringList temp;
    temp += "Liked" ;
    temp += "Disliked" ;
    temp += "Hide" ;
    temp += "Limbo" ;
    temp += "Wait" ;
    temp += "Stem" ;
    temp += "Queue" ;
    temp += "Reading" ;
    temp += "Finished" ;
    temp += "Gems";
    temp += "Rec";
    temp += "Series";
    return temp;
}

bool Tags::ExportToFile(QString filename)
{
    QString exportFile = filename;
    auto closeDb = sql::Database::database("TagExport");
    if(closeDb.isOpen())
        closeDb.close();
    bool success = QFile::remove(exportFile);
    Q_UNUSED(success);

    auto exportDb = database::sqlite::InitSqliteDatabase("TagExport", false);
    database::sqlite::ReadDbFile("dbcode/tagexportinit.sql", "TagExport");
    sql::ExportTagsToDatabase(db, exportDb); // todo check in git
    return true;
}

bool Tags::ImportFromFile(QString filename)
{
    QString importFile = filename;
    auto closeDb = sql::Database::database("TagExport");
    if(closeDb.isOpen())
        closeDb.close();
    if(!QFile::exists(importFile))
    {
        qDebug() << "Could not find importfile";
        return false;
    }
    auto otherDb = database::sqlite::InitSqliteDatabase("TagImport", false);
    database::sqlite::ReadDbFile("dbcode/tagexportinit.sql", "TagImport");
    database::Transaction transaction(db);
    sql::ImportTagsFromDatabase(db, otherDb);
    transaction.finalize();
    return true;
}

QSet<int> Tags::GetAllTaggedFics(TagIDFetcherSettings)
{
    return sql::GetAllTaggedFics(db).data;
}

QSet<int> Tags::GetFicsTaggedWith(TagIDFetcherSettings settings)
{
    return sql::GetFicsTaggedWith(settings.tags, settings.useAND, db).data;
}

QSet<int> Tags::GetAuthorsForTags(QStringList tags)
{
    return sql::GetAuthorsForTags(tags, db).data;
}

QVector<core::Identity> Tags::GetAllFicsThatDontHaveDBID()
{
    auto result =  sql::GetAllFicsThatDontHaveDBID(db).data;
    QVector<core::Identity> pack;
    pack.reserve(result.size());
    for(auto id: std::as_const(result))
    {
        core::Identity newIdentity;
        newIdentity.id = id;
        pack.push_back(newIdentity);
    }
    return pack;
}

QHash<QString, int> Tags::GetTagSizes(QStringList tags)
{
    return sql::GetTagSizes(tags, db).data;
}

bool Tags::FillDBIDsForFics(QVector<core::Identity> pack)
{
    return sql::FillDBIDsForFics(pack, db).success;
}

bool Tags::FetchTagsForFics(QVector<core::Fanfic> * fics)
{
    return sql::FetchTagsForFics(fics, db).success;
}

bool Tags::RemoveTagsFromEveryFic(QStringList tags)
{
    return sql::RemoveTagsFromEveryFic(tags, db).data;
}

}
