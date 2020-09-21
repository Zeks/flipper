#pragma once

#include <QVector>
#include <QSharedPointer>
#include "Interfaces/discord/users.h"
#include "grpc/grpc_source.h"

namespace discord{

void FetchFicsForDisplayPageCommand(QSharedPointer<FicSourceGRPC> source,
                                    QSharedPointer<discord::User> user,
                                    int size,
                                    QVector<core::Fanfic>* fics);

int FetchPageCountForFilterCommand(QSharedPointer<FicSourceGRPC> source,
                                    QSharedPointer<discord::User> user,
                                    int size);

void FetchFicsForDisplayRngCommand(int size,
                                   QSharedPointer<FicSourceGRPC> source,
                                   QSharedPointer<discord::User> user,
                                   QVector<core::Fanfic>* fics,
                                   int qualityCutoff = 1);


}
