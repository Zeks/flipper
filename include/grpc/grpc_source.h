#pragma once
#include "include/storyfilter.h"
#include "include/Interfaces/data_source.h"
#include <QSharedPointer>

class FicSourceGRPCImpl;
namespace core {
struct StoryFilter;
}

namespace ProtoSpace {
class Filter;
class Fanfic;
class Fandom;
}

class FicSourceGRPC : public FicSource
{
public:
    FicSourceGRPC(QString connectionString, int deadline);
    virtual ~FicSourceGRPC() override;

    QSharedPointer<FicSourceGRPCImpl> impl;
    virtual void FetchData(core::StoryFilter filter, QVector<core::Fic>*) override;
    virtual int GetFicCount(core::StoryFilter filter) override;


};

ProtoSpace::Filter StoryFilterIntoProtoFilter(const core::StoryFilter& filter);
core::StoryFilter ProtoFilterIntoStoryFilter(const ProtoSpace::Filter& filter);

bool ProtoFicToLocalFic(const ProtoSpace::Fanfic& protoFic, core::Fic& coreFic);
bool LocalFicToProtoFic(const core::Fic& coreFic, ProtoSpace::Fanfic *protoFic);

void LocalFandomToProtoFandom(const core::Fandom& coreFandom, ProtoSpace::Fandom* protoFandom);
void ProtoFandomToLocalFandom(const ProtoSpace::Fandom& protoFandom, core::Fandom &coreFandom);
