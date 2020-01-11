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
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSettings>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFileInfo>
#include <QDir>
#include "sqlcontext.h"
#include "db_synch.h"

#include "include/Interfaces/interface_sqlite.h"

void SetupLogger()
{
    QSettings settings("settings/settings_schema_updater.ini", QSettings::IniFormat);

    An<QsLogging::Logger> logger;
    logger->setLoggingLevel(static_cast<QsLogging::Level>(settings.value("Logging/loglevel").toInt()));
    QString logFile = "schema_updater.log";
    QsLogging::DestinationPtr fileDestination(

                QsLogging::DestinationFactory::MakeFileDestination(logFile,
                                                                   settings.value("Logging/rotate", true).toBool(),
                                                                   settings.value("Logging/filesize", 5120).toInt()*1000000,
                                                                   settings.value("Logging/amountOfFilesToKeep", 50).toInt()));

    QsLogging::DestinationPtr debugDestination(
                QsLogging::DestinationFactory::MakeDebugOutputDestination() );
    logger->addDestination(debugDestination);
    logger->addDestination(fileDestination);
}

void SetupCommandLineParser(QCommandLineParser& parser){

    parser.setApplicationDescription("Incremental Schema Updater");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions({
                          {{"s", "sqldriver"},
                           QCoreApplication::translate("main", "Type of sql <database> to use"),
                           QCoreApplication::translate("main", "database")},
                          {{"f", "folder"},
                           QCoreApplication::translate("main", "<folder> to get the migration data from."),
                           QCoreApplication::translate("main", "folder")},
                          {{"c", "connection"},
                           QCoreApplication::translate("main", "<connection> information."),
                           QCoreApplication::translate("main", "connection")},
                          {{"p", "password"},
                           QCoreApplication::translate("main", "<password> for the database."),
                           QCoreApplication::translate("main", "password")}
                      });
}


bool OperateOnTimedPriorityFolders(QDir rootFolder,
                                   std::function<bool(QString)> folderEntryCondition,
                                   std::function<bool(QString)> fileProcessor){
    if(!folderEntryCondition(rootFolder.canonicalPath()))
        return true;
    // first we need to read all the entries
    qDebug() << "Processing Folder: " << rootFolder.canonicalPath();
    QStringList fileEntries = rootFolder.entryList(QStringList() << "*", QDir::Files);
    QStringList folderEntries = rootFolder.entryList(QStringList() << "*", QDir::Dirs);
    QStringList allEntries = fileEntries + folderEntries;
    std::sort(allEntries.begin(), allEntries.end());
    bool success = true;
    for(auto entry : allEntries){
        if(entry == "." || entry == "..")
            continue;
        QString tokenPath = rootFolder.canonicalPath() + "/" + entry;
        if(folderEntries.contains(entry))
        {
            // need to go deeper

            success = OperateOnTimedPriorityFolders(rootFolder.canonicalPath() + "/" + entry, folderEntryCondition, fileProcessor);
            if(!success)
                break;
        }
        else if(fileEntries.contains(entry)){
            // need to execute
            success = fileProcessor(rootFolder.canonicalPath() + "/" + entry);
            if(!success )
                break;
        }
    }
    return success;
}

auto GetCommandLineArguments(QCommandLineParser& parser){
    qDebug() << "value of s is:" << parser.value("sqldriver");
    return std::make_tuple(parser.value("sqldriver"), parser.value("folder"), parser.value("connection"),  parser.value("password"));
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("schema_updater");
    SetupLogger();



    QCommandLineParser parser;
    SetupCommandLineParser(parser);
    parser.process(a);
    auto [database, folder, ip, password] = GetCommandLineArguments(parser);
    {}
    qDebug() << database << QFileInfo(folder).canonicalPath() << ip;
    qDebug() << "Path: " << QDir::currentPath();
    QSqlDatabase db;
    bool ok  = false;
    if(database == "postgres"){
        db = QSqlDatabase::addDatabase("QPSQL");
        db.setHostName(ip.split(":").at(0));
        db.setDatabaseName("postgres");
        db.setUserName("postgres");
        db.setPort(ip.split(":").at(1).toInt());
        db.setPassword(password);
        ok = db.open();
    }
//    {
//        using namespace database::puresql;
//        QString qs = QString("SELECT 'FanficData.database_settings'::regclass;");
//        SqlContext<QString> ctx(db, qs);
//        ctx(true);
//    }

    // latest already processed migration
    QString lastId = "0";
    {
        using namespace database::puresql;
        QString qs = QString(" SELECT 'FanficData.database_settings'::regclass ");
        SqlContext<bool> ctx(db, qs);

        if(!ctx.ExecAndCheck())
            lastId = "0";
        else{
            QString qs = QString("select value from FanficData.database_settings where name = 'Last Migration Id'");
            SqlContext<QString> ctx(db, qs);
            ctx.FetchSingleValue<QString>("value", "-1");
            qDebug() << ctx.result.data;
            lastId = ctx.result.data;
        }
    }

    QRegularExpression rx("(\\d{12})[_]");
    auto entryCondition = [rx, &lastId](QString entry)->bool{
        auto match = rx.match(entry);
        if(!match.hasMatch())
            return true;
        QString id = match.capturedTexts().at(1);
        if(id <= lastId)
        {
            qDebug() << "Skipping entry because its id is lower or the same as currently last processed one" << entry;
            return false;
        }
        lastId = id;
        return true;
    };
    auto transactionResult = db.transaction();
    auto result = OperateOnTimedPriorityFolders(folder, entryCondition, [rx, db](QString entry)->bool{
        auto match = rx.match(entry);
        QString id = match.capturedTexts().at(1);
        if(!match.hasMatch())
        {
            qDebug() << "Skipping file because it's not in the correctly named folder: " << entry;
            return true;
        }
        qDebug() << "Processing file: " << entry;
        auto result = database::ExecuteCommandsFromDbScriptFile(entry, db, false);
        if(!result)
            return false;
        return true;
    });
    if(!result)
        db.rollback();
    else{
        db.commit();
        {
            using namespace database::puresql;
            QString qs = QString(" update fanficdata.database_settings set value = %1 where name = 'Last Migration Id'").arg(lastId);
            SqlContext<bool> ctx(db, qs);
            ctx.ExecAndCheck();
        }
    }


    return 0;
}
