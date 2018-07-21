#include "servers/token_processing.h"
#include "in_tag_accessor.h"
#include "Logger/QsLog.h"
#include "proto/feeder_service.grpc.pb.h"
#include "proto/feeder_service.pb.h"

#include <QRegularExpression>

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
