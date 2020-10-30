#include "grpc/grpc_log.h"
namespace grpcutils{
void DumpToLog(QString precede, const ::google::protobuf::Message* message)
{
    std::string asText;
    ::google::protobuf::TextFormat::Printer printer;
    printer.SetUseUtf8StringEscaping(true);
    printer.PrintToString(*message, &asText);
    QLOG_INFO() << precede << QString::fromStdString(asText);
}

}
