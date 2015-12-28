#include "include/Logging/loggerhelper.h"
#include "L_Logger/include/QsLog.h"
#include <QxtBasicSTDLoggerEngine>
#include <QxtBasicFileLoggerEngine>
#include <QFileInfo>
#include <QDateTime>


bool LoggerHelper::traceLevel;
bool LoggerHelper::debugLevel;
bool LoggerHelper::infoLevel;
bool LoggerHelper::warnLevel;
bool LoggerHelper::errorLevel;
bool LoggerHelper::criticalLevel;
QString LoggerHelper::logFileName;
QString LoggerHelper::logFileExtension;
QString LoggerHelper::logFilePath;
int LoggerHelper::maxSize = 10;
LoggerHelper::LoggerHelper()
{
}
void LoggerHelper::SplitFile()
{
    QString logFile = logFilePath + logFileName + "." +  logFileExtension;
    QFileInfo info(logFile);
    bool newLogFile = info.size()/(1000*1000) > maxSize;
    QFile file(logFile);
    file.open(QIODevice::ReadWrite);
    if(newLogFile)
    {
        SaveLoggerStatus();
        qxtLog->removeLoggerEngine("std");
        qxtLog->removeLoggerEngine("file");
        file.rename(logFilePath + logFileName + QDateTime::currentDateTime().toString("hh-ddMM") + "." + logFileExtension);
        file.close();
        qxtLog->addLoggerEngine("file", new QxtBasicFileLoggerEngine(logFilePath + logFileName + "." + logFileExtension));
        qxtLog->addLoggerEngine("std", new QxtBasicSTDLoggerEngine());
        RestoreLoggerStatus();
    }
}

void LoggerHelper::InitLogger(int logLevel)
{
    QString logFile = logFilePath + logFileName + "." + logFileExtension;
    QFile file(logFile);
    file.open(QIODevice::ReadWrite);
    qxtLog->addLoggerEngine("file", new QxtBasicFileLoggerEngine(logFilePath + logFileName + "." + logFileExtension));
    qxtLog->addLoggerEngine("std", new QxtBasicSTDLoggerEngine());
    qxtLog->disableAllLogLevels();
    switch(logLevel)
    {
    case -2:
        break;
    case -1:
        qxtLog->enableLogLevels("std", QxtLogger::AllLevels);
        qxtLog->enableLogLevels("file", QxtLogger::AllLevels);
        break;
    case 0:
        qxtLog->enableLogLevels("std", QxtLogger::TraceLevel | QxtLogger::DebugLevel | QxtLogger::InfoLevel | QxtLogger::ErrorLevel | QxtLogger::WarningLevel | QxtLogger::CriticalLevel);
        qxtLog->enableLogLevels("file", QxtLogger::TraceLevel | QxtLogger::DebugLevel |QxtLogger::InfoLevel | QxtLogger::ErrorLevel | QxtLogger::WarningLevel | QxtLogger::CriticalLevel);
        break;
    case 1:
        qxtLog->enableLogLevels("std", QxtLogger::DebugLevel | QxtLogger::InfoLevel | QxtLogger::ErrorLevel | QxtLogger::WarningLevel | QxtLogger::CriticalLevel);
        qxtLog->enableLogLevels("file", QxtLogger::DebugLevel | QxtLogger::InfoLevel | QxtLogger::ErrorLevel | QxtLogger::WarningLevel | QxtLogger::CriticalLevel);
        break;
    case 2:
        qxtLog->enableLogLevels("std", QxtLogger::InfoLevel | QxtLogger::ErrorLevel | QxtLogger::WarningLevel | QxtLogger::CriticalLevel);
        qxtLog->enableLogLevels("file",  QxtLogger::InfoLevel | QxtLogger::ErrorLevel | QxtLogger::WarningLevel | QxtLogger::CriticalLevel);
        break;
    case 3:
        qxtLog->enableLogLevels("std",  QxtLogger::ErrorLevel | QxtLogger::WarningLevel | QxtLogger::CriticalLevel);
        qxtLog->enableLogLevels("file",   QxtLogger::ErrorLevel | QxtLogger::WarningLevel | QxtLogger::CriticalLevel);
        break;
    case 4:
        qxtLog->enableLogLevels("std",  QxtLogger::ErrorLevel | QxtLogger::CriticalLevel);
        qxtLog->enableLogLevels("file",   QxtLogger::ErrorLevel  | QxtLogger::CriticalLevel);
        break;
    }
    SplitFile();
}
void LoggerHelper::SetParams(QString path, QString name, QString extension, int size)
{
    logFilePath = path;
    logFileName = name;
    logFileExtension = extension;
    maxSize = size;
}
void LoggerHelper::SaveLoggerStatus()
{
    traceLevel = qxtLog->isLogLevelEnabled("std", QxtLogger::TraceLevel);
    debugLevel = qxtLog->isLogLevelEnabled("std", QxtLogger::DebugLevel);
    infoLevel = qxtLog->isLogLevelEnabled("std", QxtLogger::InfoLevel);
    warnLevel = qxtLog->isLogLevelEnabled("std", QxtLogger::WarningLevel);
    errorLevel = qxtLog->isLogLevelEnabled("std", QxtLogger::ErrorLevel);
    criticalLevel = qxtLog->isLogLevelEnabled("std", QxtLogger::CriticalLevel);
}
void LoggerHelper::RestoreLoggerStatus()
{
    if(traceLevel)
        qxtLog->enableLogLevels(QxtLogger::TraceLevel);
    if(debugLevel)
        qxtLog->enableLogLevels(QxtLogger::DebugLevel);
    if(infoLevel)
        qxtLog->enableLogLevels(QxtLogger::InfoLevel);
    if(warnLevel)
        qxtLog->enableLogLevels(QxtLogger::WarningLevel);
    if(errorLevel)
        qxtLog->enableLogLevels(QxtLogger::ErrorLevel);
    if(criticalLevel)
        qxtLog->enableLogLevels(QxtLogger::CriticalLevel);
}
