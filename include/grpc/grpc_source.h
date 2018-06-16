#pragma once
#include "include/storyfilter.h"
#include "include/Interfaces/data_source.h"
#include <QSharedPointer>

class FicSourceGRPCImpl;
class FicSourceGRPC : public FicSource
{
public:
    FicSourceGRPC();
    virtual ~FicSourceGRPC() override;

    QSharedPointer<FicSourceGRPCImpl> impl;
    virtual void FetchData(core::StoryFilter filter, QList<core::Fic>*) override;
    virtual int GetFicCount(core::StoryFilter filter) override;


};
