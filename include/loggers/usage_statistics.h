#pragma once
#include "logger/QsLog.h"
namespace QsLogging
{
class StatLogger : public Logger{};
}

BIND_TO_SELF_SINGLE(QsLogging::StatLogger);

#define STAT_INFO() QsLogging::Logger::Helper(QsLogging::TraceLevel, An<QsLogging::StatLogger>().getData()).stream() << " "
