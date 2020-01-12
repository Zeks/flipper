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
#include "GlobalHeaders/run_once.h"

#include "include/Interfaces/interface_sqlite.h"

void SetupLogger()
{
    QSettings settings("settings/settings_data_transfer.ini", QSettings::IniFormat);

    An<QsLogging::Logger> logger;
    logger->setLoggingLevel(static_cast<QsLogging::Level>(settings.value("Logging/loglevel").toInt()));
    QString logFile = "data_transfer.log";
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

    parser.addOptions({{{"f", "folder"},
                           QCoreApplication::translate("main", "<folder> to get the migration data from."),
                           QCoreApplication::translate("main", "folder")},
                      });
}


bool OperateOnDataTasks(QDir rootFolder, std::function<bool(QString)> fileProcessor){
    // first we need to read all the entries
    qDebug() << "Processing Folder: " << rootFolder.canonicalPath();
    QStringList fileEntries = rootFolder.entryList(QStringList() << "*.ini", QDir::Files);
    QStringList allEntries = fileEntries;
    std::sort(allEntries.begin(), allEntries.end());
    bool success = true;
    for(auto entry : allEntries){
        if(entry.left(1) == "_")
            continue;
        QString tokenPath = rootFolder.canonicalPath() + "/" + entry;

        if(fileEntries.contains(entry)){
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
    return std::make_tuple(parser.value("folder"));
}




QSqlDatabase OpenSqliteDatabase(QString filename, QString connectionName){
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(filename);
    bool isOpen = db.open();
    return db;
}

QSqlDatabase OpenPostgresDatabase(QString host, int port, QString user, QString password, QString connectionName){
    QSqlDatabase db;
    bool ok  = false;
    db = QSqlDatabase::addDatabase("QPSQL", connectionName);
    db.setHostName(host);
    db.setDatabaseName(user);
    db.setUserName(user);
    db.setPort(port);
    db.setPassword(password);
    ok = db.open();
    return db;
}

QSqlDatabase OpenDatabase(QSettings& settings, QString group){
    FuncCleanup f([&](){settings.endGroup();});
    settings.beginGroup(group);
    QString type = settings.value("type").toString();
    if(type == "sqlite")
        return OpenSqliteDatabase(settings.value("location").toString(), group);
    if(type == "postgres")
        return OpenPostgresDatabase(settings.value("hostname").toString(),
                                    settings.value("port").toInt(),
                                    settings.value("user").toString(),
                                    settings.value("pass").toString(),
                                    group);
    return QSqlDatabase();
}

bool VerifyDatabaseSettings(QSettings& settings, QString group){
    FuncCleanup f([&](){settings.endGroup();});
    settings.beginGroup(group);
    auto keyVerifier = [group](QSettings& settings, QStringList keys){
        for(auto key: keys){
            if(!settings.childKeys().contains(key))
            {
                qDebug() << group << " doesn't contain necessary key: " << key;
                return false;
            }
        }
        return true;
    };
    if(!keyVerifier(settings, {"type"}))
        return false;

    QString type = settings.value("type").toString();
    if(type == "sqlite")
    {
        if(!keyVerifier(settings, {"location"}))
            return false;
        auto dbFile = settings.value("location").toString();
        if(dbFile.isEmpty() || !QFileInfo::exists(dbFile)){
            qDebug() << group << " no sqlite file: " << dbFile ;
        }
    }
    else if(type == "postgres"){
        if(!keyVerifier(settings, {"hostname","port", "user", "pass"}))
            return false;
    }
    else{
        qDebug() << "database: " << group << " contains invalid type key:" << type;
        return false;
    }
    return true;
}


auto GetDatabases(QString folder){
    auto result = std::make_tuple(QSqlDatabase::database(), QSqlDatabase::database());
    QSettings dbInfo(folder + "/databases.info", QSettings::IniFormat);
    QString source = dbInfo.value("source").toString();
    QString target = dbInfo.value("target").toString();
    if(source.isEmpty() || target.isEmpty())
        return result;

    auto groupDiagnostic = [](QSettings& settings, QString group){
        if(!settings.childGroups().contains(group))
        {
            qDebug() << "no such group: " << group;
            return true;
        }
        return false;
    };

    QSettings databases("settings/databases.ini", QSettings::IniFormat);
    if(!groupDiagnostic(databases,source) || !groupDiagnostic(databases,target))
        return result;

    if(!VerifyDatabaseSettings(databases,source) || !VerifyDatabaseSettings(databases,target))
        return result;

    result = std::make_tuple(OpenDatabase(databases,source), OpenDatabase(databases,target));
    return result;
}

class PostgresSchemaAccess{
public:
    static bool HasColumn(QString table, QString column, QSqlDatabase db){
        QString qs = "SELECT EXISTS (SELECT 1  FROM information_schema.columns WHERE table_schema='%1' AND table_name='%2' AND column_name='%3');";
        qs = qs.arg(table.split(".").at(0)).arg(table.split(".").at(1)).arg(column);
        using namespace database::puresql;
        SqlContext<bool> ctx(db, qs);
        if(!ctx.ExecAndCheck())
            return false;
        return ctx.result.data;
    }
    static bool HasTable(QString table, QSqlDatabase db){
        using namespace database::puresql;
        QString qs = QString(" SELECT '%1'::regclass ").arg(table);
        SqlContext<bool> ctx(db, qs);

        if(!ctx.ExecAndCheck())
            return false;
        return true;
    }
    static QStringList GetAllColumns(QString table, QSqlDatabase db){
        using namespace database::puresql;
        QString qs  = "SELECT * FROM information_schema.columns WHERE table_schema = '%1' AND table_name = '%2'";
        qs = qs.arg(table.split(".").at(0)).arg(table.split(".").at(1));
        SqlContext<QStringList> ctx(db, qs);
        ctx.FetchLargeSelectIntoList<QString>("column_name", qs);
        if(!ctx.result.success)
            return {};
        return ctx.result.data;
    }
};

class SqliteSchemaAccess{
public:
    static bool HasColumn(QString tableName, QString columnName, QSqlDatabase db){
        QString qs = "SELECT COUNT(*) AS CNTREC FROM pragma_table_info('%1') WHERE name='%2' collate nocase";
        qs = qs.arg(tableName).arg(columnName);
        using namespace database::puresql;
        SqlContext<bool> ctx(db, qs);
        if(!ctx.ExecAndCheck())
            return false;
        return ctx.result.data;
    }
    static bool HasTable(QString tableName, QSqlDatabase db){
        QString qs = "SELECT count(name) FROM sqlite_master WHERE type='table' AND name='%1' collate nocase";
        qs = qs.arg(tableName);
        using namespace database::puresql;
        SqlContext<bool> ctx(db, qs);
        if(!ctx.ExecAndCheck())
            return false;
        return ctx.result.data;
    }
    static QStringList GetAllColumns(QString tableName, QSqlDatabase db){
        QString qs = "select name from pragma_table_info('%1')";
        qs = qs.arg(tableName);
        using namespace database::puresql;
        SqlContext<QStringList> ctx(db, qs);
        ctx.FetchLargeSelectIntoList<QString>("name", qs);
        if(!ctx.result.success)
            return {};
        return ctx.result.data;
    }

};

bool TableExists(QString table, QSqlDatabase database){
    QString driverName = database.driverName();
    if(driverName == "QSQLITE")
        return SqliteSchemaAccess::HasTable(table, database);
    if(driverName == "QPSQL")
        return PostgresSchemaAccess::HasTable(table, database);
    qDebug() << "Unknown driver name:" << driverName;
    return false;
};

QStringList GetAllColumns(QString table, QSqlDatabase database){
    QString driverName = database.driverName();
    if(driverName == "QSQLITE")
        return SqliteSchemaAccess::GetAllColumns(table, database);
    if(driverName == "QPSQL")
        return PostgresSchemaAccess::GetAllColumns(table, database);
    qDebug() << "Unknown driver name:" << driverName;
    return {};
}

struct TaskPartSource{
    QStringList columns;
    QString table;
    QString pivot;
    QString pivotType;
    QString identity;
};
struct TaskPartTarget{
    QStringList TransformColumnNames(QStringList sources){
        QStringList result;
        for(auto source : sources)
            if(renames.contains(source))
                result.push_back(renames[source]);
            else
                result.push_back(source);
        return result;
    }
    QString table;
    QString pivot;
    QString pivotType;
    QString identity;

    bool preClean = false;
    QString mode;
    QHash<QString, QString> renames;

};

bool VerifyTaskFileAndFillTasks(QString file,
                    QSqlDatabase sourceDB,
                    QSqlDatabase targetDB,
                    TaskPartSource& taskSource,
                    TaskPartTarget& taskTarget){
    auto keyExistsVerifier = [](QSettings& settings, QString group, QStringList keys){
        FuncCleanup f([&](){settings.endGroup();});
        settings.beginGroup(group);
        for(auto key: keys){
            if(!settings.childKeys().contains(key))
            {
                qDebug() << group << " doesn't contain necessary key: " << key;
                return false;
            }
        }
        return true;
    };

    QSettings taskFile(file, QSettings::IniFormat);
    // first: verify that all necessary keys exist
    if(!keyExistsVerifier(taskFile, "source", {"columns", "table","pivot","pivotType","identity"}))
        return false;
    if(!keyExistsVerifier(taskFile, "target", {"preclean", "mode", "renames", "table","pivot","pivotType","identity"}))
        return false;

    // verify that mode is one of the valid modes
    taskTarget.mode = taskFile.value("target/mode").toString();
    QStringList validModes = {"replace_rows","append_rows","replace_data"};
    if(!validModes.contains(taskTarget.mode))
    {
        qDebug() << "Incorrect processing mode: " << taskTarget.mode;
        qDebug() << "Valid modes include: " << validModes;
        return false;
    }
    // then verify that tables exist
    taskSource.table = taskFile.value("source/table").toString();
    taskTarget.table = taskFile.value("target/table").toString();

    bool sourceTableExists = TableExists(taskSource.table, sourceDB);
    bool targetTableExists = TableExists(taskTarget.table, targetDB);
    if(!sourceTableExists  || !targetTableExists)
        return false;

    auto allSourceColumns = GetAllColumns(taskFile.value("source/table").toString(), sourceDB);
    auto allTargetColumns = GetAllColumns(taskFile.value("target/table").toString(), targetDB);
    if(allSourceColumns.isEmpty() || allTargetColumns.isEmpty()){
        qDebug() << "Source or target columns are empty";
        return false;
    }

    // verify that columns exist
    QStringList sourceColumns = taskFile.value("source/columns").toString().split(",", QString::SkipEmptyParts);
    for(auto column : sourceColumns){
        if(column == "*")
        {
            taskSource.columns = allSourceColumns;
            break;
        }
        if(!allSourceColumns.contains(column))
        {
            qDebug() << "Source column doesn't exist in table: " << column;
            return false;
        }
    }
    taskSource.columns = sourceColumns;

    // then verify that renames exist
    QStringList renamesList = taskFile.value("target/renames").toString().split(",", QString::SkipEmptyParts);
    for(auto rename : renamesList)
    {
        QStringList renameBits = rename.split("#:#", QString::SkipEmptyParts);
        if(renameBits.size() != 2)
        {
            qDebug() << "Rename pair invalid: " << renameBits;
            return false;
        }
        if(!allSourceColumns.contains(renameBits.at(0)))
        {
            qDebug() << "Source column doesn't exist in table: " << renameBits.at(0);
            return false;
        }
        if(!allTargetColumns.contains(renameBits.at(1)))
        {
            qDebug() << "Target column doesn't exist in table: " << renameBits.at(1);
            return false;
        }
        taskTarget.renames[renameBits.at(0)] = renameBits.at(1);
    }

    // then verify that pivot and identity exist
    auto columnExistsVerifier = [](QStringList columnList, QString columnName, QString columnDesignation){
        if(!columnList.contains(columnName))
        {
            qDebug() << "Column doesn't exit: " << columnDesignation;
            return false;
        }
        return true;
    };

    QString sourcePivot = taskFile.value("source/pivot").toString();
    QString sourcePivotType = taskFile.value("source/pivotType").toString();
    QString sourceIdentity= taskFile.value("source/identity").toString();
    QString targetPivot = taskFile.value("target/pivot").toString();
    QString targetPivotType = taskFile.value("target/pivotType").toString();
    QString targetIdentity= taskFile.value("target/identity").toString();
    if(!columnExistsVerifier(allSourceColumns, sourcePivot, "source pivot")
            || !columnExistsVerifier(allSourceColumns, sourcePivotType, "source pivot type")
            || !columnExistsVerifier(allSourceColumns, sourcePivot, "source identity") )
        return false;
    if(!columnExistsVerifier(allTargetColumns, targetPivot, "target pivot")
            || !columnExistsVerifier(allTargetColumns, targetPivotType, "target pivot type")
            || !columnExistsVerifier(allTargetColumns, targetIdentity, "target identity") )
        return false;

    taskSource.pivot = sourcePivot;
    taskSource.pivotType = sourcePivotType;
    taskSource.identity = sourceIdentity;

    taskTarget.pivot = targetPivot;
    taskTarget.pivotType = targetPivotType;
    taskTarget.identity = targetIdentity;
    taskTarget.preClean = taskFile.value("target/preclean").toBool();
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("schema_updater");
    SetupLogger();



    QCommandLineParser parser;
    SetupCommandLineParser(parser);
    parser.process(a);
    auto [folder] = GetCommandLineArguments(parser);
    {}


    auto[sourceDB, targetDB] = GetDatabases(folder);
    {}
    auto dbDiagnostics = [](QSqlDatabase db, QString name){
        if(!db.isOpen())
        {
            qDebug() << "failed to open database: " << name;
            return false;
        }
        return true;
    };

    if(!dbDiagnostics(sourceDB, "source") || !dbDiagnostics(targetDB, "target"))
        return -1;

    auto transactionResult = targetDB.transaction();
    auto result = OperateOnDataTasks(folder, [sourceDB, targetDB](QString entry)->bool{
        using namespace database::puresql;
        TaskPartSource taskSource;
        TaskPartTarget taskTarget;
        VerifyTaskFileAndFillTasks(entry, sourceDB, targetDB, taskSource, taskTarget);

        // wiping full table if necessary
        if(taskTarget.preClean == true)
        {
            QString qs = QString("delete from %1");
            qs = qs.arg(taskTarget.table);
            SqlContext<bool> ctx(targetDB, qs);
            ctx();
        }



        QString selectFromSource = "select " + taskSource.columns.join(",") + " from " + taskSource.table;
        QVariant pivotValue;
        if(taskSource.pivotType == "date")
        {
            QString qs = QString("select max(%1) as pivot from %2");
            SqlContext<QDateTime> ctx(targetDB, qs);
            ctx.FetchSingleValue<QDateTime>("pivot", QDateTime(), false);
            if(!ctx.result.success)
                return false;
            pivotValue = ctx.result.data;
        }
        else if(taskSource.pivotType == "string")
        {
            QString qs = QString("select max(%1) as pivot from %2");
            SqlContext<QString> ctx(targetDB, qs);
            ctx.FetchSingleValue<QString>("pivot", "-1", false);
            if(!ctx.result.success)
                return false;
            pivotValue = ctx.result.data;
        }


        bool validDatePivot = taskSource.pivotType == "date" && pivotValue.toDateTime().isValid();
        bool validStringPivot = taskSource.pivotType == "string" && !pivotValue.toString().isEmpty();
        if(validDatePivot || validStringPivot)
            selectFromSource += " where " + taskSource.pivot + " > :" + taskSource.pivot;



        QStringList keyList = {"tag","id"};
        QString insertQS = QString("insert into %1(%2) values(%3)");
        QStringList targetColumns = taskTarget.TransformColumnNames(taskSource.columns);
        insertQS = insertQS.arg(taskTarget.table).arg(targetColumns.join(","));
        QStringList binds;
        for(auto column: targetColumns)
            binds.push_back(":" + column);
        insertQS = insertQS.arg(binds.join(","));

        QStringList conflictPart;
        QString conflictMerged;
        for(auto column: targetColumns)
            binds.push_back(column + "= :" + column);
        if(conflictPart.size() > 0)
        {
            conflictMerged += " on conflict do update set " + conflictPart.join(",") + " where " + taskTarget.identity + " = :" + taskTarget.identity;
        }
        insertQS += conflictMerged;


        ParallelSqlContext<bool> ctx (sourceDB, selectFromSource, taskSource.columns,
                                      targetDB, insertQS, targetColumns);
        if(validDatePivot)
            ctx.bindSourceValue(taskSource.pivot, pivotValue.toDateTime());
        else if(validStringPivot)
            ctx.bindSourceValue(taskSource.pivot, pivotValue.toString());
        ctx();
        return true;
    });
    if(!result)
        targetDB.rollback();
    else{
        targetDB.commit();
    }


    return 0;
}
