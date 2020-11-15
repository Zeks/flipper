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
#include "servers/token_processing.h"
#include "in_tag_accessor.h"
#include "logger/QsLog.h"
#include "proto/feeder_service.grpc.pb.h"
#include "proto/feeder_service.pb.h"

#include <QRegularExpression>

bool VerifyUserToken(QString userToken, ProtoSpace::ResponseInfo* info)
{
    //QLOG_INFO() << "Verifying user token";
    QRegularExpression rx("{[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}}");
    if(userToken.length() != 38)
        return false;
    auto match = rx.match(userToken);
    if(!match.hasMatch())
    {
        SetTokenError(info);
        QLOG_INFO() << "token error, exiting";
        return false;
    }
    return true;
}


bool ProcessUserToken(const ::ProtoSpace::UserData& user_data, QString userToken, ProtoSpace::ResponseInfo * responseInfo)
{
    if(!VerifyUserToken(userToken, responseInfo))
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
