/*
Flipper is a recommendation and search engine for fanfiction.net
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
#include "include/sqlitefunctions.h"


#include "servers/feed.h"
#include "logger/QsLog.h"
#include "loggers/usage_statistics.h"

#include <QCoreApplication>
#include <QSettings>
#include <QtConcurrent>



void SetupLogger()
{
    QSettings settings("settings/settings_server.ini", QSettings::IniFormat);

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
}

void SetupStatLogger()
{
    An<QsLogging::StatLogger> logger;
    logger->setLoggingLevel(static_cast<QsLogging::Level>(0));
    QString logFile = "stat.log";
    QsLogging::DestinationPtr fileDestination(

                QsLogging::DestinationFactory::MakeFileDestination(logFile,
                                                                   true,
                                                                   50000000,
                                                                   10));
    QsLogging::DestinationPtr debugDestination(
                QsLogging::DestinationFactory::MakeDebugOutputDestination() );
    logger->addDestination(debugDestination);
    logger->addDestination(fileDestination);

}


inline std::string CreateConnectString(QString ip,QString port)
{
    QString server_address_proto("%1:%2");
    std::string result = server_address_proto.arg(ip,port).toStdString();
    return result;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("Flipper");

    QSettings stateFile("server_state.ini", QSettings::IniFormat);
    stateFile.setValue("server_state", "Initializing");
    stateFile.sync();
    SetupLogger();
    SetupStatLogger();
    QLOG_INFO() << "Feeder app started server";
    FeederService service;
    auto serverSetup = [&](){
        QSettings settings("settings/settings_server.ini", QSettings::IniFormat);
        auto ip = settings.value("Settings/serverIp", "127.0.0.1").toString();
        auto port = settings.value("Settings/serverPort", "3055").toString();

        std::string server_address = CreateConnectString(ip, port);
        QLOG_INFO() << "Connection string is: " << QString::fromStdString(server_address);

        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        builder.SetMaxMessageSize(1024 * 1024 * 1024);
        std::unique_ptr<Server> server(builder.BuildAndStart());
        QLOG_INFO() << "Starting server";
        stateFile.setValue("server_state", "Ready");
        stateFile.sync();
        settings.sync();
        server->Wait();
    };
    QtConcurrent::run(serverSetup);
    return a.exec();
}

