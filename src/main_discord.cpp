#include "discord/client.h"

#include "include/grpc/grpc_source.h"
#include "include/sqlitefunctions.h"

#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "Interfaces/fanfics.h"
#include "Interfaces/ffn/ffn_fanfics.h"
#include "Interfaces/ffn/ffn_authors.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/authors.h"


#include "GlobalHeaders/SingletonHolder.h"
#include "logger/QsLog.h"




#include <QDebug>
#include <QSettings>
#include <QtConcurrent>
#include <QSqlDatabase>
#include <QCoreApplication>

void SetupLogger()
{
    QSettings settings("settings_discord.ini", QSettings::IniFormat);

    An<QsLogging::Logger> logger;
    logger->setLoggingLevel(static_cast<QsLogging::Level>(settings.value("Logging/loglevel").toInt()));
    QString logFile = settings.value("Logging/filename", "discord.log").toString();
    QsLogging::DestinationPtr fileDestination(

                QsLogging::DestinationFactory::MakeFileDestination(logFile,
                                                                   settings.value("Logging/rotate", true).toBool(),
                                                                   settings.value("Logging/filesize", 512).toInt()*1000000,
                                                                   settings.value("Logging/amountOfFilesToKeep", 50).toInt()));

    QsLogging::DestinationPtr debugDestination(
                QsLogging::DestinationFactory::MakeDebugOutputDestination() );
    logger->addDestination(debugDestination);
    logger->addDestination(fileDestination);

}


inline QString CreateConnectString(QString ip,QString port)
{
    QString server_address_proto("%1:%2");
    std::string result = server_address_proto.arg(ip).arg(port).toStdString();
    return QString::fromStdString(result);
}


int main(int argc, char *argv[]) {
        QCoreApplication a(argc, argv);
        a.setApplicationName("ffnet sane search engine");
        SetupLogger();
        QSharedPointer<database::IDBWrapper> userDbInterface (new database::SqliteInterface());
        //userDbInterface->EnsureUUIDForUserDatabase();
        //userDbInterface->ReadDbFile("dbcode/discord_init.sql", "database/DiscordDB");
        userDbInterface->SetDatabase(database::sqlite::InitAndUpdateDatabaseForFile("database","DiscordDB","dbcode/discord_init.sql", "DiscordDB", false));
        userDbInterface->EnsureUUIDForUserDatabase();

        QSharedPointer<database::IDBWrapper> pageCacheInterface (new database::SqliteInterface());
        auto pageCacheDb = pageCacheInterface->InitDatabase2("PageCache","pcInit");
        pageCacheInterface->ReadDbFile("dbcode/pagecacheinit.sql", "pcInit");

    QSettings settings("settings/settings_discord.ini", QSettings::IniFormat);
    QSettings bot("settings/bot_token.ini", QSettings::IniFormat);
    auto token = bot.value("Login/botToken").toString().toStdString();
    MyClientClass client(token, 2);

        client.userDbInterface = userDbInterface;
        client.UsersInit();
        client.authors = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
        client.fandoms = QSharedPointer<interfaces::Fandoms> (new interfaces::Fandoms());
        client.fandoms->isClient = true;
        client.fanfics = QSharedPointer<interfaces::Fanfics> (new interfaces::FFNFanfics());
        client.authors->portableDBInterface = client.userDbInterface;
        client.fanfics->authorInterface = client.authors;
        client.fanfics->fandomInterface = client.fandoms;
        client.fandoms->portableDBInterface = client.userDbInterface;
        client.authors->db = client.userDbInterface->GetDatabase();
        client.fanfics->db = client.userDbInterface->GetDatabase();
        client.fandoms->db = client.userDbInterface->GetDatabase();





    auto ip = settings.value("Login/serverIp", "127.0.0.1").toString();
    auto port = settings.value("Login/serverPort", "3055").toString();

        userDbInterface->userToken = userDbInterface->GetUserToken();
        client.ficSource.reset(new FicSourceGRPC(CreateConnectString(ip, port), userDbInterface->userToken,  160));

        QVector<core::Fandom> fandoms;
        client.ficSource->GetFandomListFromServer(client.fandoms->GetLastFandomID(), &fandoms);
        if(fandoms.size() > 0)
            client.fandoms->UploadFandomsIntoDatabase(fandoms);

    auto serverSetup = [&](){
    client.run();
    };
    QtConcurrent::run(serverSetup);
    return a.exec();
}

