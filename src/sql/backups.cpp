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
#include "sql/backups.h"
#include "sqlitefunctions.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QMessageBox>
namespace database {
namespace sqlite{

sql::DiagnosticSQLResult<sql::DBVerificationResult> VerifyDatabase(sql::ConnectionToken connectionToken){

    sql::DiagnosticSQLResult<sql::DBVerificationResult>  result;
    auto db = sql::Database::addDatabase("QSQLITE","TEST");
    db.setConnectionToken(connectionToken);
    bool open = db.open();
    if(!open)
    {
        result.success = false;
        result.data.data.push_back("Database file failed to open");
        return result;
    }
    result = sql::VerifyDatabaseIntegrity(db);
    db.close();
    return result;
}

bool ProcessBackupForInvalidDbFile(QString pathToFile,
                                   QString fileName,
                                   QStringList error)

{
    auto backupPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups";
    QDir dir;
    dir.setPath(backupPath);
    dir.setNameFilters({fileName + "*.sqlite"});
    auto entries = dir.entryList(QDir::NoFilter, QDir::Time|QDir::Reversed);

    bool backupRestored = false;
    QString fullFileName = pathToFile + "/" + fileName + ".sqlite";
    if(entries.size() > 0)
    {

        if(QFileInfo::exists(fullFileName))
            QFile::copy(fullFileName, pathToFile + "/" + fileName + ".sqlite.corrupted." + QDateTime::currentDateTime().toString("yyMMdd_hhmm"));

        QMessageBox::StandardButton reply;
        QString message = "Current database file is corrupted, but there is a backup in the ~User folder. Do you want to restore the backup?"
                          "\n\n"
                          "If \"No\" is selected a new database will be created.\n"
                          "If \"Yes\" a latest backup will be restored."
                          "\n\n"
                          "Corruption error:\n%1";
        auto trimmedError = error.join("\n");
        trimmedError = trimmedError.mid(0, 200);
        message = message.arg(trimmedError);
        reply = QMessageBox::question(nullptr, "Warning!", message,
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            if(fullFileName.length() > 10)
            {
                QFile::remove(fullFileName);
                QFile::copy(backupPath + "/" + entries.at(0), fullFileName);
                backupRestored = true;
            }
        }

        else{
            if(fullFileName.length() > 10)
                QFile::remove(fullFileName);
        }
    }
    else{
        if(fullFileName.length() > 10)
            QFile::remove(fullFileName);
    }
    return backupRestored;
}

void RemoveOlderBackups(QString fileName){
    QDir dir;
    auto backupPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups";
    dir.mkpath(backupPath);
    dir.setPath(backupPath);

    dir.setNameFilters({fileName + "*.sqlite"});
    auto entries = dir.entryList(QDir::NoFilter, QDir::Time);

    if(entries.size() > 10)
    {
        for(int i = 10; i < entries.size(); i++)
            QFile::remove(backupPath + "/" + entries.at(i));
    }
}

bool CreateDatabaseBackup(sql::Database originalDb, QString targetFolder, QString targetFile, QString dbInitFile)
{
    QDir backupDir;
    backupDir.mkpath(targetFolder);

    auto backupDb = database::sqlite::InitAndUpdateSqliteDatabaseForFile(targetFolder, targetFile,
                                                                         dbInitFile, targetFile, false);

    bool success = true;
    database::Transaction transaction(backupDb);
    success &= sql::PassScoresToAnotherDatabase(originalDb, backupDb).success;
    success &= sql::PassTagSetToAnotherDatabase(originalDb, backupDb).success;
    success &= sql::PassFicTagsToAnotherDatabase(originalDb, backupDb).success;
    success &= sql::PassSnoozesToAnotherDatabase(originalDb, backupDb).success;
    success &= sql::PassFicNotesToAnotherDatabase(originalDb, backupDb).success;
    success &= sql::PassClientDataToAnotherDatabase(originalDb, backupDb).success;
    success &= sql::PassRecentFandomsToAnotherDatabase(originalDb, backupDb).success;
    success &= sql::PassReadingDataToAnotherDatabase(originalDb, backupDb).success;
    success &= sql::PassIgnoredFandomsToAnotherDatabase(originalDb, backupDb).success;
    success &= sql::PassFandomListSetToAnotherDatabase(originalDb, backupDb).success;
    success &= sql::PassFandomListDataToAnotherDatabase(originalDb, backupDb).success;
    if(!success){
        transaction.cancel();
        return false;
    }
    transaction.finalize();
    return true;
}

}}
