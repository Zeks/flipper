#pragma once
#include <QString>
namespace  ProtoSpace{
class UserData;
class ResponseInfo;
}
bool VerifyUserToken(QString userToken);
bool ProcessUserToken(const ProtoSpace::UserData& user_data, QString userToken);
void SetTokenError(ProtoSpace::ResponseInfo* info);
void SetFilterDataError(ProtoSpace::ResponseInfo* info);
void SetRecommedationDataError(ProtoSpace::ResponseInfo* info);
void SetFicIDSyncDataError(ProtoSpace::ResponseInfo* info);
void SetTokenMatchError(ProtoSpace::ResponseInfo* info);
