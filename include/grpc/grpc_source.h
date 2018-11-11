/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2018  Marchenko Nikolai

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
#include <QSharedPointer>
#include <QVector>

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
class RecommendationListCreationRequest;
}

struct RecommendationListGRPC
{
    core::RecommendationList listParams;
    QVector<int> fics;
    QVector<int> matchCounts;
    QVector<int> authorIds;
    QHash<int, int> matchReport;
};

struct ServerStatus
{
    ServerStatus(){}
    ServerStatus(bool isValid,
                 bool dbAttached,
                 bool messageRequired,
                 QDateTime lastDBUpdate,
                 QString motd):isValid(isValid), dbAttached(dbAttached), messageRequired(messageRequired), lastDBUpdate(lastDBUpdate),motd(motd) {}
    bool isValid = false;
    bool dbAttached = false;
    bool messageRequired = false;
    bool protocolVersionMismatch = false;
    QDateTime lastDBUpdate;
    QString motd;
    QString error;
};

class FicSourceGRPC : public FicSource
{
public:
    FicSourceGRPC(QString connectionString, QString userToken, int deadline);
    virtual ~FicSourceGRPC() override;

    virtual void FetchData(core::StoryFilter filter, QVector<core::Fic>*) override;
    virtual int GetFicCount(core::StoryFilter filter) override;
    bool GetFandomListFromServer(int lastFandomID, QVector<core::Fandom>* fandoms);
    bool GetRecommendationListFromServer(RecommendationListGRPC& recList);
    bool GetInternalIDsForFics(QVector<core::IdPack>*);
    bool GetFFNIDsForFics(QVector<core::IdPack>*);

    ServerStatus GetStatus();

    QString userToken;
    QSharedPointer<FicSourceGRPCImpl> impl;
};

bool VerifyFilterData(const ProtoSpace::Filter& filter, const ProtoSpace::UserData &user);
bool VerifyIDPack(const ::ProtoSpace::SiteIDPack& idPack);
bool VerifyRecommendationsRequest(const ProtoSpace::RecommendationListCreationRequest* request);

namespace proto_converters
{

ProtoSpace::Filter StoryFilterIntoProto(const core::StoryFilter& filter, ProtoSpace::UserData *userData);
core::StoryFilter ProtoIntoStoryFilter(const ProtoSpace::Filter& filter, const ProtoSpace::UserData &userData);

bool ProtoFicToLocalFic(const ProtoSpace::Fanfic& protoFic, core::Fic& coreFic);
bool LocalFicToProtoFic(const core::Fic& coreFic, ProtoSpace::Fanfic *protoFic);

void LocalFandomToProtoFandom(const core::Fandom& coreFandom, ProtoSpace::Fandom* protoFandom);
void ProtoFandomToLocalFandom(const ProtoSpace::Fandom& protoFandom, core::Fandom &coreFandom);
QString FS(const std::string& s);
std::string TS(const QString& s);
QDateTime DFS(const std::string& s);
std::string DTS(const QDateTime & date);
}
