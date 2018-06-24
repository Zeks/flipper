/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
#include "Interfaces/interface_sqlite.h"
#include "pure_sql.h"
#include <QFile>
#include <QDebug>

namespace interfaces {

void Tags::LoadAlltags()
{
    allTags = ReadUserTags();
}

bool Tags::DeleteTag(QString tag)
{
    return database::puresql::DeleteTagFromDatabase(tag, db).success;
}

bool Tags::CreateTag(QString tag)
{
    return database::puresql::CreateTagInDatabase(tag, db).success;
}

QStringList Tags::ReadUserTags()
{
    QStringList tags = database::puresql::ReadUserTags(db).data;
    if(tags.empty())
    {
        tags = CreateDefaultTagList();
        database::puresql::PushTaglistIntoDatabase(tags,db);
    }
    return tags;
}

bool Tags::SetTagForFic(int ficId, QString tag)
{
    tag = tag.trimmed();
    if(!allTags.contains(tag))
        CreateTag(tag);
    allTags.push_back(tag);
    database::puresql::AssignTagToFanfic(tag, ficId, db);
    return true;
}

bool Tags::RemoveTagFromFic(int ficId, QString tag)
{
    return database::puresql::RemoveTagFromFanfic(tag, ficId, db).success;
}

QStringList Tags::CreateDefaultTagList()
{
    QStringList temp;
    temp += "Meh" ;
    temp += "Unknown_fandom" ;
    temp += "Limbo" ;
    temp += "Read_queue";
    temp += "Reading" ;
    temp += "Smut";
    temp += "Finished" ;
    temp += "Accumulating" ;
    temp += "Deleted";
    temp +=  "WTF" ;
    return temp;
}

bool Tags::ExportToFile(QString filename)
{
    QString exportFile = filename;
    auto closeDb = QSqlDatabase::database("TagExport");
    if(closeDb.isOpen())
        closeDb.close();
    bool success = QFile::remove(exportFile);
    QSharedPointer<database::IDBWrapper> tagExportInterface (new database::SqliteInterface());
    auto tagExportDb = tagExportInterface->InitDatabase("TagExport", false);
    tagExportInterface->ReadDbFile("dbcode/tagexportinit.sql", "TagExport");
    database::puresql::ExportTagsToDatabase(db, tagExportDb);
    return true;
}

bool Tags::ImportFromFile(QString filename)
{
    QString importFile = filename;
    auto closeDb = QSqlDatabase::database("TagExport");
    if(closeDb.isOpen())
        closeDb.close();
    if(!QFile::exists(importFile))
    {
        qDebug() << "Could not find importfile";
        return false;
    }
    QSharedPointer<database::IDBWrapper> tagImportInterface (new database::SqliteInterface());
    auto tagImportDb = tagImportInterface->InitDatabase("TagExport", false);
    tagImportInterface->ReadDbFile("dbcode/tagexportinit.sql", "TagImport");
    database::Transaction transaction(db);
    database::puresql::ImportTagsFromDatabase(db, tagImportInterface->GetDatabase());
    transaction.finalize();
    return true;
}

QSet<int> Tags::GetAllTaggedFics(QStringList tags)
{
    return database::puresql::GetAllTaggedFics(tags, db).data;
}

QVector<core::IdPack> Tags::GetAllFicsThatDontHaveDBID()
{
    auto result =  database::puresql::GetAllFicsThatDontHaveDBID(db).data;
    QVector<core::IdPack> pack;
    pack.reserve(result.size());
    for(auto id: result)
    {
        pack.push_back({-1, id, -1,-1, -1});
    }
    return pack;
}

bool Tags::FillDBIDsForFics(QVector<core::IdPack> pack)
{
    return database::puresql::FillDBIDsForFics(pack, db).success;
}

}
