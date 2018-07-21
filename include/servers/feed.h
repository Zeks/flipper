#pragma once


#include <QDateTime>
#include <QReadWriteLock>
#include <QSet>
#include <atomic>

#include "proto/feeder_service.grpc.pb.h"
#include "proto/feeder_service.pb.h"

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <google/protobuf/text_format.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;
namespace core{
class StoryFilter;
}
struct StatisticsToken
{
    QDateTime lastUsed;
    QString uuid;
    QList<QDateTime> usedAt;
    int genericSearches = 0;
    int recommendationsSearches = 0;
    int randomSearches = 0;
};


class FeederService final : public ProtoSpace::Feeder::Service {
public:
    FeederService();

    Status GetStatus(ServerContext* context, const ProtoSpace::StatusRequest* task,
                     ProtoSpace::StatusResponse* response) override;

    Status Search(ServerContext* context, const ProtoSpace::SearchTask* task,
                  ProtoSpace::SearchResponse* response) override;


    Status GetFicCount(ServerContext* context, const ProtoSpace::FicCountTask* task,
                       ProtoSpace::FicCountResponse* response) override;


    Status SyncFandomList(ServerContext* context, const ProtoSpace::SyncFandomListTask* task,
                          ProtoSpace::SyncFandomListResponse* response) override;

    Status RecommendationListCreation(ServerContext* context, const ProtoSpace::RecommendationListCreationRequest* task,
                                      ProtoSpace::RecommendationListCreationResponse* response) override;


    Status GetDBFicIDS(ServerContext* context, const ProtoSpace::FicIdRequest* task,
                       ProtoSpace::FicIdResponse* response) override;



    QHash<QString, StatisticsToken> tokenData;
    QHash<QDateTime, QString> serverUses;
    std::atomic<int> genericSearches;
    std::atomic<int> recommendationsSearches;
    std::atomic<int> randomSearches;
    QSet<QString> allTokens;
    QSet<QString> searchedTokens;
    QDateTime startedAt;
    QReadWriteLock lock;
private:
    void AddToStatistics(QString uuid, const core::StoryFilter& filter);
    void CleaupOldTokens();
    void PrintStatitics();
};
