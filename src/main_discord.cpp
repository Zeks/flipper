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
//#include "discord/client.h"
#include "discord/client_v2.h"
#include "discord/discord_user.h"
#include "discord/discord_init.h"
#include "discord/command_controller.h"
#include "discord/db_vendor.h"
#include "pure_sql.h"
#include "sql/discord/discord_queries.h"

#include "include/grpc/grpc_source.h"
#include "include/sqlitefunctions.h"

#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "Interfaces/fanfics.h"
#include "Interfaces/discord/users.h"
#include "Interfaces/ffn/ffn_fanfics.h"
#include "Interfaces/ffn/ffn_authors.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/authors.h"

#include "sql/discord/discord_queries.h"


#include "GlobalHeaders/SingletonHolder.h"
#include "logger/QsLog.h"




#include <QDebug>
#include <QSettings>
#include <QtConcurrent>
#include "sql_abstractions/sql_database.h"
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


static inline QString CreateConnectString(QString ip,QString port)
{
    QString server_address_proto("%1:%2");
    std::string result = server_address_proto.arg(ip,port).toStdString();
    return QString::fromStdString(result);
}


int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    a.setApplicationName("Socrates");
    SetupLogger();

    QSettings settings("settings/settings_discord.ini", QSettings::IniFormat);

    sql::ConnectionToken pgConnectionToken;
    pgConnectionToken.ip = settings.value("users/serverIp").toString().toStdString();
    pgConnectionToken.password = settings.value("users/password").toString().toStdString();
    pgConnectionToken.port = settings.value("users/port").toInt();
    pgConnectionToken.user = settings.value("users/login").toString().toStdString();
    pgConnectionToken.tokenType = "PQXX";
    sql::Database db;
    db = sql::Database::addDatabase(pgConnectionToken.tokenType);
    db.setConnectionToken(pgConnectionToken);
    auto openResult = db.open();
    if(!openResult)
    {
        QLOG_ERROR() << "Failed to open database at the application start, exiting";
        return -1;
    }

    QString discordServiceToken;
    discordServiceToken = sql::GetUserToken(db).data;
    if(discordServiceToken.isEmpty())
    {
        QLOG_ERROR() << "couldn't get discord service token from the database, exiting";
        return -1;
    }


    QSharedPointer<database::IDBWrapper> pageCacheInterface (new database::SqliteInterface());
    pageCacheInterface->SetDatabase(database::sqlite::InitAndUpdateSqliteDatabaseForFile("database","PageCache","dbcode/pagecacheinit.sql", "PageCache", false));
    pageCacheInterface->ReadDbFile("dbcode/pagecacheinit.sql", "PageCache");

    An<discord::DatabaseVendor> vendor;
    vendor->AddConnectionToken("users", pgConnectionToken);
    vendor->AddConnectionToken("pagecache", {"PageCache", "dbcode/pagecacheinit.sql", "database"});

    QSettings bot("settings/bot_token.ini", QSettings::IniFormat);
    auto token = bot.value("Login/botToken").toString().toStdString();

    QSharedPointer<interfaces::Fandoms> fandomsInterface (new interfaces::Fandoms());

    auto ip = settings.value("Login/serverIp", "127.0.0.1").toString();
    auto port = settings.value("Login/serverPort", "3055").toString();
    QLOG_INFO() << "will connect to grpc via: " << ip << " "  << port;

    QSharedPointer<FicSourceGRPC> ficSource;
    ficSource.reset(new FicSourceGRPC(CreateConnectString(ip, port), discordServiceToken,  160));

    fandomsInterface->db = db;
    QVector<core::Fandom> fandoms;
    auto lastFandomId = fandomsInterface->GetLastFandomID();
    ficSource->GetFandomListFromServer(lastFandomId, &fandoms);
    fandomsInterface->LoadAllFandoms();
    if(fandoms.size() > 0)
        fandomsInterface->UploadFandomsIntoDatabase(fandoms, false);
    database::discord_queries::FillUserUids(fandomsInterface->db);
    sql::Database::removeDatabase("DiscordDB");

    //discord::InitHelpForCommands();
    discord::Client client(token);
    client.InitClient();
    client.InitCommandExecutor();
    client.executor->Init(4);
    auto serverSetup = [&](){
        while(true){
            try{
                client.run();
            }
            catch (const SleepyDiscord::ErrorCode& error){
                QLOG_INFO() << "Discord error:" << error;
            }
            catch (const std::bad_variant_access& error){
                QLOG_INFO() << "error:" << error.what();
            }
        }
    };
    QtConcurrent::run(serverSetup);
    return a.exec();
}

