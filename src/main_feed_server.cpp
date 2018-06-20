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
#include "Interfaces/db_interface.h"
#include "Interfaces/interface_sqlite.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/recommendation_lists.h"
#include "include/sqlitefunctions.h"
#include "include/db_fixers.h"
#include "include/feeder_environment.h"
#include "include/Interfaces/data_source.h"
#include "include/timeutils.h"
#include "include/in_tag_accessor.h"
#include "grpc/grpc_source.h"
#include "proto/feeder_service.grpc.pb.h"
#include "proto/feeder_service.pb.h"
#include "logger/QsLog.h"


#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QDebug>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <google/protobuf/text_format.h>

//class Feeder{
//public:
//    CoreEnvironment env;
//};
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

void SetupLogger()
{
    QSettings settings("settings_server.ini", QSettings::IniFormat);

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


void ProcessUserToken(const ::ProtoSpace::UserData& user_data, QString userToken)
{
    const auto& taskTags = user_data.user_tags();


    UserData userTags;
    for(int i = 0; i < taskTags.searched_tags_size(); i++)
        userTags.activeTags.insert(taskTags.searched_tags(i));
    for(int i = 0; i < taskTags.all_tags_size(); i++)
        userTags.allTags.insert(taskTags.all_tags(i));

    const auto& ignoredFandoms = user_data.ignored_fandoms();
    for(int i = 0; i < ignoredFandoms.fandom_ids_size(); i++)
        userTags.ignoredFandoms[ignoredFandoms.fandom_ids(i)] = ignoredFandoms.ignore_crossovers(i);

    UserInfoAccessor::userData[userToken] = userTags;
}

class DatabaseContext{

public:
    DatabaseContext(){
        QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
        auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
        dbInterface->ReadDbFile("dbcode/dbinit.sql");
    }

    QSharedPointer<database::IDBWrapper> dbInterface;
};


class FeederService final : public ProtoSpace::Feeder::Service {
public:
    Status Search(ServerContext* context, const ProtoSpace::SearchTask* task,
                  ProtoSpace::SearchResponse* response) override
    {
        Q_UNUSED(context);
        QString userToken = QString::fromStdString(task->controls().user_token());
        QLOG_INFO() << "////////////Received search task from: " << userToken;

        core::StoryFilter filter;
        TimedAction ("Converting filter",[&](){
            filter = ProtoFilterIntoStoryFilter(task->filter());
        }).run();
        //filter.Log();

        QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
        auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
        dbInterface->ReadDbFile("dbcode/dbinit.sql");

        QSharedPointer<FicSource> ficSource;
        ficSource.reset(new FicSourceDirect(dbInterface));
        FicSourceDirect* convertedFicSource = dynamic_cast<FicSourceDirect*>(ficSource.data());
        convertedFicSource->InitQueryType(true, userToken);

        ProcessUserToken(task->user_data(), userToken);

        QVector<core::Fic> data;
        TimedAction action("Fetching data",[&](){
            ficSource->FetchData(filter, &data);
        });
        action.run();
        QLOG_INFO() << "Fetch performed in: " << action.ms;
        TimedAction convertAction("Converting data",[&](){
            for(const auto& fic: data)
                LocalFicToProtoFic(fic, response->add_fanfics());
        });
        convertAction.run();
        QLOG_INFO() << "Convert performed in: " << convertAction.ms;

        QLOG_INFO() << "Fetched fics: " << data.size();
        QLOG_INFO() << " ";
        QLOG_INFO() << " ";
        QLOG_INFO() << " ";
        return Status::OK;
    }

    Status GetFicCount(ServerContext* context, const ProtoSpace::FicCountTask* task,
                       ProtoSpace::FicCountResponse* response) override
    {
        Q_UNUSED(context);
        QString userToken = QString::fromStdString(task->controls().user_token());
        QLOG_INFO() << "////////////Received fic count task from: " << userToken;
        core::StoryFilter filter;
        TimedAction ("Converting filter",[&](){
            filter = ProtoFilterIntoStoryFilter(task->filter());
        }).run();
        //filter.Log();

        QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
        auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
        dbInterface->ReadDbFile("dbcode/dbinit.sql");

        QSharedPointer<FicSource> ficSource;
        ficSource.reset(new FicSourceDirect(dbInterface));
        QVector<core::Fic> data;

        ProcessUserToken(task->user_data(), userToken);

        int count = 0;
        TimedAction ("Getting fic count",[&](){
            count = ficSource->GetFicCount(filter);
        }).run();

        response->set_fic_count(count);
        QLOG_INFO() << " ";
        QLOG_INFO() << " ";
        QLOG_INFO() << " ";
        return Status::OK;
    }

    Status SyncFandomList(ServerContext* context, const ProtoSpace::SyncFandomListTask* task,
                          ProtoSpace::SyncFandomListResponse* response) override
    {
        Q_UNUSED(context);
        QLOG_INFO() << "////////////Received sync fandoms task from: " << QString::fromStdString(task->controls().user_token());

        QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
        auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
        dbInterface->ReadDbFile("dbcode/dbinit.sql");
        QSharedPointer<interfaces::Fandoms> fandomInterface (new interfaces::Fandoms());
        fandomInterface->portableDBInterface = dbInterface;
        auto lastServerFandomID = fandomInterface->GetLastFandomID();
        QLOG_INFO() << "Client last fandom ID: " << task->last_fandom_id();
        QLOG_INFO() << "Server last fandom ID: " << lastServerFandomID;
        if(lastServerFandomID == task->last_fandom_id())
            response->set_needs_update(false);
        else
        {
            TimedAction ("Processing fandoms",[&](){
                response->set_needs_update(true);
                auto fandoms = fandomInterface->LoadAllFandomsAfter(task->last_fandom_id());
                QLOG_INFO() << "Sending new fandoms to the client:" << fandoms.size();
                for(auto coreFandom: fandoms)
                {
                    auto* protoFandom = response->add_fandoms();
                    LocalFandomToProtoFandom(*coreFandom, protoFandom);
                }
            }).run();
        }
        QLOG_INFO() << "Finished fandom sync task";
        QLOG_INFO() << " ";
        QLOG_INFO() << " ";
        QLOG_INFO() << " ";
        return Status::OK;
    }
    Status RecommendationListCreation(ServerContext* context, const ProtoSpace::RecommendationListCreationRequest* task,
                                      ProtoSpace::RecommendationListCreationResponse* response) override
    {

        Q_UNUSED(context);
        QLOG_INFO() << "////////////Received sync fandoms task from: " << QString::fromStdString(task->controls().user_token());

        DatabaseContext dbContext;

        QSharedPointer<interfaces::RecommendationLists> recsInterface (new interfaces::RecommendationLists());
        recsInterface->portableDBInterface = dbContext.dbInterface;


        return Status::OK;
    }

};

inline std::string CreateConnectString(QString ip,QString port)
{
    QString server_address_proto("%1:%2");
    std::string result = server_address_proto.arg(ip).arg(port).toStdString();
    return result;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("Flipper");
    SetupLogger();
    QLOG_INFO() << "Feeder app started server";

    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());

    QSettings settings("settings.ini", QSettings::IniFormat);
    auto ip = settings.value("Settings/serverIp", "127.0.0.1").toString();
    auto port = settings.value("Settings/serverPort", "3055").toString();

    std::string server_address = CreateConnectString(ip, port);
    QLOG_INFO() << "Connection string is: " << QString::fromStdString(server_address);
    FeederService service;
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    QLOG_INFO() << "Starting server";
    server->Wait();
    return a.exec();
}

