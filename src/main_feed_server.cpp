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
#include "include/sqlitefunctions.h"
#include "include/db_fixers.h"
#include "include/feeder_environment.h"
#include "include/Interfaces/data_source.h"
#include "grpc/grpc_source.h"
#include "proto/feeder_service.grpc.pb.h"
#include "proto/feeder_service.pb.h"

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


class FeederService final : public ProtoSpace::Feeder::Service {
public:
    Status Search(ServerContext* context, const ProtoSpace::SearchTask* task,
                 ProtoSpace::SearchResponse* response) override
    {
        Q_UNUSED(context);

        auto filter = ProtoFilterIntoStoryFilter(task->filter());

        QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
        auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
        dbInterface->ReadDbFile("dbcode/dbinit.sql");

        QSharedPointer<FicSource> ficSource;
        ficSource.reset(new FicSourceDirect(dbInterface));
        QVector<core::Fic> data;
        ficSource->FetchData(filter, &data);
        for(const auto& fic: data)
            LocalFicToProtoFic(fic, response->add_fanfics());
        return Status::OK;
    }

    Status GetFicCount(ServerContext* context, const ProtoSpace::FicCountTask* task,
                 ProtoSpace::FicCountResponse* response) override
    {
        Q_UNUSED(context);

        auto filter = ProtoFilterIntoStoryFilter(task->filter());

        QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
        auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
        dbInterface->ReadDbFile("dbcode/dbinit.sql");

        QSharedPointer<FicSource> ficSource;
        ficSource.reset(new FicSourceDirect(dbInterface));
        QVector<core::Fic> data;
        int count = ficSource->GetFicCount(filter);
        response->set_fic_count(count);
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
    QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());

    QSettings settings("settings.ini", QSettings::IniFormat);
    auto ip = settings.value("Settings/serverIp", "127.0.0.1").toString();
    auto port = settings.value("Settings/serverPort", "3055").toString();

    std::string server_address = CreateConnectString(ip, port);
    FeederService service;
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    server->Wait();
    return a.exec();
}

