/*
FFSSE is a replacement search engine for fanfiction.net search results
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
#include <QCoreApplication>
#include "sql_abstractions/sql_database.h"
#include "sql_abstractions/sql_query.h"
#include <QSqlError>
#include <QMetaType>
#include <QSqlDriver>
#include "sql_abstractions/sql_query.h"

#include "Interfaces/tags.h"
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "pure_sql.h"
#include <QFile>
#include <QDebug>
#include <QDir>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("Tag extractor");


    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
    qDebug() << "current appPath is: " << QDir::currentPath();
    auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
    
    QString exportFile = "TagExport.sqlite";
    QFile::remove(exportFile);
    QSharedPointer<database::IDBWrapper> tagExportInterface (new database::SqliteInterface());
    auto tagExportDb = tagExportInterface->InitDatabase("TagExport", false);
    tagExportInterface->ReadDbFile("dbcode/tagexportinit.sql", "TagExport");
    sql::ExportTagsToDatabase(mainDb, tagExportDb);

    return 0;
}
