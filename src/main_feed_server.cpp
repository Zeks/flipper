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
#include "Interfaces/fanfics.h"
#include "Interfaces/recommendation_lists.h"
#include "include/sqlitefunctions.h"
#include "include/db_fixers.h"
//#include "include/feeder_environment.h"
#include "include/Interfaces/data_source.h"
#include "include/Interfaces/ffn/ffn_authors.h"
#include "include/Interfaces/ffn/ffn_fanfics.h"
#include "include/timeutils.h"
#include "include/favholder.h"
#include "include/in_tag_accessor.h"
#include "include/tokenkeeper.h"
//#include "include/tasks/recommendations_reload_precessor.h"
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
#define TO_STR2(x) #x
#define STRINGIFY(x) TO_STR2(x)


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

bool VerifyUserToken(QString userToken)
{
    QRegularExpression rx("{[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}}");
    if(userToken.length() != 38)
        return false;
    auto match = rx.match(userToken);
    if(!match.hasMatch())
        return false;
    return true;
}


bool ProcessUserToken(const ::ProtoSpace::UserData& user_data, QString userToken)
{
    if(!VerifyUserToken(userToken))
        return false;

    const auto& taskTags = user_data.user_tags();


    //UserData userTags;
    QSharedPointer<UserData> userTags = QSharedPointer<UserData>(new UserData);
    for(int i = 0; i < taskTags.searched_tags_size(); i++)
        userTags->ficIDsForActivetags.insert(taskTags.searched_tags(i));
    for(int i = 0; i < taskTags.all_tags_size(); i++)
        userTags->allTaggedFics.insert(taskTags.all_tags(i));

    const auto& ignoredFandoms = user_data.ignored_fandoms();
    for(int i = 0; i < ignoredFandoms.fandom_ids_size(); i++)
        userTags->ignoredFandoms[ignoredFandoms.fandom_ids(i)] = ignoredFandoms.ignore_crossovers(i);

    //    An<UserInfoAccessor> accessor;
    //    accessor->SetData(userToken, userTags);
    auto* userData = ThreadData::GetUserData();
    *userData = *userTags;

    return true;
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

void SetTokenError(ProtoSpace::ResponseInfo* info){
    info->set_is_valid(false);
    info->set_has_usertoken_error(true);
}
void SetFilterDataError(ProtoSpace::ResponseInfo* info){
    info->set_is_valid(false);
    info->set_data_size_limit_reached(true);
}
void SetRecommedationDataError(ProtoSpace::ResponseInfo* info){
    info->set_is_valid(false);
    info->set_data_size_limit_reached(true);
}
void SetFicIDSyncDataError(ProtoSpace::ResponseInfo* info){
    info->set_is_valid(false);
    info->set_data_size_limit_reached(true);
}
void SetTokenMatchError(ProtoSpace::ResponseInfo* info){
    info->set_is_valid(false);
    info->set_token_in_use(true);
}

bool VerifyIDPack(const ::ProtoSpace::SiteIDPack& idPack)
{
    if(idPack.ffn_ids().size() == 0 && idPack.ao3_ids().size() == 0 && idPack.sb_ids().size() == 0 && idPack.sv_ids().size() == 0 )
        return false;
    if(idPack.ffn_ids().size() > 10000)
        return false;
    if(idPack.ao3_ids().size() > 10000)
        return false;
    if(idPack.sb_ids().size() > 10000)
        return false;
    if(idPack.sv_ids().size() > 10000)
        return false;
    return true;
}


bool VerifyRecommendationsRequest(const ProtoSpace::RecommendationListCreationRequest* request)
{
    if(request->list_name().size() > 100)
        return false;
    if(request->min_fics_to_match() <= 0 || request->min_fics_to_match() > 10000)
        return false;
    if(request->max_unmatched_to_one_matched() <= 0)
        return false;
    if(request->always_pick_at() < 1)
        return false;
    if(!VerifyIDPack(request->id_packs()))
        return false;
    return true;
}


class FeederService final : public ProtoSpace::Feeder::Service {
public:
    Status GetStatus(ServerContext* context, const ProtoSpace::StatusRequest* task,
                     ProtoSpace::StatusResponse* response) override
    {
        Q_UNUSED(context);
        QString userToken = QString::fromStdString(task->controls().user_token());
        QLOG_INFO() << "Received status request from: " << userToken;
        QSettings settings("settings_server.ini", QSettings::IniFormat);
        auto motd = settings.value("Settings/motd", "Have fun searching.").toString();
        bool attached = settings.value("Settings/DBAttached", true).toBool();
        response->set_message_of_the_day(motd.toStdString());
        response->set_database_attached(attached);
        response->set_need_to_show_motd(settings.value("Settings/motdRequired", false).toBool());
        auto protocolVersion = QString(STRINGIFY(PROTOCOL_VERSION)).toInt();
        QLOG_INFO() << "Passing protocol version: " << protocolVersion;
        response->set_protocol_version(protocolVersion);
        return Status::OK;
    }
    Status Search(ServerContext* context, const ProtoSpace::SearchTask* task,
                  ProtoSpace::SearchResponse* response) override
    {
        Q_UNUSED(context);
        QString userToken = QString::fromStdString(task->controls().user_token());
        QLOG_INFO() << "////////////Received search task from: " << userToken;
        if(!ProcessUserToken(task->user_data(), userToken))
        {
            SetTokenError(response->mutable_response_info());
            return Status::OK;
        }
        if(!VerifyFilterData(task->filter(), task->user_data()))
        {
            SetFilterDataError(response->mutable_response_info());
            return Status::OK;
        }
        An<UserTokenizer> keeper;
        auto safetyToken = keeper->GetToken(userToken);
        if(!safetyToken)
        {
            SetTokenMatchError(response->mutable_response_info());
            return Status::OK;
        }

        core::StoryFilter filter;
        TimedAction ("Converting filter",[&](){
            filter = proto_converters::ProtoIntoStoryFilter(task->filter(), task->user_data());
        }).run();
        //filter.Log();

        QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
        auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);

        static QSharedPointer<FicSource> ficSource(new FicSourceDirect(dbInterface));
        FicSourceDirect* convertedFicSource = dynamic_cast<FicSourceDirect*>(ficSource.data());
        convertedFicSource->InitQueryType(true, userToken);

        auto* recs = ThreadData::GetRecommendationData();

        recs->recommendationList = filter.recsHash;

        QVector<core::Fic> data;
        TimedAction action("Fetching data",[&](){
            ficSource->FetchData(filter, &data);
        });
        action.run();
        QLOG_INFO() << "Fetch performed in: " << action.ms;
        TimedAction convertAction("Converting data",[&](){
            for(const auto& fic: data)
                proto_converters::LocalFicToProtoFic(fic, response->add_fanfics());
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
        if(!ProcessUserToken(task->user_data(), userToken))
        {
            SetTokenError(response->mutable_response_info());
            return Status::OK;
        }
        if(!VerifyFilterData(task->filter(), task->user_data()))
        {
            SetFilterDataError(response->mutable_response_info());
            return Status::OK;
        }
        An<UserTokenizer> keeper;
        auto safetyToken = keeper->GetToken(userToken);
        if(!safetyToken)
        {
            SetTokenMatchError(response->mutable_response_info());
            return Status::OK;
        }
        core::StoryFilter filter;
        TimedAction ("Converting filter",[&](){
            filter = proto_converters::ProtoIntoStoryFilter(task->filter(), task->user_data());

        }).run();
        //filter.Log();

        QSharedPointer<database::IDBWrapper> dbInterface (new database::SqliteInterface());
        auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);

        static QSharedPointer<FicSource> ficSource;
        ficSource.reset(new FicSourceDirect(dbInterface));
        FicSourceDirect* convertedFicSource = dynamic_cast<FicSourceDirect*>(ficSource.data());
        convertedFicSource->InitQueryType(true, userToken);
        QVector<core::Fic> data;

        auto* recs = ThreadData::GetRecommendationData();
        recs->recommendationList = filter.recsHash;

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
        QString userToken = QString::fromStdString(task->controls().user_token());
        QLOG_INFO() << "////////////Received sync fandoms task from: " << userToken;

        if(!VerifyUserToken(userToken))
        {
            SetTokenError(response->mutable_response_info());
            return Status::OK;
        }

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
                    proto_converters::LocalFandomToProtoFandom(*coreFandom, protoFandom);
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
        QString userToken = QString::fromStdString(task->controls().user_token());
        QLOG_INFO() << "////////////Received reclist s task from: " << userToken;

        if(!VerifyUserToken(userToken))
        {
            SetTokenError(response->mutable_response_info());
            QLOG_INFO() << "token error, exiting";
            return Status::OK;
        }
        if(!VerifyRecommendationsRequest(task))
        {
            SetRecommedationDataError(response->mutable_response_info());
            QLOG_INFO() << "data size error, exiting";
            return Status::OK;
        }
        An<UserTokenizer> keeper;
        auto safetyToken = keeper->GetToken(userToken);
        if(!safetyToken)
        {
            SetTokenMatchError(response->mutable_response_info());
            return Status::OK;
        }
        DatabaseContext dbContext;

        QSharedPointer<core::RecommendationList> params(new core::RecommendationList);
        params->name =  proto_converters::FS(task->list_name());
        params->minimumMatch = task->min_fics_to_match();
        params->pickRatio = task->max_unmatched_to_one_matched();
        params->alwaysPickAt = task->always_pick_at();

        QSharedPointer<interfaces::Fanfics> fanficsInterface (new interfaces::FFNFanfics());

        auto* recs = ThreadData::GetRecommendationData();

        QSet<int> sourceFics;
        for(int i = 0; i < task->id_packs().ffn_ids_size(); i++)
        {
            sourceFics.insert(task->id_packs().ffn_ids(i));
            //recs->recommendationList[task->id_packs().ffn_ids(i)]
        }
        if(sourceFics.size() == 0)
            return Status::OK;
        recs->sourceFics = sourceFics;
        TimedAction action("Fic ids conversion",[&](){
            sourceFics = fanficsInterface->ConvertFFNSourceFicsToDB(userToken);
        });
        action.run();

        An<core::FavHolder> holder;
        auto list = holder->GetMatchedFicsForFavList(sourceFics, params);

        auto* targetList = response->mutable_list();
        targetList->set_list_name(proto_converters::TS(params->name));
        targetList->set_list_ready(true);
        for(int key: list.recommendations.keys())
        {
            //QLOG_INFO() << " n_fic_id: " << key << " n_matches: " << list[key];
            if(sourceFics.contains(key))
                continue;
            targetList->add_fic_ids(key);
            targetList->add_fic_matches(list.recommendations[key]);
        }
        for(int key: list.matchReport.keys())
        {
            if(sourceFics.contains(key))
                continue;
            (*targetList->mutable_match_report())[key] = list.matchReport[key];
        }

        return Status::OK;
    }

    Status GetDBFicIDS(ServerContext* context, const ProtoSpace::FicIdRequest* task,
                       ProtoSpace::FicIdResponse* response) override
    {
        Q_UNUSED(context);
        QString userToken = QString::fromStdString(task->controls().user_token());
        QLOG_INFO() << "////////////Received sync fandoms task from: " << userToken;

        if(!VerifyUserToken(userToken))
        {
            SetTokenError(response->mutable_response_info());
            return Status::OK;
        }
        if(!VerifyIDPack(task->ids()))
        {
            SetFicIDSyncDataError(response->mutable_response_info());
            return Status::OK;
        }
        An<UserTokenizer> keeper;
        auto safetyToken = keeper->GetToken(userToken);
        if(!safetyToken)
        {
            SetTokenMatchError(response->mutable_response_info());
            return Status::OK;
        }
        DatabaseContext dbContext;

        QHash<int, int> idsToFill;
        for(int i = 0; i < task->ids().ffn_ids_size(); i++)
            idsToFill[task->ids().ffn_ids(i)] = -1;
        QSharedPointer<interfaces::Fanfics> fanficsInterface (new interfaces::FFNFanfics());
        bool result = fanficsInterface->ConvertFFNTaggedFicsToDB(idsToFill);
        if(!result)
        {
            response->set_success(false);
            return Status::OK;
        }
        response->set_success(true);
        for(int fic: idsToFill.keys())
        {
            QLOG_INFO() << "Returning fic ids: " << "DB: " << idsToFill[fic] << " FFN: " << fic;
            response->mutable_ids()->add_ffn_ids(fic);
            response->mutable_ids()->add_db_ids(idsToFill[fic]);
        }
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
    auto mainDb = dbInterface->InitDatabase("CrawlerDB", true);
    dbInterface->ReadDbFile("dbcode/dbinit.sql");


    An<core::FavHolder> holder;
    auto authors = QSharedPointer<interfaces::Authors> (new interfaces::FFNAuthors());
    authors->db = mainDb;
    holder->LoadFavourites(authors);

    QSettings settings("settings_server.ini", QSettings::IniFormat);
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

