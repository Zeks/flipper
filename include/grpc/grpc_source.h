/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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
#pragma once
#include "include/storyfilter.h"
#include "include/Interfaces/data_source.h"
#include "include/rec_calc/rec_calculator_base.h"
#include "reclist_author_result.h"

#include <QSharedPointer>
#include <QVector>
#include <optional>

class FicSourceGRPCImpl;
namespace core {
struct StoryFilter;
}

namespace ProtoSpace {
class Filter;
class Fanfic;
class Fandom;
class UserData;
class SiteIDPack;
class FavListDetails;
class RecommendationListCreationRequest;
class AuthorsForFicsResponse;
class ResponseInfo;
class ControlInfo;
}



struct ServerStatus
{
    ServerStatus(){}
    ServerStatus(bool isValid,
                 bool dbAttached,
                 bool messageRequired,
                 QString lastDBUpdate,
                 QString motd):isValid(isValid), dbAttached(dbAttached), messageRequired(messageRequired), lastDBUpdate(lastDBUpdate),motd(motd) {}
    bool isValid = false;
    bool dbAttached = false;
    bool messageRequired = false;
    bool protocolVersionMismatch = false;
    QString lastDBUpdate;
    QString motd;
    QString error;
};

struct InputsForMatches{
    QStringList userFics;
    QStringList userIgnores;
};

class FicSourceGRPC : public FicSource
{
public:
    FicSourceGRPC(QString connectionString, QString userToken, int deadline);
    virtual ~FicSourceGRPC() override;

    virtual void FetchData(const core::StoryFilter& filter, QVector<core::Fanfic>*) override;
    void FetchFic(int ficId, QVector<core::Fanfic>*fics, core::StoryFilter::EUseThisFicType idType = core::StoryFilter::EUseThisFicType::utf_ffn_id);
    virtual int GetFicCount(const core::StoryFilter& filter) override;
    bool GetFandomListFromServer(int lastFandomID, QVector<core::Fandom>* fandoms);

    core::DiagnosticsForReclist GetDiagnosticsForRecListFromServer(QSharedPointer<core::RecommendationList> recList);
    bool GetRecommendationListFromServer(QSharedPointer<core::RecommendationList> recList);
    bool GetInternalIDsForFics(QVector<core::Identity>*);
    bool GetFFNIDsForFics(QVector<core::Identity>*);
    std::optional<core::FavListDetails> GetStatsForFicList(QVector<core::Identity>);
    QHash<uint32_t, uint32_t> GetAuthorsForFicList(QSet<int>);
    QSet<int> GetAuthorsForFicInRecList(int sourceFic, QString authors);
    QHash<int, core::FavouritesMatchResult> GetMatchesForUsers(int sourceUser, QList<int> users);
    QHash<int, core::FavouritesMatchResult> GetMatchesForUsers(InputsForMatches, QList<int> users);
    QSet<int> GetExpiredSnoozes(QHash<int, core::FanficSnoozeStatus>);

    ServerStatus GetStatus();
    void SetUserToken(QString token);
    void ClearUserData();

    QString userToken;
    QSharedPointer<FicSourceGRPCImpl> impl;
};

bool VerifyFilterData(const ProtoSpace::Filter& filter, const ProtoSpace::UserData &user);
bool VerifyIDPack(const ::ProtoSpace::SiteIDPack& idPack, ProtoSpace::ResponseInfo* info);
bool VerifyRecommendationsRequest(const ProtoSpace::RecommendationListCreationRequest* request, ProtoSpace::ResponseInfo *info);


namespace proto_converters
{

ProtoSpace::Filter StoryFilterIntoProto(const core::StoryFilter& filter, ProtoSpace::UserData *userData);
core::StoryFilter ProtoIntoStoryFilter(const ProtoSpace::Filter& filter, const ProtoSpace::UserData &userData);

bool ProtoFicToLocalFic(const ProtoSpace::Fanfic& protoFic, core::Fanfic& coreFic);
bool LocalFicToProtoFic(const core::Fanfic& coreFic, ProtoSpace::Fanfic *protoFic);

bool FavListProtoToLocal(const ProtoSpace::FavListDetails& protoStats, core::FavListDetails& stats);
bool FavListLocalToProto(const core::FavListDetails& stats, ProtoSpace::FavListDetails *protoStats);
bool AuthorListProtoToLocal(const ProtoSpace::AuthorsForFicsResponse& protoStats,  QHash<uint32_t, uint32_t>& result);
void LocalFandomToProtoFandom(const core::Fandom& coreFandom, ProtoSpace::Fandom* protoFandom);
void ProtoFandomToLocalFandom(const ProtoSpace::Fandom& protoFandom, core::Fandom &coreFandom);
QString FS(const std::string& s);
std::string TS(const QString& s);
QDateTime DFS(const std::string& s);
std::string DTS(const QDateTime & date);
}
