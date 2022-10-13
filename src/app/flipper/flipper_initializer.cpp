#include "include/app/flipper/flipper_initializer.h"
#include "ui/mainwindow.h"
#include <QStandardPaths>


bool FlipperInitializer::Init(){

    SetupLogger();
    ProcessStateOfExistingDatabase();
    coreEnvironment.reset(new FlipperClientLogic());

    if(RequiresReinit() && !PerformInitialSetup())
        return false;
    else
        SetupForValidDatabase();


    SetupMainWindow();
    database::sqlite::RemoveOlderBackups("UserDB");

    return true;
}

void FlipperInitializer::EnsureBackupPathExists(QString path) const{
    QDir dir;
    dir.mkpath(path);
}

void FlipperInitializer::SetupLogger() const
{

    An<QsLogging::Logger> logger;
    logger->setLoggingLevel(static_cast<QsLogging::Level>(settings.value("Logging/loglevel").toInt()));
    QString logFile = settings.value("Logging/filename").toString();
    QsLogging::DestinationPtr fileDestination(

                QsLogging::DestinationFactory::MakeFileDestination(logFile,
                                                                   settings.value("Logging/rotate", true).toBool(),
                                                                   settings.value("Logging/filesize", 512).toInt()*1000000,
                                                                   settings.value("Logging/amountOfFilesToKeep", 50).toInt()));

    QsLogging::DestinationPtr debugDestination(
                QsLogging::DestinationFactory::MakeDebugOutputDestination() );
    logger->addDestination(debugDestination);
    logger->addDestination(fileDestination);
    QLOG_INFO() << "current appPath is: " << QDir::currentPath();
}



typedef sql::DiagnosticSQLResult<sql::DBVerificationResult> VerificationResult;
void FlipperInitializer::ProcessStateOfExistingDatabase(){
    auto backupPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups";
    EnsureBackupPathExists(backupPath);

    QString flipperMainDBFile = "UserDB";


    QString databaseFolderPath = uiSettings.value("Settings/dbPath", QCoreApplication::applicationDirPath()).toString();
    QString currentDatabaseFile = databaseFolderPath + "/" + flipperMainDBFile + ".sqlite";
    hasDBFile = QFileInfo::exists(currentDatabaseFile);

    // to make sure it's not been explicitly reset in ini file
    if(databaseFolderPath.length() > 0)
    {
        VerificationResult verificationResult;
        sql::ConnectionToken token;
        token.folder = databaseFolderPath.toStdString();
        token.initFileName = "dbcode/user_db_init";
        token.serviceName = flipperMainDBFile.toStdString();

        if(hasDBFile)
            verificationResult = database::sqlite::VerifyDatabase(token);
        hasDBFile  = hasDBFile && verificationResult.success;
        if(!hasDBFile)
        {
            backupRestored = database::sqlite::ProcessBackupForInvalidDbFile(databaseFolderPath, flipperMainDBFile, verificationResult.data.data);
            if(!backupRestored)
                hasDBFile = false;
            else
                hasDBFile =true;

        }
    }

}

bool FlipperInitializer::PerformInitialSetup()
{
    InitialSetupDialog setupDialog;
    setupDialog.setWindowTitle("Welcome!");
    setupDialog.setWindowModality(Qt::ApplicationModal);
    setupDialog.env = coreEnvironment;
    setupDialog.exec();
    if(!setupDialog.initComplete)
        return false;
    if(setupDialog.recsCreated)
        newInstanceCreatedRecs = true;

    slashFilterScheduled = !setupDialog.readsSlash;
    return true;
}

bool FlipperInitializer::SetupForValidDatabase()
{

    coreEnvironment->InstantiateClientDatabases(uiSettings.value("Settings/dbPath", QCoreApplication::applicationDirPath()).toString());
    if(hasDBFile && !backupRestored)
        database::sqlite::CreateDatabaseBackup(coreEnvironment->GetUserDatabase(),
                                               QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups",
                                               "UserDB_" + QDateTime::currentDateTime().toString("yyMMdd"),
                                               QDir::currentPath() + "/dbcode/user_db_init.sql");

    coreEnvironment->InitInterfaces();
    coreEnvironment->Init();

    return true;
}

bool FlipperInitializer::RequiresReinit() const
{
    return !hasDBFile || !uiSettings.value("Settings/initialInitComplete", false).toBool();
}

bool FlipperInitializer::SetupMainWindow()
{
    w.reset(new MainWindow);
    if(newInstanceCreatedRecs || backupRestored)
        w->QueueDefaultRecommendations();

    w->env = coreEnvironment;
    if(!w->InitFromReadyEnvironment(slashFilterScheduled))
        return false;
    w->InitConnections();
    w->show();
    w->DisplayInitialFicSelection();
    w->StartTaskTimer();
    return true;
}


void FlipperInitializer::EnsureDelayForUserParsing() const{
    An<PageManager>()->timeout = settings.value("Settings/pageFetchTimeout", 2000).toInt();
}

