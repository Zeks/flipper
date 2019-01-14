#pragma once


#include <QDateTime>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <QSet>
#include <QTimer>
#include <QObject>
#include <atomic>

#include "proto/feeder_service.grpc.pb.h"
#include "proto/feeder_service.pb.h"
#include "include/tokenkeeper.h"
#include "include/storyfilter.h"
#include "servers/database_context.h"
#include "rng.h"


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
class FicSource;


struct UsedInSearch{
  QSharedPointer<FicSource> ficSource;
  core::StoryFilter filter;
  bool isValid = false;
};

struct StatisticsToken
{
    QDateTime lastUsed;
    QString uuid;
    QList<QDateTime> usedAt;
    int genericSearches = 0;
    int recommendationsSearches = 0;
    int randomSearches = 0;
};
struct RecommendationsData;
class FeederService;
struct RequestContext{
    RequestContext(QString requestName, const ::ProtoSpace::ControlInfo&, FeederService* server);
    bool Process(::ProtoSpace::ResponseInfo*);
    QSharedPointer<UserToken<UserTokenizer>> safetyToken;
    QString userToken;
    FeederService* server;
    DatabaseContext dbContext;
    RecommendationsData* recsData;
};

class FeederService final : public QObject, public ProtoSpace::Feeder::Service  {
Q_OBJECT
    friend struct  RequestContext;
public:
    FeederService(QObject* parent = nullptr);
    ~FeederService() override;

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

    Status GetFFNFicIDS(ServerContext* context, const ProtoSpace::FicIdRequest* task,
                       ProtoSpace::FicIdResponse* response) override;

    Status GetFavListDetails(ServerContext* context, const ProtoSpace::FavListDetailsRequest* task,
                       ProtoSpace::FavListDetailsResponse* response) override;
    Status GetAuthorsForFicList(ServerContext* context, const ProtoSpace::AuthorsForFicsRequest* task,
                       ProtoSpace::AuthorsForFicsResponse* response) override;
    Status GetAuthorsFromRecListContainingFic(ServerContext* context, const ProtoSpace::AuthorsForFicInReclistRequest* task,
                       ProtoSpace::AuthorsForFicInReclistResponse* response) override;
    Status SearchByFFNID(ServerContext* context, const ProtoSpace::SearchByFFNIDTask* task,
                       ProtoSpace::SearchByFFNIDResponse* response) override;
    Status GetUserMatches(ServerContext* context, const ProtoSpace::UserMatchRequest* task,
                       ProtoSpace::UserMatchResponse* response) override;



    QHash<QString, StatisticsToken> tokenData;
    QHash<QDateTime, QString> serverUses;
    std::atomic<int> allSearches;
    std::atomic<int> genericSearches;
    std::atomic<int> recommendationsSearches;
    std::atomic<int> randomSearches;
    QSet<QString> allTokens;
    QSet<QString> recommendationTokens;
    QSet<QString> searchedTokens;
    QDateTime startedAt;
    QReadWriteLock lock;
    QSharedPointer<QTimer> logTimer;
    QSharedPointer<core::RNGData> rngData;
private:
    void AddToStatistics(QString uuid, const core::StoryFilter& filter);
    void AddToStatistics(QString uuid);
    void AddToRecStatistics(QString uuid);
    void CleaupOldTokens();
    void PrintStatistics();
    bool VerifySearchInput(QString,
                           const ::ProtoSpace::Filter&,
                           const ::ProtoSpace::UserData&,
                           ::ProtoSpace::ResponseInfo*);
    core::StoryFilter FilterFromTask(const ::ProtoSpace::Filter&,
                                     const ::ProtoSpace::UserData&);

    QSharedPointer<FicSource> InitFicSource(QString userToken, QSharedPointer<database::IDBWrapper> dbInterface);
    QSet<int> ProcessIDPackIntoFfnFicSet(const ::ProtoSpace::SiteIDPack& );
    QSet<int> ProcessFFNIDPackIntoFfnFicSet(const ProtoSpace::SiteIDPack & pack);
    UsedInSearch PrepareSearch(::ProtoSpace::ResponseInfo* response,
                               const ::ProtoSpace::Filter& filter,
                               const ::ProtoSpace::UserData& userData,
                               RequestContext& reqContext);
public slots:
    void OnPrintStatistics();
};
