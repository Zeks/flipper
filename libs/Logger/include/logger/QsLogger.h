#ifndef QSLOGGER_H
#define QSLOGGER_H
#ifdef LOGGER
#include "QsLog.h"
#else
class L_LOGGERSHARED_EXPORT Logger
{};
#define QLOG_TRACE() qDebug()
#define QLOG_DEBUG() qDebug()
#define QLOG_INFO()  qDebug()
#define QLOG_WARN()  qDebug()
#define QLOG_ERROR() qDebug()
#define QLOG_FATAL() qDebug()
#endif


#endif // QSLOGGER_H
