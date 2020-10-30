#pragma once
#include "logger/QsLog.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#include "google/protobuf/util/time_util.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/empty.pb.h"
#pragma GCC diagnostic pop

namespace grpcutils{
void DumpToLog(QString precede, const ::google::protobuf::Message* message);
}

